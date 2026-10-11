// th_events
// the events thread

#include<errno.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"
#include "../engine/type_enum.h"
#include "module.h"

#define MAX_EVENTS_IN_PKT 150
#define INC()\
  line_next = memchr(line_idx, '\t', line_end - line_idx);\
  if(!line_next)\
  {\
    log_warn("premature end of line, skipping");\
    goto iter_skip;\
  }\
  log_dbg3("the current line has %i bytes after the current index", line_end - line_next);\
  line_idx = line_next + 1;

int parse_lce_stream(const void *in, unsigned int in_sz, struct limeade_indiv_event *out, unsigned int out_sz)
{
  /* parses LCE data intro struct limeade_indiv_event
   * returns the number of packets successfully parsed
   */
  log_dbg3("we entered the function!");
  const void *line_end, *line_idx, *line_next, *max;
  char *endptr;
  int nr_parsed = 0, line_rem, type;
  int64_t ts;
  struct limeade_indiv_event *j;

  max = in + in_sz;
  line_idx = line_end = in;

  for(int i = 0; in_sz > 0 && i < out_sz; i++)
  {
    log_dbg3("we made it to the first memchr!");
    line_end = memchr(line_idx, '\n', max - line_idx);
    if(!line_end)
      break;
    log_dbg3("we made it past the first memchr!");

    line_rem = line_end - line_idx;

    j = &out[i];

    log_dbg3("we survived dereferencing!");

    ts = strtoll(line_idx, &endptr, 10);

    log_dbg3("we survived strtoll!");

    if(endptr == line_idx)
    {
      log_warn("failed to parse timestamp from entry, skipping");
      goto iter_skip;
    }

    INC();

    log_dbg3("we survived increment!");

    j->pid = strtol(line_idx, &endptr, 10);
    log_dbg3("we survived pid strtol! (%i)", j->pid);
    if(endptr == line_idx)
    {
      log_warn("failed to parse pid, skipping");
      goto iter_skip;
    }

    INC();

    log_dbg3("we survived second increment!");

    type = strtol(line_idx, &endptr, 10);
    log_dbg3("we survived type strtol! (%i)", type);
    if(endptr == line_idx)
    {
      log_warn("failed to parse type, skipping");
      goto iter_skip;
    }

    if(type > LCE_EVENT_MAX)
    {
      log_warn("type identifier %i was more than max (%i), skipping", type, LCE_EVENT_MAX);
      goto iter_skip;
    }
    j->syscall = (char*)LCE_EVENT_REPR[type];

    log_dbg3("we survived assigning syscall!");

    INC();

    log_dbg3("we survied third increment!");

    j->arg1 = (char*)line_idx;

    INC();

    log_dbg3("we survived fourth increment!");

    j->arg2 = (char*)line_idx;

    INC();

    log_dbg3("we survived fifth increment!");

    j->retval = strtol(line_idx, &endptr, 10);
    if(endptr == line_idx)
    {
      log_warn("failed to parse ret, skipping");
      goto iter_skip;
    }

    log_dbg3("we survived retval strtol!");

    log_dbg3("ts_s=%i, ts_ms=%i, pid=%i", j->ts_ms, j->ts_ms, j->pid);
    log_dbg3("syscall=%s", j->syscall);
    log_dbg3("arg1=%s, arg2=%s", j->arg1, j->arg2);
    log_dbg3("retval=%i", j->retval);

    if(!ts)
      log_warn("timestamp @ 0 somehow lol");
    else
    {
      j->ts_ms  = ts       / 1000000;
      j->ts_s   = j->ts_ms / 1000;
      j->ts_ms -= j->ts_s;
    }

    nr_parsed++;

    goto iter_end;
    
    iter_skip:
    i--;

    iter_end:

    line_idx = ++line_end;
    in_sz    = max - line_idx;

  }

  return nr_parsed;
}

void *th_events(struct thread_pass *data)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;
  struct limeade_events packet;
  struct limeade_indiv_event *events;
  int amt, i, ret;
  void *buffer;

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

    packet.nr_events = parse_lce_stream(buffer, amt, events, MAX_EVENTS_IN_PKT);
    if(!packet.nr_events)
      log_dbg2("didn't parse anything?");

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

#undef INC

#define buff_amt (1<<10)
int main()
{
  logging_init();
  enable_really_verbose_logging();
  load_lce();
  FILE *fd = fopen("/proc/lce", "r");
  if(!fd)
  {
    log_err("failed to open \"%s\"", "/proc/lce");
    return -1;
  }
  const void * const unparsed_buffer = malloc(buff_amt);
  void * const parsed_buffer   = malloc(buff_amt);

  log_dbg("unparsed: %p", unparsed_buffer);
  log_dbg("parsed:   %p", parsed_buffer);

  fread((void*)unparsed_buffer, 1, buff_amt - 1, fd);
  write(STDOUT_FILENO, unparsed_buffer, buff_amt);

  log_dbg("here we go!");
  int ret = parse_lce_stream(unparsed_buffer, buff_amt, parsed_buffer, buff_amt);
  log_info("parse_lce_stream() returned %i", ret);

  fclose(fd);

  log_dbg("unparsed: %p", unparsed_buffer);
  log_dbg("parsed:   %p", parsed_buffer);

  free((void*)unparsed_buffer);
  free(parsed_buffer);

  return 0;
}
#undef buff_amt

