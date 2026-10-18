// th_proc.c

#include<errno.h>
#include<dirent.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>

#include<liblimeade/liblimeade.h>
#include "logging.h"
#include "thread_pass.h"

// TODO make only the necessary data duplicated
struct proc_data
{
  /* data about a certain process in its purest form
   * much of this data has to be processed in order to ascertain useful data
   * from it
   */
  pid_t pid;
  pid_t ppid;

  uid_t ruid;
  uid_t euid;
  uid_t suid;
  uid_t fsuid;

  gid_t rgid;
  gid_t egid;
  gid_t sgid;
  gid_t fsgid;

  uint32_t threads;
  uint64_t starttime;
  uint64_t utime;
  uint64_t stime;

  uint32_t vmsize;
  uint32_t vmrss;
  uint32_t rssfile;
  uint32_t rssmem;

  char state;
  char command[50];
};

struct proc_entry
{
  /* double linked list for storage of process data
   * contains the latest two iterations of data acquisition about a process
   */
  struct proc_entry *next;
  struct proc_entry *prev;
  struct proc_entry *sorting_next;
  struct proc_entry *sorting_prev;
  uint16_t idx;

  struct proc_data d1;
  struct proc_data d2;

  struct proc_data *prev_data;
  struct proc_data *next_data;
};

struct proc_entry *new_proc_entry(void)
{
  struct proc_entry *ret = malloc(sizeof(*ret));

  ret->prev_data = &ret->d1;
  ret->next_data = &ret->d2;

  ret->next = ret->prev = NULL;
  ret->sorting_next = ret->sorting_prev = NULL;

  return ret;
}

/* iterates over every entry */
#define FOR_EVERY_PROC_ENTRY(head, i)\
  for(struct proc_entry *i = head; i; i = i->next)

/* iterates over every entry, leaving the last entry at `last` */
#define FOR_EVERY_PROC_ENTRY_LAST(head, i, last)\
    for(struct proc_entry *i = head; i; last = i, i = i->next)

/* iterates over every entry above the provided one */
#define FOR_EVERY_PROC_ENTRY_ABOVE(node, i)\
  FOR_EVERY_PROC_ENTRY(node->next, i)

/* iterates over every entry below the provided one */
#define FOR_EVERY_PROC_ENTRY_BELOW(node, i)\
  for(struct proc_entry *i = node; i; i = i->prev)

/* removes a node from the sorting data of the list */
#define POP_SORTING_PROC_ENTRY(node)\
  if(node->sorting_prev) node->sorting_prev->sorting_next = node->sorting_next;\
  if(node->sorting_prev) node->sorting_next->sorting_prev = node->sorting_prev;

struct proc_entry *append_proc_entry(struct proc_entry *node)
{
  /* appends an entry to the end of a list of `proc_entry`s.
   * takes any node of the list as an argument
   * the latter the node, the better the performance
   */
  struct proc_entry *i;
  for(i = node; i->next; i = i->next);
  i->next = new_proc_entry();
  i->next->prev = i;
  return i->next;
}

inline void pop_proc_entry(struct proc_entry *node)
{
  /* removes a node */
  if(node->prev)
    node->prev->next = node->next;
  if(node->next)
    node->next->prev = node->prev;
}

int index_proc_entries(struct proc_entry *head)
{
  /* populates double linked list with index values */
  uint16_t ret = 0;
  for(struct proc_entry *i = head; i; i = i->next, ret++)
    i->idx = ret;
  return ret;
}

struct proc_entry *pop_proc_entry_idx(uint16_t idx, struct proc_entry *node)
{
  /* removes a node from a double linked list and returns it
   * only should be used on indexed lists
   * any node of the list can be used, however, the closer the provided node is
   * to the targeted node, the better the performance will be
   * doesn't free any memory
   * if the index isn't found, NULL is returned
   */
  if(node->idx < idx)
    for(struct proc_entry *i = node; i; i = i->next)
      if(i->idx == idx)
        return i;
  if(node->idx > idx)
    for(struct proc_entry *i = node; i; i = i->prev)
      if(i->idx == idx)
        return i;
  return NULL;
}

void free_proc_entries(struct proc_entry *node)
{
  /* frees double linked list
   * any node can be provided as an argument */
  struct proc_entry *i, *j;
  for(j = node, i = j->next; i; j = i, i = i->next)
    free(j);
  free(j);
  for(j = node, i = j->prev; i; j = i, i = i->prev)
    free(j);
  free(j);
  free(j);
}

