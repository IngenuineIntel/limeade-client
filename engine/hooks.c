// hooks.c

#include "lce.h"

int lce_user_strcpy(const char __user *s, char *dst, int sz)
{
  int ret;

  if(!s || !dst || sz < 2)
    return -EINVAL;

  ret = strncpy_from_user(dst, s, sz - 1);
  if (ret <= 0)
    return -EFAULT;

  dst[ret] = '\x00';
  return 0;
}

static void lce_hashtbl_register(struct lce_event *ev)
{
  struct lce_event_pending *entry;
  unsigned long lock_flags;

  spin_lock_irqsave(&lce_hashtbl_lock, lock_flags);
  hash_for_each_possible(lce_hashtbl, entry, node, (u32)ev->pid)
  {
    if(entry->ev.pid == ev->pid)
    {
      entry->ev = *ev;
      spin_unlock_irqrestore(&lce_hashtbl_lock, lock_flags);
      return;
    }
  }
  // unlocking hash table because allocation might take a while
  spin_unlock_irqrestore(&lce_hashtbl_lock, lock_flags);

  entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
  if(!entry)
    return;

  entry->ev = *ev;

  spin_lock_irqsave(&lce_hashtbl_lock, lock_flags);
  hash_add(lce_hashtbl, &entry->node, (u32)ev->pid);
  spin_unlock_irqrestore(&lce_hashtbl_lock, lock_flags);
}

static bool lce_hashtbl_take(struct lce_event *out, pid_t match)
{
  struct lce_event_pending *entry;
  unsigned long lock_flags;

  spin_lock_irqsave(&lce_hashtbl_lock, lock_flags);
  hash_for_each_possible(lce_hashtbl, entry, node, (u32)match)
  {
    if(entry->ev.pid == match)
    {
      *out = entry->ev;
      hash_del(&entry->node);
      spin_unlock_irqrestore(&lce_hashtbl_lock, lock_flags);
      kfree(entry);
      return true;
    }
  }
  spin_unlock_irqrestore(&lce_hashtbl_lock, lock_flags);
  return false;
}

int lce_hook_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;
  unsigned long lock_flags;

  if(!lce_hashtbl_take(&ev, current->pid))
    return 0;

  ev.ret = regs_return_value(regs);

  spin_lock_irqsave(&lce_kfifo_lock, lock_flags);
  if(kfifo_is_full(&lce_kfifo))
  {
    struct lce_event discard;
    (void)kfifo_get(&lce_kfifo, &discard);
    pr_warn("LCE kfifo buffer was full, 1 event discarded");
  }
  
  kfifo_put(&lce_kfifo, ev);
  spin_unlock_irqrestore(&lce_kfifo_lock, lock_flags);

  return 0;
}

#define LCE_REGISTER(ev) lce_hashtbl_register(&ev)
#define LCE_PREP(ev, ev_type)\
  ev.ts = ktime_get_ns();\
  ev.pid = current->pid;\
  ev.type = ev_type;\

/*** the part with the hooks ***/

int lce_hook_open(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  const char __user *path = (const char __user*)regs->di;
  int flags               = (int)regs->si;

  if(lce_user_strcpy(path, ev.arg1, sizeof(ev.arg1)) < 0)
    return 0;

  snprintf(ev.arg2, sizeof(ev.arg2), "flags=%i", flags);

  LCE_PREP(ev, LCE_EVENT_OPEN);
  LCE_REGISTER(ev);
  return 0;
}


int lce_hook_openat(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;
  struct open_how how;

  int dirfd                = (int)regs->di;
  const char __user *path  = (const char __user*)regs->si;
  const void __user *how_u = (const char __user*)regs->dx;
  const void *how_k        = (const void*)regs->dx;

  if(lce_user_strcpy(path, ev.arg1, sizeof(ev.arg1)) < 0)
    goto end;

  if(how_u && copy_from_user(&how, how_u, sizeof(how)) != 0)
    if(how_k && copy_from_kernel_nofault(&how, how_k, sizeof(how)) != 0)
      goto end;

  snprintf(ev.arg2, sizeof(ev.arg2),
           "dirfd=%i, flags=%llu, mode=%llu, resolve=%llu",
           dirfd, how.flags, how.mode, how.resolve);

  LCE_PREP(ev, LCE_EVENT_OPENAT);
  LCE_REGISTER(ev);

end:
  return 0;
}

