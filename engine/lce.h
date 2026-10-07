// lce.h

// TODO hooks
// renameat / renameat2
// send/sendmsg

#pragma once

#include<linux/atomic.h>
#include<linux/hashtable.h>
#include<linux/kfifo.h>
#include<linux/kprobes.h>
#include<linux/proc_fs.h>

#include"config.h"
#include"type_enum.h"

/*** KFIFO & EVENTS***/

struct lce_event
{
  u64 ts;
  pid_t pid;
  u16 type; // LCE_EVENT_*
  char arg1[LCE_ARG_REPR_SZ];
  char arg2[LCE_ARG_REPR_SZ];
  int ret;
};

struct lce_event_pending
{
  struct lce_event ev;
  struct hlist_node node;
};

#define LCE_HASH_BITS 8

static DEFINE_KFIFO(lce_kfifo, struct lce_event, LCE_FIFO_SZ);
static DEFINE_SPINLOCK(lce_kfifo_lock);

static DEFINE_HASHTABLE(lce_hashtbl, LCE_HASH_BITS);
static DEFINE_SPINLOCK(lce_hashtbl_lock);

/*** PROCFILE ***/

int lce_proc_open(struct inode *inode, struct file *file);
ssize_t lce_proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);
static struct proc_dir_entry *lce_proc_entry;

/*** HOOKS ***/

extern atomic_t lce_ready;

#define LCE_HOOK_GUARD()\
if(!atomic_read(&lce_ready))\
  return 0;

// copies a string from userspace
int lce_user_strcpy(const char __user *s, char *dst, int sz);

#define GEN_HOOK(name) int name(struct kprobe *p, struct pt_regs *regs)
int lce_hook_ret(struct kretprobe_instance *ri, struct pt_regs *regs);

#ifdef LCE_HOOK_OPEN
GEN_HOOK(lce_hook_open);
GEN_HOOK(lce_hook_openat);
#endif

#ifdef LCE_HOOK_CLOSE
GEN_HOOK(lce_hook_close);
#endif

#ifdef LCE_HOOK_UNLINK
GEN_HOOK(lce_hook_unlink);
#endif

#ifdef LCE_HOOK_RENAME
GEN_HOOK(lce_hook_rename);
#endif

#ifdef LCE_HOOK_READ
GEN_HOOK(lce_hook_read);
#endif

#ifdef LCE_HOOK_WRITE
GEN_HOOK(lce_hook_write);
#endif

#ifdef LCE_HOOK_FORK
GEN_HOOK(lce_hook_fork);
#endif

#ifdef LCE_HOOK_EXECVE
GEN_HOOK(lce_hook_execve);
GEN_HOOK(lce_hook_execveat);
#endif

#ifdef LCE_HOOK_KILL
GEN_HOOK(lce_hook_kill);
#endif

#ifdef LCE_HOOK_BIND
GEN_HOOK(lce_hook_bind);
#endif

#ifdef LCE_HOOK_SENDTO
GEN_HOOK(lce_hook_sendto);
#endif

#ifdef LCE_HOOK_RECVFROM
GEN_HOOK(lce_hook_recvfrom);
#endif

#ifdef LCE_HOOK_CONNECT
GEN_HOOK(lce_hook_connect);
#endif

#ifdef LCE_HOOK_ACCEPT
GEN_HOOK(lce_hook_accept);
#endif

#ifdef LCE_HOOK_GETUID_FAMILY
GEN_HOOK(lce_hook_getuid);
GEN_HOOK(lce_hook_geteuid);
GEN_HOOK(lce_hook_getresuid);
#endif

#ifdef LCE_HOOK_GETGID_FAMILY
GEN_HOOK(lce_hook_getgid);
GEN_HOOK(lce_hook_getegid);
GEN_HOOK(lce_hook_getpgid);
GEN_HOOK(lce_hook_getresgid);
#endif

#ifdef LCE_HOOK_GETPID_FAMILY
GEN_HOOK(lce_hook_getpid);
GEN_HOOK(lce_hook_getppid);
#endif

#ifdef LCE_HOOK_SETUID_FAMILY
GEN_HOOK(lce_hook_setuid);
GEN_HOOK(lce_hook_setresuid);
GEN_HOOK(lce_hook_setfsuid);
#endif

