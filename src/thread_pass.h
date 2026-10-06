// thread_pass.h

#pragma once

#include<pthread.h>

#include<liblimeade/liblimeade.h>

// all sending threads need a single data structure through which to communicate
// this is that structure
struct thread_pass
{
  struct limeade_context lctx; // liblimeade object
  pthread_mutex_t mtx_shutdown; // mutex for the main thread to kill all other
                                // threads
  int event_hz, proc_hz, perf_hz; // thread interation frequency
  pthread_mutex_t mtx_events_err; // locked by the events thread on fatal error
  pthread_mutex_t mtx_proc_err;   // locked by the proc thread on fatal error
  pthread_mutex_t mtx_perf_err;   // locked by the perf thread on fatal error
  const char *procfile; // path to LCE output
};

