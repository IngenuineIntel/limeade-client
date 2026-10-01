// procfile.c
// AGPL

#include "lce.h"

static int lce_proc_open(struct inode *inode, struct file *file)
{
  return 0;
}

static ssize_t lce_proc_read(struct file *file, char __user *ubuf, size_t count,
                             loff_t *ppos)
{
  struct lce_event ev;
  char line[1024];
  ssize_t total = 0;
  unsigned long lock_flags;
  int len;

  while(count > 0)
  {
    spin_lock_irqsave(&lce_kfifo_lock, lock_flags);
    if(!kfifo_get(&lce_kfifo, &ev))
    {
      spin_unlock_irqrestore(&lce_kfifo_lock, lock_flags);
      break;
    }
    spin_unlock_irqrestore(&lce_kfifo_lock, lock_flags);

    len = snprintf(line, sizeof(line), "%llu\t%d\t%i\t%s\t%s\t%i", ev.ts,
                   ev.pid, (int)ev.type, ev.arg1, ev.arg2, ev.ret);

    if(len <= 0)
      continue;

    if(len > count)
      break;

    if(copy_to_user(ubuf + total, line, len))
    {
      if(!total)
        return -EFAULT;
      break;
    }

    total += len;
    count -= len;
  }
  return total;
}