// keys to match while parsing /proc/[PID]/status
static const char PROC_STATE_TARGET[]   = "State:";
static const char PROC_PPID_TARGET[]    = "PPid:";
static const char PROC_UID_TARGET[]     = "Uid:";
static const char PROC_GID_TARGET[]     = "Gid:";
static const char PROC_VMSIZE_TARGET[]  = "VmSize:";
static const char PROC_VMRSS_TARGET[]   = "VmRSS:";
static const char PROC_RSSFILE_TARGET[] = "RssFile:";
static const char PROC_RSSMEM_TARGET[]  = "RssShmem:";
static const char PROC_THREAD_TARGET[]  = "Threads:";

#define MATCH_COND(target)\
  sizeof(target) < line_len\
  && memcmp(line_start, target, sizeof(target) - 1) == 0

#define INT_CAST(dst, src)\
  dst = strtol(src, &endptr, 10);\
  if(endptr == src)\
  {\
    log_dbg2("failed strtol (%02x instead of integer)", *src);\
    return -1;\
  }

#define CHECK_AND_CAST_INT(indicator, match, dst, src)\
  if(!indicator && MATCH_COND(match))\
  {\
    src += sizeof(match);\
    INT_CAST(dst, src);\
    indicator = 1;\
  }

int populate_proc_data_status(struct proc_data *data, pid_t pid)
{
  /* populates from following values form /proc/pid/status into `data`:
   * state, ppid, *uid, *gid, vmsize, vmrss, rssfile, rssmem, & num_thread
   * 0 on success, -1 on error
   */
  FILE *fd;
  char buffer[4096];
  char path[25];
  char *line_start, *line_end, *idx, *endptr;
  int amt, rem, line_len;
  uint8_t got_state, got_ppid, got_uids, got_gids, got_vmsize;
  uint8_t got_vmrss, got_rssfile, got_rssmem, got_threads;

  got_state = got_ppid = got_uids = got_gids = got_vmsize = 0;
  got_vmrss = got_rssfile = got_rssmem = got_threads = 0;

  snprintf(path, sizeof(path), "/proc/%i/status", pid);
  fd = fopen(path, "r");

  if(!fd)
    return -1;

  amt = fread(buffer, 1, sizeof(buffer) - 1, fd);
  fclose(fd);

  if(amt <= 0)
    return -1;

  buffer[amt] = '\0';

  rem = amt;
  line_start = buffer;

  for(;;)
  {
    line_end = memchr(line_start, '\n', rem);
    if(!line_end)
      return -1;

    line_len = (line_end - line_start);
    idx = line_start;

    if(!got_state && MATCH_COND(PROC_STATE_TARGET))
    {
      idx += sizeof(PROC_STATE_TARGET);
      data->state = *idx;
      got_state = 1;
    }
    else CHECK_AND_CAST_INT(got_ppid, PROC_PPID_TARGET, data->ppid, idx)
    else if(!got_uids && MATCH_COND(PROC_UID_TARGET))
    {
      idx += sizeof(PROC_UID_TARGET);
      INT_CAST(data->ruid, idx);
      endptr++;
      INT_CAST(data->euid, endptr);
      endptr++;
      INT_CAST(data->suid, endptr);
      endptr++;
      INT_CAST(data->fsuid, endptr);
      got_uids = 1;
    }
    else if(!got_gids && MATCH_COND(PROC_GID_TARGET))
    {
      idx += sizeof(PROC_GID_TARGET);
      INT_CAST(data->rgid, idx);
      endptr++;
      INT_CAST(data->egid, endptr);
      endptr++;
      INT_CAST(data->sgid, endptr);
      endptr++;
      INT_CAST(data->fsgid, endptr);
      got_gids = 1;
    }
    else CHECK_AND_CAST_INT(got_vmsize, PROC_VMSIZE_TARGET, data->vmsize, idx)
    else CHECK_AND_CAST_INT(got_vmrss, PROC_VMRSS_TARGET, data->vmrss, idx)
    else CHECK_AND_CAST_INT(got_rssfile, PROC_RSSFILE_TARGET, data->rssfile, idx)
    else CHECK_AND_CAST_INT(got_rssmem, PROC_RSSMEM_TARGET, data->rssmem, idx)
    else CHECK_AND_CAST_INT(got_threads, PROC_THREAD_TARGET, data->threads, idx)
    else goto next;

    // checks are in reverse order to how they're expected to be read to attempt
    // to limit unecessary checks
    if(got_threads && got_rssmem && got_rssfile && got_vmrss
    && got_vmsize  && got_gids   && got_uids    && got_state)
      return 0;

    next:

    line_start = line_end + 1;
    rem       -= line_len + 1;
  }
}

