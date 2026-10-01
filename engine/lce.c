// lce.c

#include<linux/atomic.h>
#include<kprobes.h>

#include "lce.h"

atomic_t as_ready = ATOMIC_INIT(0);
atomic_t as_collecting = ATOMIC_INIT(1);

static const struct proc_ops lce_proc_ops = {
  .proc_open = lce_proc_open,
  .proc_read = lce_proc_read,
  .proc_lseek = noop_llseek,
};

static int __init lce_init(void)
{
  INIT_KFIFO(lce_kfifo);

  
}

