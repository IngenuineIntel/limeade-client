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

#define MAX_EVENTS_IN_PKT 150

void *th_events(struct thread_pass *data)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;
  struct limeade_events packet;
  struct limeade_indiv_event *events;
  int amt, amt_rem, idx, i, last_line;
  void *buffer, *line_start, *line_end;

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


    line_start = data;
    amt_rem = amt;

    for(i = 0, idx = 0; i < MAX_EVENTS_IN_PKT && amt_rem > 2; i++)
    {
      line_end = memmem(line_start, amt_rem, LCE_LINE_SEP, LCE_SEP_SZ);

      if(!line_end)
        line_end = line_start + amt_rem;


      // TODO
      
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



