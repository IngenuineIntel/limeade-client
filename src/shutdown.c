// shutdown.c
// shutdown manager

#include<errno.h>
#include<signal.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>

#include "module.h"

// frequency of operation
// hardcoded because it doesn't really matter
#define PROC_SHUTDOWN_HZ 4

void proc_shutdown(pid_t target)
{
  struct timespec rqtp, rmtp;

  rqtp.tv_sec  = 0;
  rqtp.tv_nsec = 999999999/PROC_SHUTDOWN_HZ;

  for(;;)
  {
    nanosleep(&rqtp, &rmtp);

    if(kill(target, 0) != 0 && errno == ESRCH)
    {
      unload_lce();
      exit(0);
    }
  }
}

#define start_shutdown_proc(target)\
  if(!fork()) proc_shutdown(target)

