// student id: 2024201594
// please change the above line to your student id

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // 使用 getopt 解析命令行参数
#define ERROR 1

void printHelp(const char *name);

// 配置SimConfig：保存命令行解析后的所有参数
typedef struct {
  int s;                  // 组索引位数
  int E;                  // 每组的行数
  int b;                  // 块偏移位数
  int verbose;            // 是否启用详细输出
  const char *trace_file; // trace 文件路径
  int help;               // 是否请求帮助
} SimConfig;

typedef struct {
  char tag[100]; // 行的 tag（二进制字符串形式）
  int time;      // LRU 时间戳（数值越大表示越“新”）
  int effect;    // 有效位：1 表示该行已被占用，0 表示空行
} Row;
typedef struct {
  Row E_all[100]; // 一组内最多 100 路（实际使用 config.E 条）
} SET;
typedef struct {
  SET set[500]; // 最多 500 组（实际使用 2^s 组）
} Cache;

// 解析命令行参数，返回配置；若出错（除 -h
// 外缺失必需参数）程序会直接打印帮助并退出
static SimConfig load_data(int argc, char **argv) {
  SimConfig config = {0};
  int opt;
  while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt) {
    case 'h':
      config.help = 1;
      break;
    case 'v':
      config.verbose = 1;
      break;
    case 's':
      config.s = atoi(optarg);
      break;
    case 'E':
      config.E = atoi(optarg);
      break;
    case 'b':
      config.b = atoi(optarg);
      break;
    case 't':
      config.trace_file = optarg;
      break;
    default:
      fprintf(stderr, "Unknown or malformed option. Use -h for help.\n");
      printHelp(argv[0]);
      exit(ERROR);
    }
  }
  if (!config.help) {
    if (config.s <= 0 || config.E <= 0 || config.b <= 0 || !config.trace_file) {
      fprintf(stderr, "Error: Missing or invalid required options.\n");
      printHelp(argv[0]);
      exit(ERROR);
    }
  }
  return config;
}

void printSummary(int hits, int misses, int evictions) {
  printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
  FILE *output_fp = fopen(".csim_results", "w");
  assert(output_fp);
  fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
  fclose(output_fp);
}

void printHelp(const char *name) {
  printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
         "Options:\n"
         "  -h         Print this help message.\n"
         "  -v         Optional verbose flag.\n"
         "  -s <num>   Number of set index bits.\n"
         "  -E <num>   Number of lines per set.\n"
         "  -b <num>   Number of block offset bits.\n"
         "  -t <file>  Trace file.\n\n"
         "Examples:\n"
         "  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
         "  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",
         name, name, name);
}

// 将无符号长整型的高位切片（固定宽度）转为二进制字符串
static void to_bin_width(unsigned long value, int width, char *out,
                         size_t out_size) {
  if (out_size == 0)
    return;
  if (width <= 0) {
    out[0] = '\0';
    return;
  }
  // 防御: 确保缓冲区足够
  if ((size_t)(width + 1) > out_size) {
    // 只输出能够容纳的最高位部分
    int start = width - ((int)out_size - 1);
    if (start < 0)
      start = 0;
    int trimmed = width - start;
    for (int i = 0; i < trimmed; ++i) {
      int bit_pos = width - 1 - (start + i);
      out[i] = ((value >> bit_pos) & 1UL) ? '1' : '0';
    }
    out[trimmed] = '\0';
    return;
  }
  for (int i = 0; i < width; ++i) {
    int bit_pos = width - 1 - i;
    out[i] = ((value >> bit_pos) & 1UL) ? '1' : '0';
  }
  out[width] = '\0';
}

