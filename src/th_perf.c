// th_perf.c

#include<signal.h>
#include<stdlib.h>
#include<string.h>

#include<liblimeade/liblimeade.h>

#include "thread_pass.h"

#define MEM_TOTAL_TARGET     "MemTotal:"
#define MEM_FREE_TARGET      "MemFree:"
#define MEM_AVAILABLE_TARGET "MemAvailable:"
#define MEM_CACHED_TARGET    "Cached:"
#define ITERATE_BACKWARDS(ptr, target)\
  for(; *ptr != target; ptr--)

int populate_meminfo(const char * const in, const int in_sz, struct limeade_perf * const out)
{
  /* populates mem_*_kb fields of `out` with data from /proc/meminfo
   * 0 on success, -1 on error */

  char *total_kb_idx, *free_kb_idx, *available_kb_idx, *cached_kb_idx, *endptr;
  void *total_ln_end, *free_ln_end, *available_ln_end, *cached_ln_end, *max_idx;
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

  total_ln_end     = memchr(total_kb_idx,     '\n', max_idx - (void*)total_kb_idx);
  free_ln_end      = memchr(free_kb_idx,      '\n', max_idx - (void*)free_kb_idx);
  available_ln_end = memchr(available_kb_idx, '\n', max_idx - (void*)available_kb_idx);
  cached_ln_end    = memchr(cached_kb_idx,    '\n', max_idx - (void*)cached_kb_idx);

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
  if(endptr == free_kb_str) return -1;

  out->mem_cached_kb = strtol(++cached_kb_str, &endptr, 10);
  if(endptr == cached_kb_str) return -1;

  return 0;
}

struct statinfo { uint32_t uptime, user, system; };
static struct statinfo stat_cache_1, stat_cache_2;
static struct statinfo *stat_prev = &stat_cache_1, *stat_next = &stat_cache_2;

inline void next_stat(void)
{
  struct statinfo *tmp;
  tmp = stat_prev;
  stat_prev = stat_next;
  stat_next = tmp;
}

int populate_statinfo(const char * const in, const int in_sz, struct statinfo *const out)
{
  // TODO
  return 0;
}

int8_t cpu_usage_from_stat(struct statinfo *start, struct statinfo *end, uint32_t interval_ms)
{
  // TODO
  return 0;
}

void *th_perf(struct thread_pass * const glbl)
{
  signal(SIGUSR1, thread_recv_sig);
  // TODO
  return NULL;
}


/*
int main()
{
  // TODO
  return 0;
}
*/

