// thread_pass.h

#pragma once

#include<pthread.h>
#include<semaphore.h>

#include<liblimeade/liblimeade.h>

// all sending threads need a single data structure through which to communicate
// this is that structure
struct thread_pass
{
  struct limeade_context lctx; // liblimeade object
  sem_t sem_shutdown; // semaphore for shutting down threads
  sem_t sem_ready;    // semaphore for holding the initialization of the
                      // threads
  int event_ms_p, proc_ms_p, perf_ms_p; // thread iteration frequency (ms(p))
  
  // the main thread will lock all of these before startings the threads, then
  // release them when it is ready
  pthread_mutex_t mtx_events_err; // unlocked by the events thread on fatal error
  pthread_mutex_t mtx_proc_err;   // unlocked by the proc thread on fatal error
  pthread_mutex_t mtx_perf_err;   // unlocked by the perf thread on fatal error
  pthread_mutex_t mtx_handle_err; // unlocked by the handle thread on fatal error
  const char *procfile; // path to LCE output
};

void thread_recv_sig(int signal)
{}

