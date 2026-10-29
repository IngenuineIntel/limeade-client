// th_handle.c
// processes incoming packets

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <signal.h>
#include<unistd.h>

#include <liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"

void *th_handle(struct thread_pass * const glbl)
{
  struct limeade_recvd pkt;
  int ret;

  signal(SIGUSR1, thread_recv_sig);

  pthread_mutex_lock(&glbl->mtx_handle_err);
  log_dbg("th_handle ready");
  sem_post(&glbl->sem_ready);
  while(sem_wait(&glbl->sem_start) != 0 && errno == EINTR);

  while(sem_trywait(&glbl->sem_shutdown) != 0 && errno == EAGAIN)
  {
    ret = limeade_recv_wait(&glbl->lctx, &pkt, 3000);
    if(ret != LIMEADE_OK)
    {
      if(ret != LIMEADE_ERROR_INTERRUPT && ret != LIMEADE_ERROR_NO_DATA)
      {
        log_err("error while awaiting packet: %s", LIMEADE_ERROR_REPRS[ret]);
        log_crit("th_handle exiting...");
        goto err;
      } else continue;
    } else
    {
      switch(pkt.type)
      {
        case LIMEADE_PACKET_CLOSE:
        {
          log_crit("Server closed connection!");
          struct limeade_close pkt_close;
          if((ret = limeade_parse_close(&pkt_close, &pkt)) != LIMEADE_OK)
            log_info("Failed to parse CLOSE packet: %s", LIMEADE_ERROR_REPRS[ret]);
          else
            log_info("Explanation from server: %s", pkt_close.explanation);
          limeade_release(&pkt_close);
          limeade_release(&pkt);
          
          log_warn("Shutting down...");
          goto err;
        }
        default:
          log_warn("Received useless packet type (%i) from server", pkt.type);
      }
    }
    limeade_release(&pkt);
  }
err:
  pthread_mutex_unlock(&glbl->mtx_handle_err);
  return NULL;
}
