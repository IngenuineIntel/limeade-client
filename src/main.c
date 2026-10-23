// limeade_client
// copyright (c) 2026 IngenuineIntel <roan.rothrock@proton.me>

#include "thread_pass.h"

#include "config.c"
#include "th_events.c"
#include "th_perf.c"
#include "th_proc.c"

#define LCE_PROCFILE "/proc/lce"

static struct thread_pass GLOBAL_STORE;
static struct prog_config CONFIG;
#define GLBL GLOBAL_STORE

int populate_global_store(void)
{
  if(limeade_init(&GLBL.lctx,
                  LIMEADE_MODE_CLIENT_SSH|LIMEADE_MODE_LOW_COMPRESSION,
                  "127.0.0.1") != LIMEADE_OK) return -1;
  pthread_mutex_init(&GLBL.mtx_events_err, NULL);
  pthread_mutex_init(&GLBL.mtx_proc_err, NULL);
  pthread_mutex_init(&GLBL.mtx_perf_err, NULL);
  sem_init(&GLBL.sem_shutdown, 1, 0);
  sem_init(&GLBL.sem_ready, 1, 0);
  sem_wait(&GLBL.sem_shutdown);
  sem_wait(&GLBL.sem_ready);
  return 0;
}

int start_threads(void)
{
}

void stop_threads(void)
{}


int main(int argc, char **argv)
{
  populate_conf(&CONFIG, argc, argv);
  return 0;
}