#undef MATCH_COND
#undef INT_CAST

int stat_skip_vals(char **ptr, const char * const ptr_max, uint8_t nr_vals)
{
  /* skips values when reading /proc/[PID]/stat 
   * returned -1 if the end of the file data is reached
   * note: this couldn't benefit from MMX or SSE SIMD
   */
  char *j = *ptr;
  for(int i = 0; i < nr_vals; i++)
  {
    for(; *j != ' ' && j < ptr_max; j++);
    for(; *j == ' ' && j < ptr_max; j++);
    j++;
  }
  return (*ptr = j) >= ptr_max ? -1 : 0;
}

#define SKIP(nr)\
  if(stat_skip_vals(&idx, max, 13) < 0) return -1

#define CAST(dst)\
  dst = strtoll(idx, &endptr, 10);\
  if(endptr == idx) return -1

int populate_proc_data_stat(struct proc_data *data, pid_t pid)
{
  /* populates data with utime, stime, & starttime found in /proc/pid/stat
   * 0 on success, -1 on error */
  FILE *fd;
  char buffer[1024], path[25], *idx, *max, *endptr;
  int amt;
  
  snprintf(path, sizeof(path), "/proc/%i/stat", pid);
  fd = fopen(path, "r");

  if(!fd)
    return -1;

  amt = fread(buffer, 1, sizeof(buffer) - 1, fd);
  fclose(fd);

  if(amt <= 0)
    return -1;

  buffer[amt] = '\0';
  idx = buffer;
  max = idx + amt;

  // data we need:
  // utime is the 14th value
  // stime is the 15th value
  // starttime is the 22nd value
  // all values are separated by a space/spaces
  // `man 5 proc_pid_stat`
  if(stat_skip_vals(&idx, max, 13) < 0)
    return -1;

  data->utime = strtoll(idx, &endptr, 10);
  if(endptr == idx) return -1;

  SKIP(13);
  CAST(data->utime);
  SKIP(1);
  CAST(data->stime);
  SKIP(7);
  CAST(data->starttime);

  return 0;
}

int populate_proc_data_command(struct proc_data *data, pid_t pid)
{
  /* populates data->command from /proc/pid/comm & /proc/pid/cmdline
   * 0 on success, -1 on error
   */
  char path[35];
  FILE *fd;
  int len;

  snprintf(path, sizeof(path), "/proc/%i/comm", pid);

  fd = fopen(path, "r");
  if(!fd)
    return -1;

  len = fread(data->command, 1, sizeof(data->command) - 1, fd);
  fclose(fd);
  if(len <= 0)
    return -1;

  snprintf(path, sizeof(path), "/proc/%i/cmdline", pid);

  fd = fopen(path, "r");
  if(!fd)
    return -1;
  
  len += fread(data->command + len, 1, (sizeof(data->command) - 1) - len, fd);
  fclose(fd);

  data->command[len] = '\0';

  return 0;
}

int populate_proc_data(struct proc_data *data, pid_t pid)
{
  /* populates `data` via the /proc/`pid` directory listings
   * 0 on success, -1 on error
   */
  register int ret;

  if((ret = populate_proc_data_command(data, pid)) < 0)
    return ret;

  if((ret = populate_proc_data_status(data, pid)) < 0)
    return ret;

  if((ret = populate_proc_data_stat(data, pid)) < 0)
    return ret;

  return 0;
}

#define POPULATE_PROC_DATA(data, pid)\
  (populate_proc_data_command(data, pid) >= 0\
  && populate_proc_data_status(data, pid) >= 0\
  && populate_proc_data_stat(data, pid)   >= 0)

int init_proc_entries(struct proc_entry *out)
{
  /* creates and initializes a data structure of proc_entry
   * -1 on error, the number of elements in the list on success
   */
  pid_t pid;
  char *endptr;
  struct proc_entry mock_head, *prev, *cur = &mock_head;
  DIR *dfd = opendir("/proc");
  if(!dfd)
    return -1;

  for(struct dirent *entry = readdir(dfd); entry; entry = readdir(dfd))
  {
    pid = strtol(entry->d_name, &endptr, 10);
    if(endptr == entry->d_name)
      continue;

    prev = cur;
    cur  = new_proc_entry();
    cur->prev  = prev;
    prev->next = cur;

    if(populate_proc_data_command(cur->next_data, pid) < 0)
      goto iter_skip;

    if(populate_proc_data_status(cur->next_data, pid) < 0)
      goto iter_skip;

    if(populate_proc_data_stat(cur->next_data, pid) == 0)
      continue;

    iter_skip:
    
    free(cur);
    cur = prev;
    prev = cur->prev;
  }

  out = mock_head.next;

  return index_proc_entries(out);

err:
  return -1;
}

