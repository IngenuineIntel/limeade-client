// th_proc.c

#include<errno.h>
#include<dirent.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<unistd.h>

#include<liblimeade/liblimeade.h>
#include "logging.h"
#include "thread_pass.h"

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

  struct
  {
    uint64_t starttime;
    uint64_t utime;
    uint64_t stime;
  } prev;

  struct
  {
    uint64_t starttime;
    uint64_t utime;
    uint64_t stime;
  } new;

  uint32_t ram_kb;
  uint32_t swap_kb;

  char state;
  char name[32];
  char command[72];
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
  struct proc_data data;
};

struct proc_entry *new_proc_entry(void)
{
  struct proc_entry *ret = malloc(sizeof(*ret));
  memset(ret, 0, sizeof(*ret));

  return ret;
}

/* iterates over every entry */
#define FOR_EVERY_PROC_ENTRY(head, i)\
  for(struct proc_entry *i = head; i; i = i->next)

/* iterates over every entry with a declared variable */
#define FOR_EVERY_PROC_ENTRY_PREDEF(head, i)\
    for(i = head; i; i = i->next)

/* iterates over every entry above the provided one */
#define FOR_EVERY_PROC_ENTRY_ABOVE(node, i)\
  FOR_EVERY_PROC_ENTRY(node->next, i)

/* iterates over every entry below the provided one */
#define FOR_EVERY_PROC_ENTRY_BELOW(node, i)\
  for(struct proc_entry *i = node; i; i = i->prev)

/* removes a node from the sorting data of the list */
#define POP_SORTING_PROC_ENTRY(node)\
  if(node->sorting_prev) node->sorting_prev->sorting_next = node->sorting_next;\
  if(node->sorting_next) node->sorting_next->sorting_prev = node->sorting_prev;

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

inline static void pop_proc_entry(struct proc_entry *node)
{
  /* removes a node */
  if(node->prev)
    node->prev->next = node->next;
  if(node->next)
    node->next->prev = node->prev;
}

static void free_proc_entries(struct proc_entry *head)
{
  while(head)
  {
    struct proc_entry *next = head->next;
    free(head);
    head = next;
  }
}

// keys to match while parsing /proc/[PID]/status
static const char PROC_STATE_TARGET[]   = "State:";
static const char PROC_PPID_TARGET[]    = "PPid:";
static const char PROC_UID_TARGET[]     = "Uid:";
static const char PROC_GID_TARGET[]     = "Gid:";
static const char PROC_VMSIZE_TARGET[]  = "VmSize:";
static const char PROC_VMSWAP_TARGET[]  = "VmSwap:";
static const char PROC_THREAD_TARGET[]  = "Threads:";

#define MATCH_COND(target)\
  sizeof(target) < line_len\
  && memcmp(line_start, target, sizeof(target) - 1) == 0

#define INT_CAST(dst, src)\
  {\
    dst = strtol(src, &endptr, 10);\
    if(endptr == src)\
    {\
      log_dbg3("failed strtol in parsing (next character was %02x", *src);\
      return -1;\
    }}

#define CHECK_AND_CAST_INT(indicator, match, dst, src)\
  if(!indicator && MATCH_COND(match))\
  {\
    src += sizeof(match);\
    INT_CAST(dst, src);\
    indicator = 1;\
  }  

static int populate_proc_data_status(struct proc_data *data, pid_t pid)
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
  uint8_t got_state, got_ppid, got_uids, got_gids, got_ram, got_swap, got_threads;
  got_state = got_ppid = got_uids = got_gids = got_ram = \
  got_swap = got_threads = 0;

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
    {
      // kernel processes won't have any of the other fields, so not finding
      // them isn't inherently unusual
      if(!got_threads || !got_uids || !got_gids)
        return -1;
      else return 0;
    }

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
      idx = endptr + 1;
      INT_CAST(data->euid, idx);
      idx = endptr + 1;
      INT_CAST(data->suid, idx);
      idx = endptr + 1;
      INT_CAST(data->fsuid, idx);
      got_uids = 1;
    }
    else if(!got_gids && MATCH_COND(PROC_GID_TARGET))
    {
      idx += sizeof(PROC_GID_TARGET) - 1;
      INT_CAST(data->rgid, idx);
      idx = endptr + 1;
      INT_CAST(data->egid, idx);
      idx = endptr + 1;
      INT_CAST(data->sgid, idx);
      idx = endptr + 1;
      INT_CAST(data->fsgid, idx);
      got_gids = 1;
    }
    else CHECK_AND_CAST_INT(got_ram, PROC_VMSIZE_TARGET, data->ram_kb, idx)
    else CHECK_AND_CAST_INT(got_swap, PROC_VMSWAP_TARGET, data->swap_kb, idx)
    else CHECK_AND_CAST_INT(got_threads, PROC_THREAD_TARGET, data->threads, idx)
    else
      goto next;

    // checks are in reverse order to how they're expected to be read to attempt
    // to limit unecessary checks
    if(got_threads && got_ram && got_gids  && got_uids && got_state)
      return 0;

    next:

    line_start = line_end + 1;
    rem       -= line_len + 1;
  }
}