#ifdef LCE_HOOK_SETGID_FAMILY
GEN_HOOK(lce_hook_setgid);
GEN_HOOK(lce_hook_setegid);
GEN_HOOK(lce_hook_setpgid);
GEN_HOOK(lce_hook_setresgid);
GEN_HOOK(lce_hook_setfsgid);
#endif

#ifdef LCE_HOOK_PTRACE
GEN_HOOK(lce_hook_ptrace);
#endif

// not prioritized
/*
#ifdef LCE_HOOK_CAPGET
GEN_HOOK(lce_hook_capget);
#endif

#ifdef LCE_HOOK_CAPSET
GEN_HOOK(lce_hook_capset);
#endif
*/

#ifdef LCE_HOOK_KEYCTL
GEN_HOOK(lce_hook_keyctl);
#endif

#define ENTRY(symbol, func)\
{\
  .symbol_name = "__x64_sys_"#symbol,\
  .pre_handler = func,\
}

static struct kprobe lce_kprobes[] = {

#ifdef LCE_HOOK_OPEN
  ENTRY(open, lce_hook_open),
  ENTRY(openat, lce_hook_openat),
#endif

#ifdef LCE_HOOK_CLOSE
  ENTRY(close, lce_hook_close),
#endif

#ifdef LCE_HOOK_UNLINK
  ENTRY(unlink, lce_hook_unlink),  
#endif

#ifdef LCE_HOOK_RENAME
  ENTRY(rename, lce_hook_rename),
#endif

#ifdef LCE_HOOK_READ
  ENTRY(read, lce_hook_read),
  ENTRY(pread64, lce_hook_read),
#endif

#ifdef LCE_HOOK_WRITE
  ENTRY(write, lce_hook_write),
  ENTRY(pwrite64, lce_hook_write),
#endif

#ifdef LCE_HOOK_FORK
  ENTRY(fork, lce_hook_fork),
#endif

#ifdef LCE_HOOK_EXECVE
  ENTRY(execve, lce_hook_execve),
  ENTRY(execveat, lce_hook_execveat),
#endif

#ifdef LCE_HOOK_KILL
  ENTRY(kill, lce_hook_kill),
#endif

#ifdef LCE_HOOK_BIND
  ENTRY(bind, lce_hook_bind),
#endif

#ifdef LCE_HOOK_SENDTO
  ENTRY(sendto, lce_hook_sendto),
#endif

#ifdef LCE_HOOK_RECVFROM
  ENTRY(recvfrom, lce_hook_recvfrom),
#endif

#ifdef LCE_HOOK_CONNECT
  ENTRY(connect, lce_hook_connect),
#endif

#ifdef LCE_HOOK_ACCEPT
  ENTRY(accept, lce_hook_accept),
#endif

#ifdef LCE_HOOK_GETUID_FAMILY
  ENTRY(getuid, lce_hook_getuid),
  ENTRY(getuid16, lce_hook_getuid),
  ENTRY(geteuid, lce_hook_geteuid),
  ENTRY(getduid16, lce_hook_getuid),
  ENTRY(getresuid, lce_hook_getresuid),
  ENTRY(getresuid16, lce_hook_getresuid),
#endif

#ifdef LCE_HOOK_GETGID_FAMILY
  ENTRY(getgid, lce_hook_getgid),
  ENTRY(getgid16, lce_hook_getgid),
  ENTRY(getegid, lce_hook_getegid),
  ENTRY(getegid16, lce_hook_getegid),
  ENTRY(getpgid, lce_hook_getpgid),
  ENTRY(getresgid, lce_hook_getresgid),
  ENTRY(getresgid16, lce_hook_getresgid),
#endif

#ifdef LCE_HOOK_GETPID_FAMILY
  ENTRY(getpid, lce_hook_getpid),
  ENTRY(getppid, lce_hook_getppid),
#endif

#ifdef LCE_HOOK_SETUID_FAMILY
  ENTRY(setuid, lce_hook_setuid),
  ENTRY(setuid16, lce_hook_setuid),
  ENTRY(seteuid16, lce_hook_setuid),
  ENTRY(setfsuid, lce_hook_setfsuid),
  ENTRY(setfsuid16, lce_hook_setfsuid),
  ENTRY(setresuid, lce_hook_setresuid),
  ENTRY(setresuid16, lce_hook_setresuid),
#endif

#ifdef LCE_HOOK_SETGID_FAMILY
  ENTRY(setgid, lce_hook_setgid),
  ENTRY(setgid16, lce_hook_setgid),
  ENTRY(setegid, lce_hook_setegid),
  ENTRY(setegid16, lce_hook_setegid),
  ENTRY(setfsgid, lce_hook_setfsgid),
  ENTRY(setfsgid16, lce_hook_setfsgid),
  ENTRY(setresgid, lce_hook_setresgid),
  ENTRY(setresgid16, lce_hook_setresgid),
#endif

#ifdef LCE_HOOK_PTRACE
  ENTRY(ptrace, lce_hook_ptrace),
#endif

/*
#ifdef LCE_HOOK_CAPGET
  ENTRY(capget, lce_hook_capget),
#endif

#ifdef LCE_HOOK_CAPSET
  ENTRY(capset, lce_hook_capset),
#endif
*/

#ifdef LCE_HOOK_KEYCTL
    ENTRY(keyctl, lce_hook_keyctl),
#endif

};

