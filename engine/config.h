// config.h
// breaker box of compile-time options

#ifndef _CONFIG_H
#define _CONFIG_H

// maximum number of events to be pulled per read
// this is so that the reading program can overshoot the amount it needs to read
// and prevent entry clobbering in the process
#define LCE_MAX_EVENTS_PER_READ 2048

// HOOK TOGGLES
// Comment out the hooks for different switches to remove them from the final
// executable. These options are, generally speaking, categorized by the domain
// the syscall pertains to.

#define LCE_HOOK_OPEN
#define LCE_HOOK_CLOSE
#define LCE_HOOK_UNLINK
#define LCE_HOOK_RENAME

// warning: noisy
//#define LCE_HOOK_READ
//#define LCE_HOOK_WRITE

#define LCE_HOOK_FORK
#define LCE_HOOK_EXECVE
#define LCE_HOOK_KILL

#define LCE_HOOK_BIND
#define LCE_HOOK_SENDTO
#define LCE_HOOK_RECVFROM
#define LCE_HOOK_CONNECT
#define LCE_HOOK_ACCEPT

#define LCE_HOOK_SETGID
#define LCE_HOOK_SETUID
#define LCE_HOOK_SETREUID

// warning: noisy
//#define LCE_HOOK_GETUID_FAMILY // getuid, geteuid, getresuid
//#define LCE_HOOK_GETGID_FAMILY // getgid, getegid, getpgid, getresgid
//#define LCE_HOOK_GETPID_FAMILY // getpid, getppid
#define LCE_HOOK_SETUID_FAMILY // setuid, seteuid, setresuid, setfsuid
#define LCE_HOOK_SETGID_FAMILY // setgid, setegid, setpgid, setresgid, setfsgid


#define LCE_HOOK_PTRACE
#define LCE_HOOK_CAPGET
#define LCE_HOOK_CAPSET

#define LCE_KEYCTL

#endif
