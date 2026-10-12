// th_events.c

#include<stdlib.h>
#include<unistd.h>

#include<liblimeade/liblimeade.h>

#include "logging.h"
#include "../engine/type_enum.h"

#define MAX(T) (~(1U<<(sizeof(T)*8-1)))

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
    log_dbg2("line start @ %p", line_end);
    for(; *line_end != '\n'; line_end++)
      if((void*)line_end >= buff_end)
        goto premature_end;
    buff_idx = line_end;
    log_dbg2("line end @ %p", line_end);

    log_dbg3("made it to strtoll");
    ts = strtoll(buff_idx, &endptr, 10);
    if(endptr == buff_idx)
    {
      log_dbg2("failed to parse timestamp, skipping");
      goto skip_iter;
    }

    for(; *buff_idx != '\t'; buff_idx++)
      if(buff_idx >= line_end)
        goto premature_line_end;
    buff_idx++;

    log_dbg3("make it to pid");
    j->pid = strtol(buff_idx, &endptr, 10);
    if(endptr == buff_idx || j->pid == MAX(pid_t))
    {
      log_dbg2("failed to parse pid, skipping");
      goto skip_iter;
    }

    for(; *buff_idx != '\t'; buff_idx++)
      if(buff_idx >= line_end)
        goto premature_line_end;
    buff_idx++;

    log_dbg3("make it to type");
    type = strtol(buff_idx, &endptr, 10);
    if(endptr == buff_idx || type >= LCE_EVENT_MAX || type <= 0)
    {
      log_dbg2("failed to parse type, skipping");
      goto skip_iter;
    }
    j->syscall = (char*)LCE_EVENT_REPR[type];

    for(; *buff_idx != '\t'; buff_idx++)
      if(buff_idx >= line_end)
        goto premature_line_end;

    j->arg1 = (char*)++buff_idx;

    for(; *buff_idx != '\t'; buff_idx++)
      if(buff_idx >= line_end)
        goto premature_line_end;

    j->arg2 = (char*)++buff_idx;

    for(; *buff_idx != '\t'; buff_idx++)
      if(buff_idx >= line_end)
        goto premature_line_end;
    buff_idx++;

    log_dbg3("make it to retval");
    j->retval = strtol(buff_idx, &endptr, 10);
    if(endptr == buff_idx || j->retval == MAX(int))
    {
      log_dbg2("failed to parse retval, skipping");
      goto skip_iter;
    }

    // TODO parsing ts into j->ts_s & j->ts_ms
    j->ts_ms  = ts / 1000000;
    j->ts_s   = j->ts_ms / 1000;
    j->ts_ms -= j->ts_s;

    goto iter_end;

premature_line_end:
    log_warn("line prematurely ended, skipping");
    
skip_iter:
    nr_parsed--;

iter_end:
    nr_parsed++;
    line_end++;
}

  return nr_parsed;

premature_end:
  log_warn("proc data prematurely ended");
  return nr_parsed;
}

#define buff_amt (1 << 12)
#define proc_amt (1 << 15)
int main()
{
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

  int total_parsed = parse_lce_stream(unparsed_buffer, buff_amt, parsed_buffer, proc_amt);
  printf("total: %i\n", total_parsed);
  return 0;



  return 0;
}
