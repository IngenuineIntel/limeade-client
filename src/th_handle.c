// th_handle.c
// processes incoming packets

#include <errno.h>
#include <signal.h>

#include <liblimeade/liblimeade.h>

#include "logging.h"
#include "thread_pass.h"

void *th_handle(void *data)
{
  struct thread_pass * const glbl = data;
  struct limeade_recvd pkt;
  int ret;

  signal(SIGUSR1, thread_recv_sig);

  pthread_mutex_lock(&glbl->mtx_handle_err);
  log_dbg("th_handle ready");
  sem_wait(&glbl->sem_ready);

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
      }
    } else
    {
      switch(pkt.type)
      {
        case LIMEADE_PACKET_COMMANDEER:
          // TODO
        case LIMEADE_PACKET_CLOSE:
          log_crit("server closed connection!");
          log_warn("shutting down...");
          goto err;
        default:
          log_warn("received useless packet type (%i) from server", pkt.type);
      }
    }
  }
err:
  pthread_mutex_unlock(&glbl->mtx_handle_err);
  return NULL;
}