int hits, misses, evictions; // 统计：命中/不命中/替换 次数
int main(int argc, char *argv[]) {
  // 读入命令行，获得构建缓存模拟器所需的所有参数(AI辅助完成)
  SimConfig config = load_data(argc, argv);
  if (config.help) { // 仅显示帮助
    printHelp(argv[0]);
    return 0;
  }
  if (config.verbose) {
    printf("解析结果: s=%d E=%d b=%d trace=%s verbose=1\n", config.s, config.E,
           config.b, config.trace_file);
  }

  // 读入trace每一行地址参数，忽略L/S、访问大小、寄存器编号
  FILE *fp = fopen(config.trace_file, "r");
  if (!fp) {
    perror("fopen trace error!");
    return ERROR;
  }
  char line[128];
  unsigned long addr;
  char op;
  Cache cache = (Cache){0};
  int tick = 0; // 全局递增时间戳，用于 LRU
  while (fgets(line, sizeof line, fp)) {
    if (line[0] == ' ' || line[0] == 'L' || line[0] == 'S' || line[0] == 'M') {
      // 解析：op 和地址，CS:APP 的 trace 逗号后只有一个数字（大小）
      if (sscanf(line, " %c %lx,%*d", &op, &addr) == 2) {
        if (op == 'I') {
          continue; // 指令取指不计入数据缓存
        }
        // 位处理：忽略低 b 位，接着取 s 位为组索引，剩余高位为 tag
        // 的二进制字符串
        unsigned long index_mask =
            (config.s == 0) ? 0UL : ((1UL << config.s) - 1UL); // 低 s 位掩码
        int data_s = (int)((addr >> config.b) & index_mask);

        // 高位 tag 数值与位宽（地址总位 - s - b）
        unsigned long tag_val = addr >> (config.s + config.b);
        int tag_bits = 64 - (config.s + config.b);
        // 保存二进制样式的 tag
        char data_tag[65];
        to_bin_width(tag_val, tag_bits, data_tag, sizeof(data_tag));

        // //调试使用
        // if (config.verbose) {
        //   printf("[line] op=%c addr=0x%lx set=%d tag_bits=%d tag=%s\n",
        //          op, addr, data_s, tag_bits, data_tag);
        // }

        // 模拟缓存（LRU 策略：time 越大越新）
        int find_data = 0;  // 是否命中标志
        int capacity = 0;   // 当前组已占用的行数
        int LRU_find = -1;  // 记录最“旧”的行下标（待淘汰）
        int find_empty = 0; // 是否发现空行
        int empty_row = -1; // 记录空行下标
        int min_time = INT_MAX; // 该组中最小的时间戳（LRU 选择依据）
        for (int i = 0; i < config.E; i++) {
          // 找到空行则记录
          if (!find_empty && !cache.set[data_s].E_all[i].effect) {
            find_empty = 1;
            empty_row = i;
          }
          // 判断是否已满（已满则需要进行替换）
          if (cache.set[data_s].E_all[i].effect) {
            capacity += 1;
            if (cache.set[data_s].E_all[i].time < min_time) {
              min_time = cache.set[data_s].E_all[i].time;
              LRU_find = i;
            }
          }
          // 判断是否命中
          if (cache.set[data_s].E_all[i].effect &&
              (strcmp(cache.set[data_s].E_all[i].tag, data_tag) == 0)) {
            hits += 1;
            find_data = 1;
            // 命中：更新时间戳（全局自增，保证严格单调）
            cache.set[data_s].E_all[i].time = ++tick;
            break;
          }
        }
        // 没有找到——空行补充/替换
        if (!find_data) {
          // 替换
          if (capacity == config.E) {
            misses += 1;
            evictions += 1;
            if (LRU_find < 0)
              LRU_find = 0; // 兜底
            cache.set[data_s].E_all[LRU_find].effect = 1;
            strcpy(cache.set[data_s].E_all[LRU_find].tag, data_tag);
            cache.set[data_s].E_all[LRU_find].time = ++tick; // 新载入视为“最新”
          }
          // 空行补充
          else {
            misses += 1;
            cache.set[data_s].E_all[empty_row].effect = 1;
            cache.set[data_s].E_all[empty_row].time =
                ++tick; // 填入空行，标记为“最新”
            strcpy(cache.set[data_s].E_all[empty_row].tag, data_tag);
          }
        }
      }
    }
  }
  fclose(fp);

  // 打印结果
  printSummary(hits, misses, evictions);
  return 0;
}