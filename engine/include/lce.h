// lce.h

#include<linux/atomic.h>
#include<linux/kfifo.h>
#include<linux/kprobes.h>
#include<linux/proc_fs.h>

extern atomic_t hooks_ready;
extern atomic_t hooks_collecting;

#include"config.h"

/*** KFIFO & EVENTS***/

enum lce_event_type
{
  LCE_EVENT_OPEN,
  LCE_EVENT_CLOSE,
  LCE_EVENT_UNLINK,
  LCE_EVENT_RENAME,
  LCE_EVENT_READ,
  LCE_EVENT_PREAD,
  LCE_EVENT_WRITE,
  LCE_EVENT_PWRITE,
  LCE_EVENT_SENDTO,
  LCE_EVENT_RECVFROM,
  LCE_EVENT_CONNECT,
  LCE_EVENT_ACCEPT,
  LCE_EVENT_SETGID,
  LCE_EVENT_SETUID,
  LCE_EVENT_SETREUID,
  LCE_EVENT_SETRESUID,
  LCE_EVENT_SETRESGUID,
  LCE_EVENT_GETPID,
  LCE_EVENT_GETPPID,
  LCE_EVENT_GETUID,
  LCE_EVENT_GETGID,
  LCE_EVENT_GETREUID,
  LCE_EVENT_PTRACE,
  LCE_EVENT_CAPGET,
  LCE_EVENT_CAPSET,
  LCE_EVENT_KEYCTL
};

// size of FIFO buffer
#define LCE_FIFO_SZ     8192
// size of argument representation
#define LCE_ARG_REPR_SZ 128

struct lce_event
{
  u32 ts_s;
  u32 ts_ns;
  pid_t pid;
  u16 type; // LCE_EVENT_*
  char arg1[LCE_ARG_REPR_SZ];
  char arg2[LCE_ARG_REPR_SZ];
  int ret;
};

static DECLARE_KFIFO(lce_kfifo, struct lce_event, LCE_FIFO_SZ);

/*** PROCFILE ***/

static int lce_proc_open(struct inode *inode, struct file *file);
static int lce_proc_read(struct inode *inode, struct file *file);

/*** HOOKS ***/

#define GEN_HOOK(name) int name(struct kprobe *p, struct pt_regs *regs)
static int lce_hook_ret(struct kretprobe_instance *ri, struct pt_regs *regs);

#ifdef LCE_HOOK_OPEN
GEN_HOOK(lce_hook_open);
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
GEN_HOOK(lce_hook_seteuid);
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

#ifdef LCE_HOOK_CAPGET
GEN_HOOK(lce_hook_capget);
#endif

#ifdef LCE_HOOK_CAPSET
GEN_HOOK(lce_hook_capset);
#endif

#ifdef LCE_HOOK_KEYCTL
GEN_HOOK(lce_hook_keyctl);
#endif

// silly little macro to lower the amount that I have to type lol
#define P(name) "__x64_sys_name"
#define ENTRY(symbol, func)\
{\
  .symbol_name = P(symbol),\
  .pre_handler = func,\
}

static struct kprobe lce_kprobes[] = {

#ifdef LCE_HOOK_OPEN
  ENTRY(open, lce_hook_open),
  ENTRY(openat2, lce_hook_open),
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
  ENTRY(seteuid, lce_hook_seteuid),
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
  ENTRY(setresgid, lce_hook_setresgid),
  ENTRY(setresgid16, lce_hook_setresgid),
#endif

#ifdef LCE_HOOK_PTRACE
  ENTRY(ptrace, lce_hook_ptrace),
#endif

#ifdef LCE_HOOK_CAPGET
  ENTRY(capget, lce_hook_capget),
#endif

#ifdef LCE_HOOK_CAPSET
  ENTRY(capset, lce_hook_capset),
#endif

#ifdef LCE_HOOK_KEYCTL
    ENTRY(keyctl, lce_hook_keyctl),
#endif

};

#undef ENTRY
#define ENTRY(symbol)\
{\
  .kp.symbol_name = "__x64_sys_symbol",\
  .handler = lce_hook_ret,\
  .maxactive = 0,\
}

static struct kretprobe lce_kretprobes[] = {
#ifdef LCE_HOOK_OPEN
  ENTRY(open),
  ENTRY(openat2),
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
#ifdef LCE_HOOK_SETGID
  ENTRY(setgid),
#endif
#ifdef LCE_HOOK_SETUID
  ENTRY(setuid),
#endif
#ifdef LCE_HOOK_SETE
};

