#define _GNU_SOURCE
#include <errno.h>
#include <gnu/libc-version.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* --- timer --- */
static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* --- reusable barrier (sense-reversing) --- */
typedef struct {
  pthread_mutex_t mu;
  pthread_cond_t cv;
  int participants;
  int arrived;
  int phase;
} barrier_t;

static void barrier_init(barrier_t *b, int n) {
  pthread_mutex_init(&b->mu, NULL);
  pthread_cond_init(&b->cv, NULL);
  b->participants = n;
  b->arrived = 0;
  b->phase = 0;
}

static void barrier_wait(barrier_t *b) {
  pthread_mutex_lock(&b->mu);
  int ph = b->phase;
  b->arrived++;
  if (b->arrived == b->participants) {
    b->arrived = 0;
    b->phase++;
    pthread_cond_broadcast(&b->cv);
  } else {
    while (ph == b->phase) pthread_cond_wait(&b->cv, &b->mu);
  }
  pthread_mutex_unlock(&b->mu);
}

/* --- mode enum --- */
typedef enum {
  MODE_SMALL = 1,
  MODE_SWEEP = 2,
  MODE_BURST = 3,
  MODE_MIXED = 4,
  MODE_THREAD_SPAWN = 5,
  MODE_LARGE_TOUCH = 6,
} bench_mode_t;

/* --- per-worker parameters --- */
typedef struct {
  bench_mode_t mode;
  uint64_t iters;
  size_t size;
  size_t burst;
  const size_t *mixed_sizes;
  size_t mixed_count;
  unsigned int seed;
  barrier_t *barrier;
} worker_args_t;

/* --- page touch helper --- */
static void touch_pages(uint8_t *p, size_t n, size_t stride) {
  volatile uint8_t sink = 0;
  for (size_t i = 0; i < n; i += stride) {
    p[i] = (uint8_t)(p[i] + 1);
    sink ^= p[i];
  }
  (void)sink;
}

/* --- simple xorshift prng (inline in worker) --- */
static inline unsigned int xorshift32(unsigned int *s) {
  unsigned int x = *s;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *s = x;
  return x;
}

/* --- worker --- */
static void *worker_main(void *arg) {
  const worker_args_t *a = (const worker_args_t *)arg;
  barrier_wait(a->barrier);
  barrier_wait(a->barrier);

  switch (a->mode) {
  case MODE_SMALL:
    for (uint64_t i = 0; i < a->iters; i++) {
      void *p = malloc(a->size);
      if (!p) abort();
      ((volatile uint8_t *)p)[0] = (uint8_t)i;
      free(p);
    }
    break;

  case MODE_SWEEP:
    /* a->iters per size; sizes[] is passed via iters/size as dummy
       but we need a cleaner path — handled in run_sweep() below */
    break;

  case MODE_BURST: {
    void **ptrs = (void **)calloc(a->burst, sizeof(void *));
    if (!ptrs) abort();
    for (uint64_t r = 0; r < a->iters; r++) {
      for (size_t i = 0; i < a->burst; i++) {
        ptrs[i] = malloc(a->size);
        if (!ptrs[i]) abort();
        ((volatile uint8_t *)ptrs[i])[0] = (uint8_t)(r + i);
      }
      for (size_t i = a->burst; i-- > 0;) free(ptrs[i]);
    }
    free(ptrs);
    break;
  }

  case MODE_MIXED: {
    unsigned int state = a->seed;
    for (uint64_t i = 0; i < a->iters; i++) {
      size_t sz = a->mixed_sizes[xorshift32(&state) % a->mixed_count];
      void *p = malloc(sz);
      if (!p) abort();
      ((volatile uint8_t *)p)[0] = (uint8_t)i;
      free(p);
    }
    break;
  }

  case MODE_THREAD_SPAWN:
    /* main thread drives this; worker unused */
    break;

  case MODE_LARGE_TOUCH:
    for (uint64_t i = 0; i < a->iters; i++) {
      uint8_t *p = (uint8_t *)malloc(a->size);
      if (!p) abort();
      memset(p, 0, a->size);
      touch_pages(p, a->size, 4096);
      free(p);
    }
    break;

  default:
    abort();
  }
  return NULL;
}

/* --- run helpers --- */

