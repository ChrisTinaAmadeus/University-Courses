#include <stdio.h>
#include <windows.h>
#include <cmath>
#include <cfloat>
#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;

// 界面美化：颜色定义
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLD "\033[1m"

#define OK 1
#define ERROR 0
typedef int Status;
int hashsize[] = {50021, 100003, 200003, 400009, 800011, 1600003, 3200003}; // 哈希表容量递增表
int s = 0;                                                                  // 当前哈希表容量索引
long long total_freq;                                                       // 频数总和
// 哈希表
typedef struct DictNode
{
    char *word;            // 词
    int frequency;         // 词频
    char *pos;             // 词性
    struct DictNode *next; // 链表指针，用于解决哈希冲突
} DictNode;
typedef struct
{
    DictNode **buckets; // 指针数组，每个指针指向一个链表的头结点
    int size;           // 哈希表大小
    int count;          // 当前存储的词条数
} HashTable;
// 哈希函数 (BKDR Hash)
unsigned int BKDRHash(const char *str)
{
    unsigned int seed = 131;
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}
// 初始化哈希表
Status InitHashTable(HashTable &HT)
{
    HT.size = hashsize[s++];
    HT.count = 0;
    HT.buckets = (DictNode **)malloc(sizeof(DictNode *) * HT.size);
    if (!HT.buckets)
        return ERROR;
    // 初始化所有桶为空
    memset(HT.buckets, 0, sizeof(DictNode *) * HT.size);
    return OK;
}
// 扩容哈希表
Status ResizeHashTable(HashTable &HT)
{
    if (s > 6)
    {
        printf(RED "哈希表已达到最大容量，无法扩容！\n" RESET);
        return ERROR;
    }
    int newSize = hashsize[s++];
    DictNode **newBuckets = (DictNode **)malloc(sizeof(DictNode *) * newSize);
    if (!newBuckets)
        return ERROR;
    memset(newBuckets, 0, sizeof(DictNode *) * newSize);
    for (int i = 0; i < HT.size; i++) // 重新哈希所有现有结点
    {
        DictNode *p = HT.buckets[i];
        while (p)
        {
            DictNode *next = p->next; // 保存链表下一个结点
            // 在新表中重新计算位置
            unsigned int newHash = BKDRHash(p->word) % newSize;
            // 头插法搬运结点
            p->next = newBuckets[newHash];
            newBuckets[newHash] = p;
            p = next;
        }
    }
    free(HT.buckets); // 释放旧桶数组
    HT.buckets = newBuckets;
    HT.size = newSize;
    return OK;
}
// 插入词条到哈希表
Status InsertDict(HashTable &HT, const char *word, int freq, const char *pos)
{
    unsigned int hashVal = BKDRHash(word) % HT.size;
    // 检查是否已存在
    DictNode *p = HT.buckets[hashVal];
    while (p)
    {
        if (strcmp(p->word, word) == 0)
        {
            return OK;
        }
        p = p->next;
    }
    // 当前容量如果超过 0.95 则扩容
    if (HT.count >= HT.size * 0.95)
    {
        if (ResizeHashTable(HT) == OK)
        {
            // 扩容后重新计算哈希值
            hashVal = BKDRHash(word) % HT.size;
        }
    }
    // 创建新结点
    DictNode *newNode = (DictNode *)malloc(sizeof(DictNode));
    if (!newNode)
        return ERROR;
    // 分配内存并复制字符串
    newNode->word = (char *)malloc(strlen(word) + 1);
    if (!newNode->word)
    {
        free(newNode);
        return ERROR;
    }
    strcpy(newNode->word, word);
    newNode->frequency = freq;
    newNode->pos = (char *)malloc(strlen(pos) + 1);
    if (newNode->pos)
        strcpy(newNode->pos, pos);
    // 头插法插入
    newNode->next = HT.buckets[hashVal];
    HT.buckets[hashVal] = newNode;
    HT.count++;
    return OK;
}
// 从文件构建哈希表
Status BuildHashTable(HashTable &HT, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf(RED "无法打开字典文件！" RESET);
        return ERROR;
    }
    char line[128];
    char word[128];
    int freq;
    char pos[128];
    // 逐行读取
    while (fgets(line, sizeof(line), fp))
    {
        // 去除行末换行符
        line[strcspn(line, "\n")] = 0;
        // 解析每一行：词 词频 词性
        int parsed = sscanf(line, "%s %d %s", word, &freq, pos);
        InsertDict(HT, word, freq, pos);
        total_freq += freq;
    }
    fclose(fp);
    return OK;
}

