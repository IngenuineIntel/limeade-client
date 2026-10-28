// limeade_client
// copyright (c) 2026 IngenuineIntel <roan.rothrock@proton.me>

#define VERSION "0.1"
#define MAINTAINER "IngenuineIntel"

#include <pthread.h>

#include <liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"

#include "config.c"
#include "intro.c"
#include "user_env.c"

#include "shutdown.c"
#include "th_events.c"
#include "th_handle.c"
#include "th_perf.c"
#include "th_proc.c"


#define LCE_PROCFILE "/proc/lce"

static struct thread_pass GLBL;
static struct prog_config CONFIG;

enum limeade_error populate_global_store()
{
  enum limeade_error ret;
  if((ret = limeade_init(&GLBL.lctx,
                         (CONFIG.net_mode == NETWORK_MODE_UDP
                         ? LIMEADE_MODE_CLIENT_ETH
                         : LIMEADE_MODE_CLIENT_SSH),
                         CONFIG.host)) != LIMEADE_OK) return ret;
  pthread_mutex_init(&GLBL.mtx_events_err, NULL);
  pthread_mutex_init(&GLBL.mtx_proc_err, NULL);
  pthread_mutex_init(&GLBL.mtx_perf_err, NULL);
  pthread_mutex_init(&GLBL.mtx_handle_err, NULL);
  sem_init(&GLBL.sem_shutdown, 1, 0);
  sem_init(&GLBL.sem_ready, 1, 0);
  sem_post(&GLBL.sem_ready);
  return 0;
}

void depopulate_global_store(void)
{
  limeade_release(&GLBL.lctx);
  pthread_mutex_destroy(&GLBL.mtx_events_err);
  pthread_mutex_destroy(&GLBL.mtx_proc_err);
  pthread_mutex_destroy(&GLBL.mtx_perf_err);
  pthread_mutex_destroy(&GLBL.mtx_handle_err);
  sem_post(&GLBL.sem_shutdown);
  sem_post(&GLBL.sem_ready);
  sem_destroy(&GLBL.sem_shutdown);
  sem_destroy(&GLBL.sem_ready);
}

static pthread_t TH_TIDS[4];
#define TH_EVENTS_TID TH_TIDS[0]
#define TH_PROC_TID   TH_TIDS[1]
#define TH_PERF_TID   TH_TIDS[2]
#define TH_HANDLE_TID TH_TIDS[3]
const char *thread_names[] = {"events", "proc", "perf", "handler"};
pthread_mutex_t *mtx_err[] = {
  &GLBL.mtx_events_err,
  &GLBL.mtx_proc_err,
  &GLBL.mtx_perf_err,
  &GLBL.mtx_handle_err,
};

int init_threads(void)
{
  if(pthread_create(&TH_EVENTS_TID, NULL, (void*(*)(void*))th_events, &GLBL) != 0)
    return -1;
  if(pthread_create(&TH_PROC_TID, NULL, (void*(*)(void*))th_proc, &GLBL) != 0)
  {
    pthread_kill(TH_EVENTS_TID, SIGKILL);
    return -1;
  }
  if(pthread_create(&TH_PERF_TID, NULL, (void*(*)(void*))th_perf, &GLBL) != 0)
  {
    pthread_kill(TH_PROC_TID, SIGKILL);
    pthread_kill(TH_EVENTS_TID, SIGKILL);
    return -1;
  }
  if(pthread_create(&TH_HANDLE_TID, NULL, (void*(*)(void*))th_handle, &GLBL) != 0)
  {
    pthread_kill(TH_PERF_TID, SIGKILL);
    pthread_kill(TH_PROC_TID, SIGKILL);
    pthread_kill(TH_EVENTS_TID, SIGKILL);
  }
  start_shutdown_proc(getpid());
  return 0;
}

#define start_threads() sem_post(&GLBL.sem_ready)

