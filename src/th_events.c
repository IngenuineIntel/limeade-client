// th_events
// the events thread

#include<errno.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

#include<liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"
#include "../engine/type_enum.h"
#include "module.h"

#define MAX_EVENTS_IN_PKT 150

#define INC_IDX(idx, delim, end)\
  idx = memchr(idx, delim, end - idx);\
  if(!idx)

void *th_events(struct thread_pass *data)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;
  struct limeade_events packet;
  struct limeade_indiv_event *events, *j;
  int amt, amt_rem, i, last_line, ret, type;
  int64_t ts;
  void *buffer, *line_start, *line_end, *idx;
  char *endptr;

  events = calloc(sizeof(struct limeade_indiv_event), MAX_EVENTS_IN_PKT);
  buffer = malloc(65536);

  packet.events = events;

  FILE *fd = fopen(data->procfile, "r");
  if(!fd)
  {
    log_err("failed to open %s: %s", data->procfile, strerror(errno));
    return NULL;
  }

  pthread_mutex_lock(&data->mtx_events_err);

  goal_wait.tv_sec  = 0;
  goal_wait.tv_nsec = 999999999/data->event_hz;

  while(pthread_mutex_trylock(&data->mtx_shutdown) == EBUSY)
  {
    clock_gettime(CLOCK_MONOTONIC, &iter_start);

    amt = fread(buffer, 1, 65536, fd);
    if(amt <= 0)
    {
      if(amt == 0)
      {
        log_dbg("no data in procfile");
        goto iter_end;
      }
      log_err("failed to read %s: %s", data->procfile, strerror(errno));
    }

    line_start = idx = buffer;
    amt_rem = amt;

    for(i = 0, idx = 0; i < MAX_EVENTS_IN_PKT && amt_rem; i++)
    {
      type = 0;

      if(!amt_rem)
        break;

      line_end = memchr(line_start, '\n', amt_rem);
      if(!line_end)
        break;

      j = &events[i];

      ts = strtoll(idx, &endptr, 10);
      if(endptr == idx)
      {
        log_warn("failed to parse timestamp from entry, skipping");
        goto iter_skip;
      }
  
      INC_IDX(idx, '\t', line_end)
      {
        log_warn("premature end of line after timestamp, skipping");
        goto iter_skip;
      }

      j->pid = strtol(idx, &endptr, 10);
      if(endptr == idx)
      {
        log_warn("failed to parse pid from entry, skipping");
        goto iter_skip;
      }

      INC_IDX(idx, '\t', line_end)
      {
        log_warn("premature end of line after pid, skipping");
        goto iter_skip;
      }

      type = strtol(idx, &endptr, 10);
      if(endptr == idx || type < LCE_EVENT_INVALID || type > LCE_EVENT_MAX)
      {
        log_warn("failed to parse type entry, skipping");
        goto iter_skip;
      }

      INC_IDX(idx, '\t', line_end)
      {
        log_warn("premature end of line after type, skipping");
        goto iter_skip;
      }

      j->arg1 = idx;

      INC_IDX(idx, '\t', line_end)
      {
        log_warn("premature end of line after arg2, skipping");
        goto iter_skip;
      }

      j->arg2 = idx;

      INC_IDX(idx, '\t', line_end)
      {
        log_warn("premature end of line after arg2, skipping");
        goto iter_skip;
      }

      j->retval = strtol(idx, &endptr, 10);
      if(endptr == idx)
      {
        log_warn("failed to parse return val, skipping");
        goto iter_skip;
      }

      j->ts_ms  = ts       / 1000000;
      j->ts_s   = j->ts_ms / 1000;
      j->ts_ms -= j->ts_s;

      j->syscall = (char*)LCE_EVENT_REPR[type];

      log_dbg3("ts_s=%i, ts_ms=%i, pid=%i", j->ts_ms, j->ts_ms, j->pid);
      log_dbg3("syscall=%s", j->syscall);
      log_dbg3("arg1=%s, arg2=%s", j->arg1, j->arg2);
      log_dbg3("retval=%i", j->retval);

      goto subiter_end;

      iter_skip:

      i--;

      subiter_end:

      amt_rem -= ++line_end - line_start;
      line_start = line_end;

    }

    packet.nr_events = i;

    // NOTE TO SELF
    // don't test until LIMEADE_PACKET_ACK includes the type of the packet _and_
    // limeade_send_await checks for such a value
    // NOTE TO ANYONE ELSE
    // a program that ues liblimeade (specifically a multithreaded program) that
    // uses both limeade_send & limeade_send_await is likely going to get weird
    // mixups about the ACKs being sent, as it will detect the wrong ones & give
    // false negatives
    ret = limeade_send_await(&data->lctx, LIMEADE_PACKET_EVENTS, events);
    if(ret != LIMEADE_OK)
    {
      log_err("`limeade_send` failed: %s", LIMEADE_ERROR_REPRS[i]);
      goto err;
    }


    clock_gettime(CLOCK_MONOTONIC, &iter_end);

    iter_end:

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
      log_dbg(
        "event thread took longer than the alloted time (%ins when the goal was %ins)",
        diff.tv_sec      * 999999999 + diff.tv_nsec,
        goal_wait.tv_sec * 999999999 + goal_wait.tv_nsec
      );

      continue;
    }

    nanosleep(&real_wait, &diff);
  }

err:
  fclose(fd);
err_wo_close:
  free(events);
  free(buffer);
  pthread_mutex_unlock(&data->mtx_events_err);
  return NULL;
}

int main()
{
  load_lce();
  // TODO
  unload_lce();
  return 0;
}