// 图
struct Edge
{
    int to;            // 指向的节点索引
    double weight;     // 权重
    char word[128];    // 词的内容
    char pos[64];      // 词性
    struct Edge *next; // 链表指针，用于邻接表
};
// 根据词性计算权重系数
double GetPosMultiplier(const char *pos)
{
    // 1. 习用语(i)、缩略语(j)、成语(l) - 最高优先级
    if (strstr(pos, "i") || strstr(pos, "j") || strstr(pos, "l"))
        return 0.1;
    // 2. 专有名词 (nr人名, ns地名, nt机构名, nz其他专名) - 高优先级
    if (strstr(pos, "nr") || strstr(pos, "ns") || strstr(pos, "nt") || strstr(pos, "nz"))
        return 0.4;
    // 3. 副词(d) - 较高优先级
    if (pos[0] == 'd')
        return 0.6;
    // 4. 动词(v)、名词(n)、形容词(a) - 保持完整性，正常优先级
    if (pos[0] == 'n' || pos[0] == 'v' || pos[0] == 'a')
        return 0.8;
    // 5. 虚词 (u助词, p介词, c连词, y语气词) - 施加惩罚
    if (pos[0] == 'u' || pos[0] == 'p' || pos[0] == 'c' || pos[0] == 'y')
        return 1.2;
    // 6. 其他 - 正常处理
    return 1.0;
}
// 初始化有向无环图
Status InitGraph(Edge **&graph, int sentence_length)
{
    graph = (Edge **)malloc(sizeof(Edge *) * (sentence_length + 1));
    if (!graph)
        return ERROR;
    for (int i = 0; i <= sentence_length; i++)
    {
        graph[i] = nullptr;
    }
    return OK;
}
// 构建有向无环图
Status BuildGraph(Edge **&graph, const char *sentence, HashTable &dict)
{
    int len = strlen(sentence);
    for (int i = 0; i < len; i++)
    {
        for (int j = i + 1; j <= len; j++)
        {
            char substr[256] = {};
            // 防止越界
            if (j - i >= 256)
                break;
            strncpy(substr, sentence + i, j - i);
            substr[j - i] = '\0';
            unsigned int hashVal = BKDRHash(substr) % dict.size;
            DictNode *p = dict.buckets[hashVal];
            while (p)
            {
                if (strcmp(p->word, substr) == 0) // 找到匹配词，添加边
                {
                    Edge *newEdge = (Edge *)malloc(sizeof(Edge));
                    newEdge->to = j;
                    // 计算基础权重：-ln(P)
                    double baseWeight = -log((double)p->frequency / total_freq);
                    // 应用词性系数
                    double posMultiplier = GetPosMultiplier(p->pos);
                    newEdge->weight = baseWeight * posMultiplier;
                    strcpy(newEdge->word, p->word);
                    strcpy(newEdge->pos, p->pos);
                    newEdge->next = graph[i];
                    graph[i] = newEdge;
                    break;
                }
                p = p->next;
            }
        }
    }
    return OK;
}