#undef MATCH_COND
#undef INT_CAST
#undef CHECK_AND_CAST_INT

static int stat_skip_vals(char **ptr, const char * const ptr_max, uint8_t nr_vals)
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
  if(stat_skip_vals(&idx, max, nr) < 0) return -1

#define CAST(dst)\
  dst = strtoll(idx, &endptr, 10);\
  if(endptr == idx) return -1

static int populate_proc_data_stat(struct proc_data *data, pid_t pid)
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

  data->prev.utime = data->new.utime;
  data->prev.stime = data->new.stime;
  data->prev.starttime = data->new.starttime;

  CAST(data->new.utime);
  SKIP(1);
  CAST(data->new.stime);
  SKIP(7);
  CAST(data->new.starttime);

  return 0;
}

#undef CAST
#undef SKIP

static int populate_proc_data_name(struct proc_data *data, pid_t pid)
{
  /* populates data->name from /proc/pid/comm
   */
  char path[35];
  FILE *fd;
  int len;

  snprintf(path, sizeof(path), "/proc/%i/comm", pid);

  fd = fopen(path, "r");
  if(!fd)
    return -1;

  len = fread(data->name, 1, sizeof(data->name) - 1, fd);
  fclose(fd);
  if(len < 0)
    return -1;

  data->name[len] = '\0';

  return 0;
}

static int populate_proc_data_command(struct proc_data *data, pid_t pid)
{
  /* populates data->command from /proc/pid/cmdline
   */
  char path[40];
  FILE *fd;
  int len = 0;

  snprintf(path, sizeof(path), "/proc/%i/comm", pid);

  fd = fopen(path, "r");
  if(!fd)
    return -1;

  len = fread(data->command, 1, sizeof(data->command) - 1, fd);
  fclose(fd);
  if(len < 0)
    return 0;
  
  // these files separate argument with null bytes, so they must be sanatized
  // I really can't imagine I can use SIMD for this
  register char *p = data->command;
  for(int i = 0; i < len; i++)
    if(p[i] == '\0')
      p[i] = ' ';
  p[len - 1] = '\0';

  return 0;
}

static int populate_proc_data(struct proc_data *data, pid_t pid)
{
  /* populates `data` via the /proc/`pid` directory listings
   * 0 on success, -1 on error
   */
  int ret;

  if((ret = populate_proc_data_name(data, pid)) < 0)
  {
    log_crit("name population failed");
    return ret;
  }

  if((ret = populate_proc_data_command(data, pid)) < 0)
  {
    log_crit("command population failed");
    return ret;
  }

  if((ret = populate_proc_data_status(data, pid)) < 0)
  {
    log_crit("status population failed");
    return ret;
  }

  if((ret = populate_proc_data_stat(data, pid)) < 0)
  {
    log_crit("stat population failed");
    return ret;
  }

  return 0;
}

static int init_proc_entries(struct proc_entry **out)
{
  /* creates and initializes a data structure of proc_entry
   * -1 on error, the number of elements in the list on success
   */
  pid_t pid;
  int ret = 0;
  char *endptr;
  struct proc_entry mock_head, *prev, *cur = &mock_head;
  DIR *dfd = opendir("/proc");
  if(!dfd)
    return -1;

  for(struct dirent *entry = readdir(dfd); entry; entry = readdir(dfd), ret++)
  {
    pid = strtol(entry->d_name, &endptr, 10);
    if(endptr == entry->d_name)
      continue;

    prev = cur;
    cur  = new_proc_entry();
    cur->prev  = prev;
    prev->next = cur;
    cur->data.pid = pid;

    if(populate_proc_data(&cur->data, pid) >= 0)
      continue;

    iter_skip:
    
    free(cur);
    cur = prev;
    prev = cur->prev;
    ret--;
  }

  *out = mock_head.next;

  return ret;
}