static void run_threaded(bench_mode_t mode, const char *name, int nthreads,
                         uint64_t iters, size_t size, size_t burst,
                         const size_t *mixed_sizes, size_t mixed_count) {
  barrier_t b;
  barrier_init(&b, nthreads + 1);
  pthread_t *tids = calloc((size_t)nthreads, sizeof(pthread_t));
  worker_args_t *args = calloc((size_t)nthreads, sizeof(worker_args_t));
  if (!tids || !args) { perror("calloc"); exit(2); }

  for (int i = 0; i < nthreads; i++) {
    args[i] = (worker_args_t){.mode = mode, .iters = iters, .size = size,
                              .burst = burst, .mixed_sizes = mixed_sizes,
                              .mixed_count = mixed_count,
                              .seed = (unsigned int)(now_ns() + (uint64_t)i),
                              .barrier = &b};
    pthread_create(&tids[i], NULL, worker_main, &args[i]);
  }

  barrier_wait(&b);
  uint64_t t0 = now_ns();
  barrier_wait(&b);
  for (int i = 0; i < nthreads; i++) pthread_join(tids[i], NULL);
  uint64_t t1 = now_ns();

  uint64_t total_ops;
  if (mode == MODE_BURST)
    total_ops = (uint64_t)nthreads * iters * burst;
  else
    total_ops = (uint64_t)nthreads * iters;

  double ns_total = (double)(t1 - t0);
  double ns_per_op = ns_total / (double)total_ops;

  printf("mode=%-14s threads=%-2d iters=%-10" PRIu64 " size=%-8zu ns_total=%-16.0f ns_per_op=%.2f\n",
         name, nthreads, iters, size, ns_total, ns_per_op);
  free(tids);
  free(args);
}

static void run_sweep(int threads, uint64_t iters) {
  static const size_t sizes[] = {16, 32, 64, 128, 160, 256, 512, 1024};
  static const size_t nsizes = sizeof(sizes) / sizeof(sizes[0]);
  for (size_t si = 0; si < nsizes; si++) {
    char label[32];
    snprintf(label, sizeof(label), "sweep_%zu", sizes[si]);
    run_threaded(MODE_SMALL, label, threads, iters, sizes[si], 0, NULL, 0);
  }
}

static void *spawn_worker(void *arg) {
  const worker_args_t *a = (const worker_args_t *)arg;
  for (uint64_t i = 0; i < a->iters; i++) {
    void *p = malloc(a->size);
    if (!p) abort();
    ((volatile uint8_t *)p)[0] = (uint8_t)i;
    free(p);
  }
  return NULL;
}

static void run_thread_spawn_v2(int total_threads, uint64_t mallocs_per_thread, size_t size) {
  uint64_t t0 = now_ns();
  for (int i = 0; i < total_threads; i++) {
    pthread_t tid;
    worker_args_t arg = {.mode = MODE_SMALL, .iters = mallocs_per_thread,
                         .size = size, .burst = 0, .barrier = NULL};
    pthread_create(&tid, NULL, spawn_worker, &arg);
    pthread_join(tid, NULL);
  }
  uint64_t t1 = now_ns();

  uint64_t total_ops = (uint64_t)total_threads * mallocs_per_thread;
  double ns_total = (double)(t1 - t0);
  double ns_per_op = ns_total / (double)total_ops;

  printf("mode=%-14s spawns=%-2d iters=%-10" PRIu64 " size=%-8zu ns_total=%-16.0f ns_per_op=%.2f\n",
         "thread_spawn", total_threads, mallocs_per_thread, size, ns_total, ns_per_op);
}

/* --- argument parsing --- */
static bool streq(const char *a, const char *b) { return strcmp(a, b) == 0; }

static int parse_int(const char *s, int *out) {
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0' || v <= 0 || v > 16384) return -1;
  *out = (int)v;
  return 0;
}

static int parse_u64(const char *s, uint64_t *out) {
  char *end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0' || v == 0) return -1;
  *out = (uint64_t)v;
  return 0;
}

static int parse_size(const char *s, size_t *out) {
  char *end = NULL;
  unsigned long long base = strtoull(s, &end, 10);
  if (errno != 0 || end == s) return -1;
  unsigned long long mul = 1;
  if (*end != '\0') {
    if (end[1] != '\0') return -1;
    switch (*end) {
    case 'K': case 'k': mul = 1024ull; break;
    case 'M': case 'm': mul = 1024ull * 1024ull; break;
    case 'G': case 'g': mul = 1024ull * 1024ull * 1024ull; break;
    default: return -1;
    }
  }
  unsigned long long v = base * mul;
  if (v == 0 || v > (unsigned long long)SIZE_MAX) return -1;
  *out = (size_t)v;
  return 0;
}

