// th_events.c

#include<errno.h>
#include<limits.h>
#include<semaphore.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<unistd.h>

#include<liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"
#include "../engine/type_enum.h"

#define MAX(T) (~(1U<<(sizeof(T)*8-1)))

#define INC(idx, end)\
  idx = memchr(idx, '\t', end - idx);\
  if(!idx)

int parse_lce_stream(const void *in, uint32_t in_sz,
                     struct limeade_indiv_event *out, uint32_t out_sz)
{
  /* Takes data read from LCE and processes it into data usable by liblimeade
   * to generate packets.
   * NOTE: this function assumes that `in` is NULL terminated
   */
  register struct limeade_indiv_event *j;
  const void * const buff_end = in + in_sz;
  const char *buff_idx, *line_end;
  char *endptr = NULL;
  int64_t ts;
  int type, nr_parsed = 0;

  line_end = in;
  for(int i = 0; i < out_sz; i++)
  {
    j = &out[i];

    // naive memchr
    buff_idx = line_end;
    line_end = memchr(line_end, '\n', buff_end - (void*)line_end);
    if(!line_end)
      goto premature_end;

    ts = strtoll(buff_idx, &endptr, 10);
    if(endptr == buff_idx)
      goto skip_iter;

    INC(buff_idx, line_end)
      goto premature_line_end;
    buff_idx++;

    j->pid = strtol(buff_idx, &endptr, 10);
    if(endptr == buff_idx || j->pid == INT_MAX)
      goto skip_iter;

    INC(buff_idx, line_end)
      goto premature_line_end;
    buff_idx++;

    type = strtol(buff_idx, &endptr, 10);
    if(endptr == buff_idx || type >= LCE_EVENT_MAX || type <= 0)
      goto skip_iter;

    j->syscall = (char*)LCE_EVENT_REPR[type];

    INC(buff_idx, line_end)
      goto premature_line_end;

    j->arg1 = (char*)++buff_idx;


    INC(buff_idx, line_end)
      goto premature_line_end;

    j->arg2 = (char*)++buff_idx;

    INC(buff_idx, line_end)
      goto premature_line_end;
    buff_idx++;

    j->retval = strtol(buff_idx, &endptr, 10);
    if(endptr == buff_idx || j->retval == INT_MAX)
      goto skip_iter;

    j->ts_ms  = ts / 1000000;
    j->ts_s   = j->ts_ms / 1000;
    j->ts_ms -= j->ts_s;

    //log_dbg2("ts_s=%i, ts_ms=%i, type=%s", j->ts_s, j->ts_ms, j->syscall);
    //log_dbg2("arg1=%s, arg2=%s, retval=%i", j->arg1, j->arg2, j->retval);

    goto iter_end;

premature_line_end:
iter_end:
    nr_parsed++;
skip_iter:
    line_end++;
}
premature_end:
  return nr_parsed;
}

void *th_events(struct thread_pass * const glbl)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;
  struct limeade_events pkt;
  int amt, ret;

  signal(SIGUSR1, thread_recv_sig);

  goal_wait.tv_sec  = glbl->event_ms_p / 1000;
  goal_wait.tv_nsec = glbl->event_ms_p % 1000 * 1000000;

  FILE *fd = fopen(glbl->procfile, "r");
  if(!fd)
  {
    log_err("failed to open %s: %s", glbl->procfile, strerror(errno));
    log_err("Is the LCE module loaded? (Run 'modprobe lce'");
    sem_post(&glbl->sem_ready);
    return NULL;
  }

  pthread_mutex_lock(&glbl->mtx_events_err);
  log_dbg("th_events ready");
  sem_post(&glbl->sem_ready);
  while(sem_wait(&glbl->sem_start) != 0 && errno == EINTR);

  struct limeade_indiv_event * const events = malloc(128 * sizeof(struct limeade_indiv_event));
  void * const buffer = malloc(65536);

  pkt.events = events;

  while(sem_trywait(&glbl->sem_shutdown) != 0 && errno == EAGAIN)
  {
    clock_gettime(CLOCK_MONOTONIC, &iter_start);

    amt = fread(buffer, 1, 65536, fd);
    if(!amt) // no data in procfile
    {
      log_dbg3("no data in procfile, skipping this iteration");
      goto iter_end;
    }
    if(amt < 0)
    {
      log_err("failed to read %s: %s", glbl->procfile, strerror(errno));
      goto err;
    }
    
    pkt.nr_events = parse_lce_stream(buffer, amt, events, 128);
    if(!pkt.nr_events)
    {
      log_dbg2("failed to parse any data from the procfile (%i bytes)", amt);
      goto iter_end;
    }

    ret = limeade_send_await(&glbl->lctx, LIMEADE_PACKET_EVENTS, events);
    if(ret != LIMEADE_OK)
    {
      log_err("limeade_send_await failed: %s", LIMEADE_ERROR_REPRS[ret]);
      goto err;
    }

iter_end:

    clock_gettime(CLOCK_MONOTONIC, &iter_end);

    diff.tv_sec  = iter_end.tv_sec - iter_start.tv_sec;
    diff.tv_nsec = iter_end.tv_nsec - iter_start.tv_nsec;
    real_wait.tv_sec  = goal_wait.tv_sec  - diff.tv_sec;
    real_wait.tv_nsec = goal_wait.tv_nsec - diff.tv_nsec;
    if(real_wait.tv_nsec < 0) // carry
    {
      real_wait.tv_sec--;
      real_wait.tv_nsec += 999999999;
    }

    if(real_wait.tv_sec < 0 || real_wait.tv_sec < 0)
    {
      log_warn(
          "event thread took longer than the allocated time (%lins when the goal was %lins)",
          ((int64_t)diff.tv_sec)      * 999999999 + diff.tv_nsec,
          ((int64_t)goal_wait.tv_sec) * 999999999 + goal_wait.tv_nsec);
      continue;
    }

    nanosleep(&real_wait, &diff);
  }

err:

  fclose(fd);
  free(events);
  free(buffer);
  pthread_mutex_unlock(&glbl->mtx_events_err);
  return NULL;
}

/*
#define buff_amt (1 << 12)
#define proc_amt (1 << 15)
int main()
{
  struct timespec start, end;
  logging_init();
  enable_really_verbose_logging();
  
  FILE *fd = fopen("/proc/lce", "r");
  if(!fd)
  {
    log_err("failed to open \"%s\"", "/proc/lce");
    return -1;
  }

  const void * const unparsed_buffer = malloc(buff_amt);
  void * const parsed_buffer = malloc(proc_amt * sizeof(struct limeade_indiv_event));

  int amt = fread((void*)unparsed_buffer, 1, buff_amt - 1, fd);
  ((char*)unparsed_buffer)[amt] = '\0';

  clock_gettime(CLOCK_MONOTONIC, &start);
  int total_parsed = parse_lce_stream(unparsed_buffer, amt, parsed_buffer, proc_amt);
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("total: %i\n", total_parsed);

  log_info("total execution time was %lius", (end.tv_nsec - start.tv_nsec)/1000);
  return 0;



  return 0;
}
*/