// N-最短路径
Status N_minpath(Edge **graph, char *sentence, int N = 1)
{
    int len = strlen(sentence);
    // D[i][k] 存储到达节点 i 的第 k 短路径长度
    double **D = (double **)malloc(sizeof(double *) * (len + 1));
    // P[i][k] 存储到达节点 i 的第 k 短路径的前驱信息 {prev_node, prev_rank}
    int ***P = (int ***)malloc(sizeof(int **) * (len + 1));
    // path_counts[i] 存储到达节点 i 的有效路径数量
    int *path_counts = (int *)malloc(sizeof(int) * (len + 1));
    for (int i = 0; i <= len; i++)
    {
        D[i] = (double *)malloc(sizeof(double) * N);
        P[i] = (int **)malloc(sizeof(int *) * N);
        path_counts[i] = 0;
        for (int k = 0; k < N; k++)
        {
            D[i][k] = DBL_MAX;
            P[i][k] = (int *)malloc(sizeof(int) * 2);
            P[i][k][0] = -1;
            P[i][k][1] = -1;
        }
    }
    // 起点
    D[0][0] = 0.0;
    path_counts[0] = 1;
    // 按照拓扑顺序处理
    for (int i = 0; i <= len; i++)
    {
        if (path_counts[i] == 0)
            continue;
        // 遍历当前节点的所有有效路径
        for (int k = 0; k < path_counts[i]; k++)
        {
            double curr_cost = D[i][k];
            // 扩展到邻居
            Edge *edge = graph[i];
            while (edge)
            {
                int to = edge->to;
                double new_cost = curr_cost + edge->weight;
                // 尝试将 new_cost 插入到 D[to] 中
                // 找到插入位置
                int insert_pos = -1;
                int current_count = path_counts[to];
                for (int m = 0; m < current_count; m++)
                {
                    if (new_cost < D[to][m])
                    {
                        insert_pos = m;
                        break;
                    }
                }
                if (insert_pos == -1)
                {
                    if (current_count < N)
                    {
                        insert_pos = current_count;
                    }
                    else
                    {
                        // 比现有的 N 条路径都长，直接忽略
                        edge = edge->next;
                        continue;
                    }
                }
                // 移动元素腾出空间
                int limit = (current_count < N) ? current_count : N - 1;
                for (int m = limit; m > insert_pos; m--)
                {
                    D[to][m] = D[to][m - 1];
                    P[to][m][0] = P[to][m - 1][0];
                    P[to][m][1] = P[to][m - 1][1];
                }
                // 插入新路径
                D[to][insert_pos] = new_cost;
                P[to][insert_pos][0] = i;
                P[to][insert_pos][1] = k;

                if (current_count < N)
                {
                    path_counts[to]++;
                }

                edge = edge->next;
            }
        }
    }

    // 输出分词结果
    int found_paths = path_counts[len];
    if (found_paths == 0)
    {
        printf(RED "分词失败，未找到有效路径！\n" RESET);
        // 释放内存
        free(path_counts);
        for (int i = 0; i <= len; i++)
        {
            for (int k = 0; k < N; k++)
            {
                free(P[i][k]);
            }
            free(P[i]);
            free(D[i]);
        }
        free(P);
        free(D);
        return ERROR;
    }

    for (int k = 0; k < found_paths; k++)
    {
        // printf("路径 %d (权重 %.4f): ", k + 1, D[len][k]);
        printf(CYAN "%d : " RESET, k + 1);
        // 回溯路径
        int index = len;
        int rank = k;
        int min_path[1024];
        int count = 0;
        min_path[count++] = len;
        bool path_valid = true;
        while (index > 0)
        {
            int prev = P[index][rank][0];
            int prev_rank = P[index][rank][1];
            if (prev == -1)
            {
                path_valid = false;
                break;
            }
            min_path[count++] = prev;
            index = prev;
            rank = prev_rank;
        }

        if (!path_valid)
        {
            printf(RED "路径无效！\n" RESET);
            continue;
        }

        for (int i = count - 1; i > 0; i--)
        {
            int start = min_path[i];
            int end = min_path[i - 1];
            int word_len = end - start;
            printf(YELLOW "%.*s" RESET " / ", word_len, sentence + start);
        }
        printf("\n");
    }

    // 释放内存
    free(path_counts);
    for (int i = 0; i <= len; i++)
    {
        for (int k = 0; k < N; k++)
        {
            free(P[i][k]);
        }
        free(P[i]);
        free(D[i]);
    }
    free(P);
    free(D);
    return OK;
}