static void usage(const char *argv0) {
  fprintf(stderr,
    "Usage: %s [--mode small|sweep|burst|mixed|thread_spawn|large_touch|all]\n"
    "          [--threads N] [--iters N] [--size BYTES] [--burst N]\n", argv0);
}

/* --- main --- */
int main(int argc, char **argv) {
  const char *mode = "all";
  int threads = 1;
  uint64_t iters = 1000000;
  size_t size = 64;
  size_t burst = 1024;

  for (int i = 1; i < argc; i++) {
    if (streq(argv[i], "--help") || streq(argv[i], "-h")) { usage(argv[0]); return 0; }
    if (streq(argv[i], "--mode") && i + 1 < argc) { mode = argv[++i]; continue; }
    if (streq(argv[i], "--threads") && i + 1 < argc) {
      if (parse_int(argv[++i], &threads) != 0) { fprintf(stderr, "Invalid --threads\n"); return 2; }
      continue;
    }
    if (streq(argv[i], "--iters") && i + 1 < argc) {
      if (parse_u64(argv[++i], &iters) != 0) { fprintf(stderr, "Invalid --iters\n"); return 2; }
      continue;
    }
    if (streq(argv[i], "--size") && i + 1 < argc) {
      if (parse_size(argv[++i], &size) != 0) { fprintf(stderr, "Invalid --size\n"); return 2; }
      continue;
    }
    if (streq(argv[i], "--burst") && i + 1 < argc) {
      size_t v;
      if (parse_size(argv[++i], &v) != 0 || v == 0 || v > (1u << 20)) {
        fprintf(stderr, "Invalid --burst\n"); return 2;
      }
      burst = v;
      continue;
    }
    fprintf(stderr, "Unknown arg: %s\n", argv[i]); usage(argv[0]); return 2;
  }

  printf("glibc=%s\n", gnu_get_libc_version());

  /* --- all-mode defaults (balanced for cross-version comparison) --- */

  /* small: single-size hot-path (tcache showcase) */
  if (streq(mode, "small") || streq(mode, "all")) {
    uint64_t n = (streq(mode, "all") && iters == 1000000) ? 2000000 : iters;
    run_threaded(MODE_SMALL, "small", threads, n, size, burst, NULL, 0);
  }

  /* sweep: size-latency curve at 8 sizes */
  if (streq(mode, "sweep") || streq(mode, "all")) {
    uint64_t n = (streq(mode, "all") && iters == 1000000) ? 2000000 : iters;
    run_sweep(threads, n);
  }

  /* burst: batch alloc/free */
  if (streq(mode, "burst") || streq(mode, "all")) {
    uint64_t rounds = iters;
    if (streq(mode, "all") && iters == 1000000) rounds = 20000;
    run_threaded(MODE_BURST, "burst", threads, rounds, size, burst, NULL, 0);
  }

  /* mixed: random interleaved sizes */
  if (streq(mode, "mixed") || streq(mode, "all")) {
    static const size_t mix_sz[] = {16, 32, 64, 128, 256};
    uint64_t n = (streq(mode, "all") && iters == 1000000) ? 2000000 : iters;
    run_threaded(MODE_MIXED, "mixed", threads, n, 0, 0, mix_sz, 5);
  }

  /* thread_spawn: create/destroy short-lived threads */
  if (streq(mode, "thread_spawn") || streq(mode, "all")) {
    int nspawns = 2000;
    uint64_t per_thread = 1000;
    run_thread_spawn_v2(nspawns, per_thread, 64);
  }

  /* large_touch: large allocation + page touching */
  if (streq(mode, "large_touch") || streq(mode, "all")) {
    uint64_t rounds = iters;
    size_t bytes = size;
    if (streq(mode, "all") && iters == 1000000 && size == 64) {
      rounds = 20;
      bytes = 64 * 1024 * 1024;
    }
    run_threaded(MODE_LARGE_TOUCH, "large_touch", 1, rounds, bytes, 0, NULL, 0);
  }

  return 0;
}