void stop_threads(void)
{
  for(int i = 0; i < sizeof(TH_TIDS) / sizeof(pthread_t); i++)
  {
    sem_post(&GLBL.sem_ready);
    sem_post(&GLBL.sem_shutdown);
  }
  for(int i = 0; i < sizeof(TH_TIDS) / sizeof(pthread_t); i++)
    pthread_join(TH_TIDS[i], NULL);
}

int check_threads(void)
{
  int ret;

  for(int i = 0; i < sizeof(mtx_err) / sizeof(pthread_mutex_t*); i++)
  {
    ret = pthread_mutex_trylock(mtx_err[i]);
    if(!ret)
    {
      pthread_mutex_unlock(mtx_err[i]);
      log_err("%s thread exited unexpectedly", thread_names[i]);
      return -1;
    }
    if(ret != EBUSY)
    {
      log_err("failed to check %s thread: %s", thread_names[i], strerror(ret));
      return -1;
    }
  }

  return 0;
}

void wait_for_something_to_happen(void)
{
  struct timespec wait = {0, 100000000};

  while(check_threads() == 0)
    nanosleep(&wait, NULL);
}

unsigned int us_from_ts(struct timespec *ts_start, struct timespec *ts_stop)
{

  uint64_t s, ns;

  s  = ts_stop->tv_sec  - ts_start->tv_sec;
  ns = ts_stop->tv_nsec - ts_start->tv_nsec;

  if(ns < 0) // carrying
  {
    s--;
    ns += 1000000000;
  }

  return (s * 1000000) + (ns / 1000);
}

#define INIT_CLOCK()    struct timespec ts_start, ts_stop
#define START_CLOCK()   clock_gettime(CLOCK_MONOTONIC, &ts_start)
#define STOP_CLOCK()    clock_gettime(CLOCK_MONOTONIC, &ts_stop)
#define MEASURE_CLOCK() us_from_ts(&ts_start, &ts_stop)

int main(int argc, char **argv)
{


  int ret = 0;
  INIT_CLOCK();

  populate_conf(&CONFIG, argc, argv);
  LOGGING_INIT(CONFIG.logfile);
  set_log_level(CONFIG.log_level);
  
  if(confirm_system_configuration() < 0)
    return 1;

  log_info("LIMEADE CLIENT v%s", VERSION);
  log_info("Created by %s under the AGPLv3 license", MAINTAINER);
  log_info("");
  log_dbg("Starting up...");
  
  START_CLOCK();

  ret = populate_global_store();
  if(ret < 0)
  {
    log_crit("Failed to initialize global store, exiting...");
    return 1;
  }

  if(init_threads() < 0)
  {
    log_crit("Failed to initialize threads, exiting...");
    depopulate_global_store();
    return 1;
  }

  log_info("Checking threads");

  if(check_threads() < 0)
  {
    log_crit("Thread(s) died after starting, exiting...");
    stop_threads();
    depopulate_global_store();
    return 1;
  }

  if((ret = limeade_connect(&GLBL.lctx)) != LIMEADE_OK)
  {
    log_crit("Failed to connect to server: %s", LIMEADE_ERROR_REPRS[ret]);
    ret = 1;
    goto undo_startup;
  }

  {
    struct limeade_intro introduction = generate_introduction();

    ret = limeade_send_await(&GLBL.lctx, LIMEADE_PACKET_INTRO, introduction);
    release_introduction(&introduction);

    if(ret != LIMEADE_OK)
    {
      log_crit("Failed to send introduction: %s", LIMEADE_ERROR_REPRS[ret]);
      release_introduction(&introduction);
      ret = 1;
      goto undo_startup;
    }
  }
  
  STOP_CLOCK();

  log_dbg("Startup completed in %ius", MEASURE_CLOCK());

  start_threads();

  START_CLOCK();

  wait_for_something_to_happen();

  STOP_CLOCK();

  log_info("Ran for %ius", MEASURE_CLOCK());

  ret = 0;

undo_startup:
  stop_threads();
  depopulate_global_store();
  return ret;
}


