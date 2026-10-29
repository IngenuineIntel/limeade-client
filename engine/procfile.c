// procfile.c
// AGPL

//#include<emmintrin.h>

#include <linux/cred.h>
#include <linux/kernel.h>
#include <linux/kernel_read_file.h>
#include <linux/user_namespace.h>

#include "lce.h"

#define LCE_GROUP_FILE "/etc/group"
#define LCE_GROUP_FILE_MAX (1024 * 1024)

static int lce_populate_limeade_gid(kgid_t *target_gid)
{
  static const char group_name[] = "limeade";
  void *buf = NULL;
  char *data, *line, *line_end, *separator, *gid_start, *gid_end;
  char gid_text[11];
  size_t file_size = 0;
  ssize_t read_size;
  u32 gid;
  int ret = -ENOENT;

  read_size = kernel_read_file_from_path(LCE_GROUP_FILE, 0, &buf,
                                         LCE_GROUP_FILE_MAX, &file_size,
                                         READING_UNKNOWN);
  if(read_size < 0)
  {
    kvfree(buf);
    return (int)read_size;
  }

  data = buf;
  file_size = min_t(size_t, file_size, read_size);
  for(line = data; file_size && line < data + file_size;)
  {
    line_end = memchr(line, '\n', data + file_size - line);
    if(!line_end)
      line_end = data + file_size;

    separator = memchr(line, ':', line_end - line);
    if(separator && separator - line == sizeof(group_name) - 1 &&
       !memcmp(line, group_name, sizeof(group_name) - 1))
    {
      gid_start = memchr(separator + 1, ':', line_end - separator - 1);
      if(!gid_start || ++gid_start >= line_end)
      {
        ret = -EINVAL;
        goto done;
      }

      gid_end = memchr(gid_start, ':', line_end - gid_start);
      if(!gid_end)
        gid_end = line_end;
      if(gid_end == gid_start || gid_end - gid_start >= sizeof(gid_text))
      {
        ret = -EINVAL;
        goto done;
      }
      for(char *digit = gid_start; digit < gid_end; digit++)
        if(*digit < '0' || *digit > '9')
        {
          ret = -EINVAL;
          goto done;
        }
      memcpy(gid_text, gid_start, gid_end - gid_start);
      gid_text[gid_end - gid_start] = '\0';
      if(kstrtou32(gid_text, 10, &gid))
      {
        ret = -EINVAL;
        goto done;
      }
      *target_gid = make_kgid(&init_user_ns, gid);
      ret = gid_valid(*target_gid) ? 0 : -EINVAL;
      goto done;
    }

    line = line_end + (line_end < data + file_size);
  }

done:
  kvfree(buf);
  return ret;
}

int lce_proc_open(struct inode *inode, struct file *file)
{
  kgid_t target_gid;

  if(lce_populate_limeade_gid(&target_gid) < 0)
    return -EACCES;

#ifdef CONFIG_MULTIUSER
  if(!in_group_p(target_gid))
    return -EACCES;
#else
  return -EACCES;
#endif

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

// TODO I think partial lines can be output, which is bad
ssize_t lce_proc_read(struct file *file, char __user *ubuf, size_t count,
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

    arg_sanatize(ev.arg1);
    arg_sanatize(ev.arg2);

    // null bytes are copied so that the userspace program can use the data as
    // strings directly without copying the data
    len = snprintf(line, sizeof(line), "%llu\t%d\t%i\t%s%c\t%s%c\t%i\n", ev.ts,
                   ev.pid, (int)ev.type, ev.arg1, '\0', ev.arg2, '\0', ev.ret);

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
  //pr_info("lce: procfile read was successful (sent %ld bytes to process %i)", total, current->pid);
  return total;
}
