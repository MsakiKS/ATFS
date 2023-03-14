#define _GNU_SOURCE

#ifndef _ATFS_STATS_H
#define _ATFS_STATS_H

#include <sched.h>

enum time_category {
  nvmemcpy_read_redo_t,
  alloc_log_t,
  nvmemcpy_write_t,
  fsync_t,
  check_log_t,
  nvmmio_memcpy_t,
  get_fd_uma_t,
  increase_uma_write_cnt_t,
  increase_uma_read_cnt_t,
  nvmmio_fence_t,
  nvmmio_write_t,
  nvmmio_flush_t,
  indexing_log_t,
  alloc_log_entry_t,
  logging_t,
  test_t,
  NR_FUNCS,
};

struct timespec **timestats_percpu;
unsigned int **countstats_percpu;

#define handle_error(msg) \
  do {                    \
    perror(msg);          \
    exit(EXIT_FAILURE);   \
  } while (0)

#define handle_error_en(en, msg) \
  do {                           \
    errno = en;                  \
    perror(msg);                 \
    exit(EXIT_FAILURE);          \
  } while (0)

#ifdef _ATFS_DEBUG
#define ATFS_DEBUG(fmt, ...)                                    \
  do {                                                          \
    fprintf(stderr, "[%s] " fmt "\n", __func__, ##__VA_ARGS__); \
  } while (0)
#else
#define ATFS_DEBUG(fmt, ...) \
  {}
#endif

#ifdef _ATFS_LOG
#define ATFS_LOG(fmt, ...)                                      \
  do {                                                          \
    fprintf(stdout, "[%s] " fmt "\n", __func__, ##__VA_ARGS__); \
  } while (0)
#else
#define ATFS_LOG(fmt, ...) \
  {}
#endif

#ifdef _ATFS_TIME
#define ATFS_INIT_TIMER() init_timer()

#define ATFS_INIT_TIME(x) struct timespec x = {0}

#define ATFS_START_TIME(name, start)                   \
  {                                                    \
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) { \
      handle_error("clock gettime");                   \
    }                                                  \
  }

#define ATFS_END_TIME(name, start)                   \
  {                                                  \
    ATFS_INIT_TIME(end);                             \
    if (clock_gettime(CLOCK_REALTIME, &end) == -1) { \
      handle_error("clock gettime");                 \
    }                                                \
    long sec = end.tv_sec - start.tv_sec;            \
    long nsec = end.tv_nsec - start.tv_nsec;         \
    if (start.tv_nsec > end.tv_nsec) {               \
      --sec;                                         \
      nsec += 1000000000;                            \
    }                                                \
    int cpu = sched_getcpu();                        \
    timestats_percpu[name][cpu].tv_sec += sec;       \
    timestats_percpu[name][cpu].tv_nsec += nsec;     \
    countstats_percpu[name][cpu] += 1;               \
  }

#define ATFS_REPORT_TIME() report_time()
#else
#define ATFS_INIT_TIMER() \
  {}
#define ATFS_INIT_TIME(x) \
  {}
#define ATFS_START_TIME(name, start) \
  {}
#define ATFS_END_TIME(name, start) \
  {}
#define ATFS_REPORT_TIME() \
  {}
#endif

void init_timer(void);
void report_time(void);

#endif /* _ATFS_STATS_H */