int update_proc_entries(struct proc_entry *head)
{
  /* updates data in proc_entry data structure
   * returns the number of processes on success, & -1 on error
   */

  pid_t pid;
  int nr_pids = 0;
  char *endptr;
  struct proc_entry *last;
  struct proc_data *cur_data;
  DIR *dfd = opendir("/proc");
  if(!dfd)
  {
    log_warn("failed to open /proc: %s", strerror(errno));
    return -1;
  }

  FOR_EVERY_PROC_ENTRY(head, node)
  {
    node->sorting_prev = node->prev;
    node->sorting_next = node->next;
  }

  for(struct dirent *entry = readdir(dfd); entry; entry = readdir(dfd))
  {
    pid = strtol(entry->d_name, &endptr, 10);
    if(*endptr != '\0')
      continue;

    FOR_EVERY_PROC_ENTRY_LAST(head, node, last)
    {
      if(node->next_data->pid != pid)
        continue;

      cur_data = node->prev_data;
      node->prev_data = node->next_data;
      node->next_data = cur_data;

      if(!POPULATE_PROC_DATA(cur_data, pid))
      {
        log_dbg3("failed to populate data for process %i, assuming dead", pid);
        goto proc_died;
      }

      POP_SORTING_PROC_ENTRY(node);

      goto next_search;

      proc_died:

      pop_proc_entry(node);
      free(node);
      nr_pids--;
      goto next_search;
    }

    last->next = new_proc_entry();
    last->next->prev = last;
    last = last->next;
    cur_data = last->next_data;
    cur_data->pid = pid;

    if(!POPULATE_PROC_DATA(cur_data, pid))
    {
      log_dbg3(
          "attempted to add a new proc entry, but failed to populate it: %i",
          pid);
      pop_proc_entry(last);
      free(last);
    }
    else
      log_dbg3("new process added to proc entries: %i", pid);

    next_search:
    nr_pids++;
  }
  return nr_pids;
}

int generic_from_entries(struct proc_entry *head, struct limeade_proc_generic *out)
{
  /* generates LIMEADE_PACKET_PROC_GENERIC from process data */
  // TODO
  return 0;
}

int dynamic_from_entries(struct proc_entry *head, struct limeade_proc_update *out)
{
  /* generates LIMEADE_PACKET_PROC_UPDATE from process data */
  // TODO
  return 0;
}

void *th_proc(struct thread_pass *glbl_data)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;
  struct proc_entry *head;

  goal_wait.tv_sec  = 0;
  goal_wait.tv_nsec = 999999999/glbl_data->proc_hz;

  pthread_mutex_lock(&glbl_data->mtx_proc_err);
  log_dbg("th_proc ready");
  sem_wait(&glbl_data->sem_ready);
  while(sem_trywait(&glbl_data->sem_shutdown) != 0 && errno == EAGAIN)
  {
    clock_gettime(CLOCK_MONOTONIC, &iter_start);

    // TODO stuff

    
    iter_end:

    clock_gettime(CLOCK_MONOTONIC, &iter_end);

    diff.tv_sec  = iter_end.tv_sec  - iter_start.tv_sec;
    diff.tv_nsec = iter_end.tv_nsec - iter_start.tv_nsec;
    real_wait.tv_sec  = goal_wait.tv_sec  - diff.tv_sec;
    real_wait.tv_nsec = goal_wait.tv_nsec - diff.tv_nsec;
    if(real_wait.tv_nsec < 0) // carry
    {
      real_wait.tv_sec--;
      real_wait.tv_sec += 999999999;
    }

    if(real_wait.tv_sec < 0 || real_wait.tv_sec < 0)
    {
      log_warn(
          "process thread took longer than the allocated time (%llins when the goal was %llins",
          ((int64_t)diff.tv_sec)      * 999999999 + diff.tv_nsec,
          ((int64_t)goal_wait.tv_sec) * 999999999 + goal_wait.tv_nsec);
      continue;
    }
    
    nanosleep(&real_wait, &diff);
  }
  
err:

  pthread_mutex_unlock(&glbl_data->mtx_events_err);
  return NULL;
}

