// lce.c

// TODO includes
#include<linux/atomic.h>
#include<kprobes.h>

#include "lce.h"

atomic_t as_ready = ATOMIC_INIT(0);
atomic_t as_collecting = ATOMIC_INIT(1);

static int __init lce_init(void)
{
  
}