static int update_proc_entries(struct proc_entry *head)
{
  /* updates data in proc_entry data structure
   * returns the number of processes on success, & -1 on error
   */
  pid_t pid;
  int nr_pids = 0;
  char *endptr;
  struct proc_entry *node;

  DIR *dfd = opendir("/proc");
  if(!dfd)
    return -1;

  log_dbg3("preparing sorting links");
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

    FOR_EVERY_PROC_ENTRY_PREDEF(head, node)
    {
      if(node->data.pid != pid)
        continue;

      if(populate_proc_data(&node->data, pid) < 0)
      {
        log_dbg3("failed to populate data for process %i, assuming dead", pid);
        goto proc_died;
      }

      POP_SORTING_PROC_ENTRY(node);

      goto next_search;

      proc_died:

      log_dbg3("%p", node);
      pop_proc_entry(node);
      log_dbg3("survived popping");
      free(node);
      log_dbg3("survived freeing");
      nr_pids--;

      goto next_search;
    }

    node = append_proc_entry(head);

    if(populate_proc_data(&node->data, pid) < 0)
    {
      log_dbg3(
          "attempted to add a new proc entry, but failed to populate it: %i",
          pid);
      pop_proc_entry(node);
      free(node);
    }

    next_search:
    nr_pids++;
  }
  return nr_pids;
}

// predicted number of processes that need to be allocated for
// changes over time to be more accurate
static int PREDICTED_PROC_NR = 300;

// a constant grace buffer for the prediction
static const int PREDICTION_LINEAR_BIAS = 50;

struct proc_tbl_head
{
  /* metadata for indiv_proc hashtable */
  int count;
  int capacity;
  int incr_inc;
};

struct limeade_indiv_proc *proc_tbl_init(void)
{
  /* initializes indiv_proc hashtable */
  struct proc_tbl_head *ret;
  ret = malloc(sizeof(*ret) + sizeof(struct limeade_indiv_proc) * PREDICTED_PROC_NR);
  ret->count    = 0;
  ret->capacity = 0;
  ret->incr_inc = PREDICTED_PROC_NR / 2;
  return (struct limeade_indiv_proc*)(ret + 1);
}

inline struct limeade_indiv_proc *proc_tbl_append(struct limeade_indiv_proc **tbl)
{
  /* returns pointer to next element of indiv_proc_hashtable */
  struct proc_tbl_head *metadata = ((typeof(metadata))*tbl - 1);
  metadata->count++;
  if(metadata->count > metadata->capacity)
  {
    metadata = realloc(metadata, sizeof(**tbl) * (metadata->count + metadata->incr_inc)
                               + sizeof(*metadata));
    metadata->capacity += metadata->incr_inc;
    *tbl = (typeof(*tbl))(metadata + 1);
  }
  return *tbl + metadata->count;
}

/* creates hashtable */
#define PROC_TBL_INIT(t) struct limeade_indiv_proc *t = proc_tbl_init()

/* gets hashtable size */
#define proc_tbl_sz(tbl)   (((struct proc_tbl_head*)tbl - 1)->count)

/* frees hashtable */
#define proc_tbl_free(tbl) (free((struct proc_tbl_head*)tbl - 1))

/* updates hashtable size prediction */
#define UPDATE_TBL_SZ_PREDICT(t)\
  PREDICTED_PROC_NR = ((PREDICTED_PROC_NR + proc_tbl_sz(t)) / 2) + PREDICTION_LINEAR_BIAS

#define CP_ATTR(node, indiv, attr)\
  indiv->attr = node->data.attr

#define DELTA_FROM_NODE(node, attr)\
  (node->data.new.attr - node->data.prev.attr)

