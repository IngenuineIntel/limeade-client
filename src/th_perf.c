// th_perf.c
// FIXME my interface makes no sense!!!

#include<errno.h>
#include<inttypes.h>
#include<signal.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"

#define MEM_TOTAL_TARGET     "MemTotal:"
#define MEM_FREE_TARGET      "MemFree:"
#define MEM_AVAILABLE_TARGET "MemAvailable:"
#define MEM_CACHED_TARGET    "Cached:"
#define ITERATE_BACKWARDS(ptr, target)\
  for(; *ptr != target; ptr--)

static int populate_meminfo(const char * const in, const int in_sz, struct limeade_perf * const out)
{
  /* populates mem_*_kb fields of `out` with data from /proc/meminfo
   * 0 on success, -1 on error */

  char *total_kb_idx, *free_kb_idx, *available_kb_idx, *cached_kb_idx, *endptr;
  char *total_ln_end, *free_ln_end, *available_ln_end, *cached_ln_end, *max_idx;
  char *total_kb_str, *free_kb_str, *available_kb_str, *cached_kb_str;

  // while this looks relatively unproductive (and it very well could be),
  // memem has the advantage of 32-byte SIMD operations, which might (I'm not
  // sure yet) be faster than splitting lines with memchr & checking the
  // keys
  total_kb_idx     = memmem(in, in_sz, MEM_TOTAL_TARGET,     sizeof(MEM_TOTAL_TARGET)     - 1);
  free_kb_idx      = memmem(in, in_sz, MEM_FREE_TARGET,      sizeof(MEM_FREE_TARGET)      - 1);
  available_kb_idx = memmem(in, in_sz, MEM_AVAILABLE_TARGET, sizeof(MEM_AVAILABLE_TARGET) - 1);
  cached_kb_idx    = memmem(in, in_sz, MEM_CACHED_TARGET,    sizeof(MEM_CACHED_TARGET)    - 1);
  
  max_idx = (void*)in + in_sz;

  if(!total_kb_idx || !free_kb_idx || !available_kb_idx || !cached_kb_idx)
    return -1;

  total_ln_end     = memchr(total_kb_idx,     '\n', max_idx - total_kb_idx);
  free_ln_end      = memchr(free_kb_idx,      '\n', max_idx - free_kb_idx);
  available_ln_end = memchr(available_kb_idx, '\n', max_idx - available_kb_idx);
  cached_ln_end    = memchr(cached_kb_idx,    '\n', max_idx - cached_kb_idx);

  if(!total_ln_end || !free_ln_end || !available_ln_end || !cached_ln_end)
    return -1;

  // jump backwards past " kB", then iterate back before number
  total_kb_str     = total_ln_end     - 4;
  free_kb_str      = free_ln_end      - 4;
  available_kb_str = available_ln_end - 4;
  cached_kb_str    = cached_ln_end    - 4;

  ITERATE_BACKWARDS(total_kb_str, ' ');
  ITERATE_BACKWARDS(free_kb_str, ' ');
  ITERATE_BACKWARDS(available_kb_str,  ' ');
  ITERATE_BACKWARDS(cached_kb_str, ' ');

  out->mem_total_kb = strtol(++total_kb_str, &endptr, 10);
  if(endptr == total_kb_str) return -1;

  out->mem_free_kb = strtol(++free_kb_str, &endptr, 10);
  if(endptr == free_kb_str) return -1;

  out->mem_available_kb = strtol(++available_kb_str, &endptr, 10);
  if(endptr == available_kb_str) return -1;

  out->mem_cached_kb = strtol(++cached_kb_str, &endptr, 10);
  if(endptr == cached_kb_str) return -1;

  return 0;
}

struct statinfo { uint64_t uptime, user, sys; };
static struct statinfo stat_cache_1, stat_cache_2;
static struct statinfo *stat_prev = &stat_cache_1, *stat_next = &stat_cache_2;

static char PROC_STAT_BUFFER[128];
static char PROC_MEMINFO_BUFFER[2048];

static inline void next_stat(void)
{
  struct statinfo *tmp;
  tmp = stat_prev;
  stat_prev = stat_next;
  stat_next = tmp;
}

static int populate_statinfo(const char * const in, const int in_sz)
{
  char line[256];
  uint64_t counters[10] = {0};
  int line_sz, count;

  if(!in || in_sz <= 0)
    return -1;

  const char *line_end = memchr(in, '\n', in_sz);
  line_sz = line_end ? (int)(line_end - in) : in_sz;
  if(line_sz < 4 || line_sz >= (int)sizeof(line) || memcmp(in, "cpu ", 4))
    return -1;

  memcpy(line, in, line_sz);
  line[line_sz] = '\0';
  count = sscanf(line, "cpu %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                 " %" SCNu64 " %" SCNu64,
                 &counters[0], &counters[1], &counters[2], &counters[3],
                 &counters[4], &counters[5], &counters[6], &counters[7],
                 &counters[8], &counters[9]);
  if(count < 4)
    return -1;

  stat_next->uptime = 0;
  for(int i = 0; i < count && i < 8; i++)
    stat_next->uptime += counters[i];
  stat_next->user = counters[0] + counters[1];
  stat_next->sys = counters[2] + counters[5] + counters[6];
  return 0;
}