// 读入字典，建立哈希表 ✅️
// 根据输入的句子构建有向无环图，设置好权重 ✅️
// 采用Dijkstra算法求解最短路径 ✅️，再扩展为求N-最短路径 ✅️
// 输出分词结果 ✅️
// 前端UI界面 ✅️

int main()
{
    // 设置控制台为UTF-8编码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HashTable dict;
    // 初始化哈希表
    if (InitHashTable(dict) != OK)
    {
        printf(RED "哈希表初始化失败！\n" RESET);
        return ERROR;
    }
    // 加载词典
    const char *dict_file = "dict_big.txt";
    if (BuildHashTable(dict, dict_file) != OK)
    {
        printf(RED "构建哈希表失败！\n" RESET);
        return ERROR;
    }
    int choice = 1;
    char input_sentence[4096] = {};
    Edge **graph = nullptr;
    while (choice != 2)
    {
        system("cls"); // 清屏
        cout << CYAN << "╔══════════════════════════════════════════════════════════╗" << RESET << endl;
        cout << CYAN << "║" << BOLD << MAGENTA << "               中文分词系统 (N-最短路径)                  " << RESET << CYAN << "║" << RESET << endl;
        cout << CYAN << "╠══════════════════════════════════════════════════════════╣" << RESET << endl;
        cout << CYAN << "║" << RESET << "                                                          " << CYAN << "║" << RESET << endl;
        cout << CYAN << "║" << RESET << "    [1] " << GREEN << "开始分词" << RESET << "                                          " << CYAN << "║" << RESET << endl;
        cout << CYAN << "║" << RESET << "    [2] " << RED << "退出系统" << RESET << "                                          " << CYAN << "║" << RESET << endl;
        cout << CYAN << "║" << RESET << "                                                          " << CYAN << "║" << RESET << endl;
        cout << CYAN << "╚══════════════════════════════════════════════════════════╝" << RESET << endl;
        cout << endl;
        cout << BOLD << "请输入选项编号 > " << RESET;
        cin >> choice;
        cin.ignore(); // 清除输入缓冲区的换行符
        if (choice == 2)
            break;
        if (choice != 1)
        {
            cout << RED << "无效选项，请重新输入！" << RESET << endl;
            Sleep(1000);
            continue;
        }
        cout << BOLD << "请输入需要分词的句子 > " << RESET;
        cin.getline(input_sentence, 4096);
        // 构建图
        if (InitGraph(graph, strlen(input_sentence)) != OK)
        {
            printf(RED "图初始化失败！\n" RESET);
            return ERROR;
        }
        if (BuildGraph(graph, input_sentence, dict) != OK)
        {
            printf(RED "图构建失败！\n" RESET);
            return ERROR;
        }
        cout << BOLD << "请输入N-最短路径中的N值 > " << RESET;
        int N;
        cin >> N;
        cin.ignore(); // 清除输入缓冲区的换行符

        cout << endl;
        cout << CYAN << "------------------ 分词结果 ------------------" << RESET << endl;
        // 最短路径分词
        N_minpath(graph, input_sentence, N);
        cout << CYAN << "----------------------------------------------" << RESET << endl;

        // 用户按回车返回主菜单
        cout << endl
             << GREEN << "按回车键返回主菜单..." << RESET;
        cin.get();
    }
    cout << YELLOW << "清除数据ing..." << RESET << endl;
    // 释放图
    if (graph != nullptr)
    {
        for (int i = 0; i <= strlen(input_sentence); i++)
        {
            Edge *p = graph[i];
            while (p)
            {
                Edge *temp = p;
                p = p->next;
                free(temp);
            }
        }
        free(graph);
    }
    // 释放哈希表
    for (int i = 0; i < dict.size; i++)
    {
        DictNode *p = dict.buckets[i];
        while (p)
        {
            DictNode *temp = p;
            p = p->next;
            free(temp->word);
            free(temp->pos);
            free(temp);
        }
    }
    free(dict.buckets);
    return 0;
}