static int generic_from_entries(struct proc_entry *head, struct limeade_proc_generic *out)
{
  /* generates LIMEADE_PACKET_PROC_GENERIC from process data
   * note: the strings pointed to by the packet structure are the ones in the
   * linked list; they are not copied */
  
  int64_t d_starttime, d_utime, d_stime;
  PROC_TBL_INIT(t);
  struct limeade_indiv_proc *cur;

  {
    struct timespec rn;
    clock_gettime(CLOCK_REALTIME, &rn);

    out->ts_s  = rn.tv_sec;
    out->ts_ms = rn.tv_nsec / 1000000;
  }

  FOR_EVERY_PROC_ENTRY(head, node)
  {
    cur = proc_tbl_append(&t);
    CP_ATTR(node, cur, pid);
    CP_ATTR(node, cur, ppid);
    CP_ATTR(node, cur, ruid);
    CP_ATTR(node, cur, euid);
    CP_ATTR(node, cur, suid);
    CP_ATTR(node, cur, fsuid);
    CP_ATTR(node, cur, rgid);
    CP_ATTR(node, cur, egid);
    CP_ATTR(node, cur, sgid);
    CP_ATTR(node, cur, fsgid);
    CP_ATTR(node, cur, threads);
    CP_ATTR(node, cur, name);
    CP_ATTR(node, cur, command);
    CP_ATTR(node, cur, ram_kb);
    CP_ATTR(node, cur, swap_kb);

    d_starttime = DELTA_FROM_NODE(node, starttime);
    d_utime     = DELTA_FROM_NODE(node, utime);
    d_stime     = DELTA_FROM_NODE(node, stime);

    if(d_starttime <= 0) // maybe a time change would cause this?
    {
      log_dbg2(
          "timestamp delta came back negative, skipping CPU stats for process %i",
          cur->pid);
      continue;
    }
    
    cur->cpu_user = d_utime > 0 ? (d_utime * 100) / (d_starttime * 100) : 0;
    cur->cpu_sys  = d_stime > 0 ? (d_stime * 100) / (d_starttime * 100) : 0;
  }

  out->procs = t;
  out->total = proc_tbl_sz(t);

  UPDATE_TBL_SZ_PREDICT(t);

  return 0;
}

#undef CP_ATTR
#undef DELTA_FROM_NODE

#define generic_release(p) proc_tbl_free(out->procs)

void *th_proc(struct thread_pass *glbl_data)
{
  struct timespec goal_wait, iter_start, iter_end, diff, real_wait;

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
          "process thread took longer than the allocated time (%llins when the goal was %llins)",
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

int main(void)
{
  struct timespec init_start, init_stop, populate_start, populate_stop;
  struct timespec print_start, print_stop, iter_start, iter_stop;
  struct proc_entry *head;
  int nr;

  struct timespec rqtp = {0, 100000000}, rmtp;

  logging_init();
  enable_really_verbose_logging();

  log_warn("testing...");

  clock_gettime(CLOCK_MONOTONIC, &init_start);
  nr = init_proc_entries(&head);
  clock_gettime(CLOCK_MONOTONIC, &init_stop);
  printf("init_proc_entries() -> %d\n", nr);

  nanosleep(&rqtp, &rmtp);

  clock_gettime(CLOCK_MONOTONIC, &populate_start);
  nr = update_proc_entries(head);
  clock_gettime(CLOCK_MONOTONIC, &populate_stop);


  clock_gettime(CLOCK_MONOTONIC, &print_start);
  FOR_EVERY_PROC_ENTRY(head, node)
  {
    log_dbg("pid=%d ppid=%d state=%c threads=%u utime=%llu stime=%llu cmd=%s",
        node->data.pid, node->data.ppid, node->data.state,
        node->data.threads, node->data.new.utime, node->data.new.stime, node->data.command);
  }
  clock_gettime(CLOCK_MONOTONIC, &print_stop);

  log_info("init took %lluus", (init_stop.tv_nsec - init_start.tv_nsec)/1000);
  log_info("update took %llums", (populate_stop.tv_nsec - populate_start.tv_nsec)/1000000);
  log_info("printing took %lluus", (print_stop.tv_nsec - print_start.tv_nsec)/1000);

  uint64_t total_procs = 0;

  clock_gettime(CLOCK_MONOTONIC, &iter_start);
  FOR_EVERY_PROC_ENTRY(head, node)
    total_procs ++;
  clock_gettime(CLOCK_MONOTONIC, &iter_stop);


  log_info("total data size: %llu kB for %llu processes",
           (total_procs * sizeof(*head))/1024, total_procs);
  log_info("time it took to iterate: %lluus",
           (iter_stop.tv_nsec - iter_start.tv_nsec)/1000);

  free_proc_entries(head);

  return 0;
}