static int populate_stat(void)
{
  int amt;
  FILE *fd = fopen("/proc/stat", "r");
  if(!fd)
    return -1;

  amt = fread(PROC_STAT_BUFFER, 1, sizeof(PROC_STAT_BUFFER) - 1, fd);
  fclose(fd);
  if(amt < 0)
    return -1;

  return populate_statinfo(PROC_STAT_BUFFER, amt);
}

static void cpu_usage_from_stat(struct limeade_perf * const out)
{
  uint64_t diff_up, diff_user, diff_sys;

  diff_up   = stat_next->uptime - stat_prev->uptime;
  diff_sys  = stat_next->sys    - stat_prev->sys;
  diff_user = stat_next->user   - stat_prev->user;

  if(!diff_up)
  {
    out->perc_cpu_user = out->perc_cpu_sys = 0;
    return;
  }

  out->perc_cpu_user = (uint8_t)((diff_user * 100) / diff_up);
  out->perc_cpu_sys  = (uint8_t)((diff_sys * 100) / diff_up);
}
static int populate_limeade_perf(struct limeade_perf * const out)
{
  int amt;
  FILE *fd;

  if(populate_stat() < 0)
  {
    log_crit("failed to populate perf data from /proc/stat");
    return -1;
  }
  cpu_usage_from_stat(out);
  next_stat();

  fd = fopen("/proc/meminfo", "r");
  if(!fd)
    return -1;

  amt = fread(PROC_MEMINFO_BUFFER, 1, sizeof(PROC_MEMINFO_BUFFER) - 1, fd);
  fclose(fd);

  if(amt < 0)
    return -1;

  PROC_MEMINFO_BUFFER[amt] = '\0';

  return populate_meminfo(PROC_MEMINFO_BUFFER, amt, out);
}

void *th_perf(struct thread_pass * const glbl)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;
  struct limeade_perf perf = {0};
  int e;

  signal(SIGUSR1, thread_recv_sig);
  goal_wait.tv_sec  = glbl->perf_ms_p / 1000;
  goal_wait.tv_nsec = glbl->perf_ms_p % 1000 * 1000000;

  if(populate_stat() < 0)
  {
    log_crit("Failed to populate initial perf data from /proc/stat");
    return NULL;
  }

  pthread_mutex_lock(&glbl->mtx_perf_err);
  log_dbg("th_perf ready");
  sem_wait(&glbl->sem_ready);

  while(sem_trywait(&glbl->sem_shutdown) != 0 && errno == EAGAIN)
  {
    clock_gettime(CLOCK_MONOTONIC, &iter_start);

    next_stat();
    if(populate_limeade_perf(&perf) < 0)
    {
      log_crit("Failed to populate Limeade PERF packet");
      goto err;
    }

    if((e = limeade_send_await(&glbl->lctx, LIMEADE_PACKET_PERF, perf)) != LIMEADE_OK)
    {
      log_crit("Failed to send Limeade PERF packet: %s", LIMEADE_ERROR_REPRS[e]);
      goto err;
    }

    clock_gettime(CLOCK_MONOTONIC, &iter_end);

    diff.tv_sec  = iter_end.tv_sec  - iter_start.tv_sec;
    diff.tv_nsec = iter_end.tv_nsec - iter_start.tv_nsec;
    real_wait.tv_sec  = goal_wait.tv_sec  - diff.tv_sec;
    real_wait.tv_nsec = goal_wait.tv_nsec - diff.tv_nsec;
    if(real_wait.tv_nsec < 0) // carry
    {
      real_wait.tv_sec--;
      real_wait.tv_nsec += 999999999;
    }
    
    if(real_wait.tv_sec < 0 || real_wait.tv_nsec < 0)
    {
      log_warn(
          "perf thread took longer than the allocated time (%lins when the goal was %lins)",
          ((int64_t)diff.tv_sec)      * 999999999 + diff.tv_nsec,
          ((int64_t)goal_wait.tv_sec) * 999999999 + goal_wait.tv_nsec);
      continue;
    }

    nanosleep(&real_wait, &diff);
  }
err:
  pthread_mutex_unlock(&glbl->mtx_perf_err);
  return NULL;
}

/*
int main(void)
{
  struct timespec wait = {1, 0};
  struct limeade_perf perf = {0};

  if(populate_stat() < 0)
  {
    perror("failed to read /proc/stat");
    return 1;
  }
  next_stat();
  if(nanosleep(&wait, NULL) < 0 ||
     populate_limeade_perf(&perf) < 0)
  {
    perror("failed to sample performance data");
    return 1;
  }

  printf("CPU user: %u%%, system: %u%%\n", perf.perc_cpu_user,
         perf.perc_cpu_sys);
  printf("Memory: total=%u kB, free=%u kB, available=%u kB, cached=%u kB\n",
         perf.mem_total_kb, perf.mem_free_kb, perf.mem_available_kb,
         perf.mem_cached_kb);
  return 0;
}
*/
