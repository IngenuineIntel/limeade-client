// lce.h

#include<linux/atomic.h>

extern atomic_t hooks_ready;
extern atomic_t hooks_collecting;

#include"config.h"

/*** HOOKS ***/

#define GEN_HOOK(name) int name(struct kprobe *p, struct pt_regs *regs)

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

#ifdef LCE_HOOK_BIND
GEN_HOOK(lce_hook_write);
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

#ifdef LCE_HOOK_SETGID
GEN_HOOK(lce_hook_setgid);
#endif

#ifdef LCE_HOOK_SETUID
GEN_HOOK(lce_hook_setuid);
#endif

#ifdef LCE_HOOK_SETREUID
GEN_HOOK(lce_hook_setreuid);
#endif

#ifdef LCE_HOOK_GETPID_FAMILY
GEN_HOOK(lce_hook_getpid_family);
#endif

#ifdef LCE_HHOK_GETUID_FAMILY
GEN_HOOK(lce_hook_getuid_family);
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