int lce_hook_close(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_CLOSE);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_unlink(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  lce_user_strcpy((const char __user*)regs->di, ev.arg1, sizeof(ev.arg1));
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_UNLINK);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_rename(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  lce_user_strcpy((const char __user*)regs->di, ev.arg1, sizeof(ev.arg1));
  lce_user_strcpy((const char __user*)regs->si, ev.arg2, sizeof(ev.arg2));

  LCE_PREP(ev, LCE_EVENT_RENAME);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_read(struct kprobe *p, struct pt_regs *regs)
{
  // TODO
  return 0;
}

int lce_hook_fork(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  ev.arg1[0] = '\x00';
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_FORK);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_execve(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  lce_user_strcpy((const char __user*)regs->di, ev.arg1, sizeof(ev.arg1));
  // TODO argv

  LCE_PREP(ev, LCE_EVENT_EXECVE);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_execveat(struct kprobe *p, struct pt_regs *regs)
{
  return 0;
  // TODO
}

int lce_hook_kill(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (pid_t)regs->di);
  snprintf(ev.arg2, sizeof(ev.arg2), "%i", (int)regs->si);

  LCE_PREP(ev, LCE_EVENT_KILL);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_bind(struct kprobe *p, struct pt_regs *regs)
{
  /* TODO */
  return 0;
}

int lce_hook_sendto(struct kprobe *p, struct pt_regs *regs)
{
  /* TODO */
  return 0;
}

int lce_hook_recvfrom(struct kprobe *p, struct pt_regs *regs)
{
  /* TODO */
  return 0;
}

int lce_hook_connect(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;
  struct sockaddr addr;

  const void __user *addr_u = (const char __user*)regs->si;
  const void *addr_k        = (const char*)regs->si;

  if(addr_u && copy_from_user(&addr, addr_u, sizeof(addr)) != 0)
    if(addr_k && copy_from_kernel_nofault(&addr, addr_k, sizeof(addr)) != 0)
      return 0;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  snprintf(ev.arg2, sizeof(ev.arg2), "family=%i, addr=%u.%u.%u.%u",
           addr.sa_family, addr.sa_data[0], addr.sa_data[1], addr.sa_data[2],
           addr.sa_data[3]);
  
  LCE_PREP(ev, LCE_EVENT_CONNECT);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_accept(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;
  struct sockaddr addr;

  const void __user *addr_u = (const char __user*)regs->si;
  const void *addr_k        = (const char*)regs->si;

  if(addr_u && copy_from_user(&addr, addr_u, sizeof(addr)) != 0)
    if(addr_k && copy_from_kernel_nofault(&addr, addr_k, sizeof(addr)) != 0)
      return 0;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  snprintf(ev.arg2, sizeof(ev.arg2), "family=%i, addr=%u.%u.%u.%u",
           addr.sa_family, addr.sa_data[0], addr.sa_data[1], addr.sa_data[2],
           addr.sa_data[3]);
  
  LCE_PREP(ev, LCE_EVENT_ACCEPT);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setuid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETUID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_seteuid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETEUID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setfsuid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETFSUID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setresuid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "ruid=%i, euid=%i, suid=%i",
           (int)regs->di, (int)regs->si, (int)regs->dx);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETRESUID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setgid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETGID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setegid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETEGID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setfsgid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;
  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';
  LCE_PREP(ev, LCE_EVENT_SETFSGID);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_setresgid(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "rgid=%i, egid=%i, sgid=%i",
           (int)regs->di, (int)regs->si, (int)regs->dx);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_SETRESGID);
  LCE_REGISTER(ev);
  return 0;
}

#define LCE_GET_HOOK(name_lower, name_upper)\
int lce_hook_##name_lower(struct kprobe *p, struct pt_regs *regs)\
{\
  LCE_HOOK_GUARD();\
  struct lce_event ev;\
  ev.arg1[0] = '\x00';\
  ev.arg2[0] = '\x00';\
  LCE_PREP(ev, LCE_EVENT_##name_upper);\
  LCE_REGISTER(ev);\
  return 0;\
}

LCE_GET_HOOK(getpid, GETPID);
LCE_GET_HOOK(getppid, GETPPID);
LCE_GET_HOOK(getuid, GETUID);
LCE_GET_HOOK(geteuid, GETEUID);
LCE_GET_HOOK(getresuid, GETRESUID);
LCE_GET_HOOK(getgid, GETGID);
LCE_GET_HOOK(getegid, GETEGID);
LCE_GET_HOOK(getpgid, GETPGID);
LCE_GET_HOOK(getresgid, GETRESGID);

#undef LCE_GET_HOOK

int lce_hook_ptrace(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di); // `op`
  snprintf(ev.arg2, sizeof(ev.arg2), "%i", (int)regs->si); // `pid`

  LCE_PREP(ev, LCE_EVENT_PTRACE);
  LCE_REGISTER(ev);
  return 0;
}

int lce_hook_keyctl(struct kprobe *p, struct pt_regs *regs)
{
  LCE_HOOK_GUARD();

  struct lce_event ev;

  snprintf(ev.arg1, sizeof(ev.arg1), "%i", (int)regs->di);
  ev.arg2[0] = '\x00';

  LCE_PREP(ev, LCE_EVENT_KEYCTL);
  LCE_REGISTER(ev);
  return 0;
}