#undef ENTRY
#define ENTRY(symbol)\
{\
  .kp.symbol_name = "__x64_sys_" #symbol,\
  .handler = lce_hook_ret,\
  .maxactive = 0,\
}


static struct kretprobe lce_kretprobes[] = {

#ifdef LCE_HOOK_OPEN
  ENTRY(open),
  ENTRY(openat),
#endif

#ifdef LCE_HOOK_CLOSE
  ENTRY(close),
#endif

#ifdef LCE_HOOK_UNLINK
  ENTRY(unlink),
#endif

#ifdef LCE_HOOK_RENAME
  ENTRY(rename),
#endif

#ifdef LCE_HOOK_READ
  ENTRY(read),
  ENTRY(pread64),
#endif

#ifdef LCE_HOOK_WRITE
  ENTRY(write),
  ENTRY(pwrite64),
#endif

#ifdef LCE_HOOK_FORK
  ENTRY(fork),
#endif

#ifdef LCE_HOOK_EXECVE
  ENTRY(execve),
  ENTRY(execveat),
#endif

#ifdef LCE_HOOK_KILL
  ENTRY(kill),
#endif

#ifdef LCE_HOOK_BIND
  ENTRY(bind),
#endif

#ifdef LCE_HOOK_SENDTO
  ENTRY(sendto),
#endif

#ifdef LCE_HOOK_RECVFROM
  ENTRY(recvfrom),
#endif

#ifdef LCE_HOOK_CONNECT
  ENTRY(connect),
#endif

#ifdef LCE_HOOK_ACCEPT
  ENTRY(accept),
#endif

#ifdef LCE_HOOK_GETUID_FAMILY
  ENTRY(getuid),
  ENTRY(getuid16),
  ENTRY(geteuid),
  ENTRY(geteuid16),
  ENTRY(getresuid),
  ENTRY(getresuid16),
#endif

#ifdef LCE_HOOK_GETGID_FAMILY
  ENTRY(getgid),
  ENTRY(getgid16),
  ENTRY(getegid),
  ENTRY(getegid16),
  ENTRY(getpgid),
  ENTRY(getresgid),
  ENTRY(getresgid16),
#endif

#ifdef LCE_HOOK_GETPID_FAMILY
  ENTRY(getpid),
  ENTRY(getppid),
#endif

#ifdef LCE_HOOK_SETUID_FAMILY
  ENTRY(setuid),
  ENTRY(setuid16),
  ENTRY(seteuid),
  ENTRY(seteuid16),
  ENTRY(setfsuid),
  ENTRY(setfsuid16),
  ENTRY(setresuid),
  ENTRY(setresuid16),
#endif

#ifdef LCE_HOOK_SETGID_FAMILY
  ENTRY(setgid),
  ENTRY(setgid16),
  ENTRY(setegid),
  ENTRY(setegid16),
  ENTRY(setfsgid),
  ENTRY(setfsgid16),
  ENTRY(setresgid),
  ENTRY(setresgid16),
#endif

#ifdef LCE_HOOK_PTRACE
  ENTRY(ptrace),
#endif

/*
#ifdef LCE_HOOK_CAPGET
  ENTRY(capget),
#endif

#ifdef LCE_HOOK_CAPSET
  ENTRY(capset),
#endif
*/

#ifdef LCE_HOOK_KEYCTL
  ENTRY(keyctl),
#endif

};

static int lce_nr_kprobes    = ARRAY_SIZE(lce_kprobes);
static int lce_nr_kretprobes = ARRAY_SIZE(lce_kretprobes);

#undef GEN_HOOK
