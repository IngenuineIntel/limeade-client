// procfile.c
// AGPL

//#include<emmintrin.h>

#include "lce.h"

int lce_proc_open(struct inode *inode, struct file *file)
{
  return 0;
}

// this file becomes very difficult to parse if there are extra tabs
// hence, this is worth doing
// I also think this is worth doing via SSE SIMD, but I'll let it wait
/*
static void arg_sanatize(char *s)
{
  __m128i endchr, tabchr, chunk, m1, m2, mask;
  int match;
  uint32_t len;

  endchr = _mm_set1_epi8('\0');
  tabchr = _mm_set1_epi8('\t');
  len    = 0;

  for(;;)
  {
    chunk = _mm_loadu_si128((const __m128i*)s);
  }
}
*/

// simple "naive" implementation in the meantime
static void arg_sanatize(char *s)
{
  for(char *i = s; *i != '\0'; i++)
    if(*i == '\t')
      *i = ' ';
    else if(*i == '\n')
      *i = ' ';
}

ssize_t lce_proc_read(struct file *file, char __user *ubuf, size_t count,
                             loff_t *ppos)
{
  struct lce_event ev;
  char line[1024];
  ssize_t total = 0;
  unsigned long lock_flags;
  int len;

  pr_info("lce: procfile being read by process %i", current->pid);

  while(count > 0)
  {
    spin_lock_irqsave(&lce_kfifo_lock, lock_flags);
    if(!kfifo_get(&lce_kfifo, &ev))
    {
      pr_err("failed to access kfifo");
      spin_unlock_irqrestore(&lce_kfifo_lock, lock_flags);
      break;
    }
    spin_unlock_irqrestore(&lce_kfifo_lock, lock_flags);

    arg_sanatize(ev.arg1);
    arg_sanatize(ev.arg2);

    // null bytes are copied so that the userspace program can use the data as
    // strings directly without copying the data
    len = snprintf(line, sizeof(line), "%llu\t%d\t%i\t%s\t\%s\t%i\n", ev.ts,
                   ev.pid, (int)ev.type, ev.arg1, ev.arg2, ev.ret);

    pr_info("lce: procfile line length: %i", len);
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
  pr_info("procfile read was successful (sent %ld bytes to userspace)", total);
  return total;
}
