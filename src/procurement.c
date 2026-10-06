// procurement.c

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "logging.h"

int get_os_release(char **out)
{
  /* Reads Linux Distribution name
   * returns -1 on error
   */

  // 512 is an arbitrary choice on my end, my intance of this file id 389B
  char data[512];
  void *line_start, *line_end, *equals;
  int amt, amt_rem, line_len, key_len, val_len;

  FILE *fd = fopen("/etc/os-release", "r");
  if(!fd)
    return -1;
  
  amt = fread(data, 1, sizeof(data), fd);
  fclose(fd);

  if(amt <= 0)
    return -1;

  line_start = data;
  amt_rem = amt;

  for(;;)
  {
    line_end = memchr(line_start, '\n', amt_rem);
    if(!line_end)
      return -1;

    line_len = (line_end - line_start);

    equals = memchr(line_start, '=', line_len);
    if(!equals)
      return -1;

    key_len = equals - line_start;

    if(memcmp(line_start, "PRETTY_NAME", key_len) == 0)
    {
      equals += 2; // jump over '="'
      val_len = line_end - equals - 1;
      *out = malloc(val_len + 1);
      memcpy(*out, equals, val_len);
      *out[val_len] = '\0';
      return 0;
    }

    line_start = ++line_end;
    amt_rem -= (line_len + 1);
  }
}

struct mem_info
{
  long long ram_total_mbs;
  long long swap_total_mbs;
};

int get_mem_info(struct mem_info *out)
{
  /* Gets memory information
   * returns -1 on error
   */

  char target1[] = "MemTotal";
  char target2[] = "SwapTotal";
  
  // 2048 is an arbitrary choice on my end, my instance of this file is 1671B
  char data[2048];

  void *line_start, *line_end, *val_start;
  int amt, amt_rem, line_len;

  out->ram_total_mbs  = -1;
  out->swap_total_mbs = -1;

  FILE *fd = fopen("/proc/meminfo", "r");
  if(!fd)
    return -1;

  amt = fread(data, 1, sizeof(data), fd);
  fclose(fd);

  if(amt <= 0)
    return -1;

  line_start = data;
  amt_rem = amt;

  for(;;)
  {
    line_end = memchr(line_start, '\n', amt_rem);
    if(!line_end)
      return -1;

    line_len = (line_end - line_start);

    if(sizeof(target1) < line_len
    && memcmp(line_start, target1, sizeof(target1) - 1) == 0)
    {
      val_start = line_start;

      while(*(char*)val_start != ' ')
        val_start++;

      while(*(char*)val_start == ' ')
        val_start++;

      char *endptr;

      out->ram_total_mbs = strtol(val_start, &endptr, 10);

      if(endptr == val_start || out->ram_total_mbs == 0)
        return -1;

      out->ram_total_mbs /= 1024;

      if(out->swap_total_mbs != -1)
        return 0;

    } else if(sizeof(target2) < line_len
           && memcmp(line_start, target2, sizeof(target2) - 1) == 0)
    {
      val_start = line_start;

      while(*(char*)val_start != ' ')
        val_start++;

      while(*(char*)val_start == ' ')
        val_start++;

      char *endptr;

      out->swap_total_mbs = strtol(val_start, &endptr, 10);

      if(endptr == val_start || out->swap_total_mbs == 0)
        return -1;

      out->swap_total_mbs /= 1024;

      if(out->ram_total_mbs != -1)
        return 0;
    }

    line_start = ++line_end;
    amt_rem -= line_len + 1;
  }
}

struct cpu_info
{
  char *vendor_id;
  char *model_name;
};

int get_cpu_info(struct cpu_info *out)
{
  /* Gets CPU information
   * return -1 on error
   */

  char target1[] = "vendor_id";
  char target2[] = "model name";

  // while this file is quite large, the necessary data should be relatively early
  char data[2048];

  void *line_start, *line_end,  *val_start;
  int amt_rem, line_len;

  out->vendor_id  = NULL;
  out->model_name = NULL;

  FILE *fd = fopen("/proc/cpuinfo", "r");
  if(!fd)
    return -1;

  amt_rem = fread(data, 1, sizeof(data), fd);
  fclose(fd);

  if(amt_rem <= 0)
    return -1;

  line_start = data;

  for(;;)
  {
    line_end = memchr(line_start, '\n', amt_rem);
    if(!line_end)
    {
      // free does its own NULL checks
      free(out->vendor_id);
      free(out->model_name);
      return -1;
    }

    line_len = (line_end - line_start);

    if(sizeof(target1) < line_len
    && memcmp(line_start, target1, sizeof(target1) - 1) == 0)
    {
      val_start = line_end;
      int sz = 0;

      while(*(char*)val_start != ':')
      {
        val_start--;
        sz++;
      }

      val_start += 2; // jump over ": "
      sz -= 2;

      out->vendor_id = malloc(sz + 1);
      memcpy(out->vendor_id, val_start, sz);
      out->vendor_id[sz + 1] = '\0';

      if(out->model_name)
        return 0;

    } else if(sizeof(target2) < line_len
           && memcmp(line_start, target2, sizeof(target2) - 1) == 0)
    {
      val_start = line_end;
      int sz = 0;

      while(*(char*)val_start != ':')
      {
        val_start--;
        sz++;
      }

      val_start += 2;
      sz -= 2;

      out->model_name = malloc(sz + 1);
      memcpy(out->model_name, val_start, sz);
      out->model_name[sz + 1] = '\0';

      if(out->vendor_id)
        return 0;
    }

    line_start = ++line_end;
    amt_rem -= line_len + 1;
  }

}

int main()
{
  int ret;
  char *distro;
  ret = get_os_release(&distro);
  printf("%i\n", ret);
  if(ret == 0)
    printf("%s\n", distro);
  free(distro);

  struct mem_info m;

  ret = get_mem_info(&m);
  printf("%i\n", ret);
  if(ret == 0)
    printf("%lld\t%lld\n", m.ram_total_mbs, m.swap_total_mbs);

  struct cpu_info c;

  ret = get_cpu_info(&c);
  printf("%i\n", ret);

  if(ret == 0)
    printf("%s\n%s\n", c.model_name, c.vendor_id);

  free(c.model_name);
  free(c.vendor_id);


}

