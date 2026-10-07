// config.h
// breaker box of compile-time options

#ifndef _CONFIG_H
#define _CONFIG_H

// size of internal KFIFO buffer
// if not a power of 2, is raised to a power of 2
#define LCE_FIFO_SZ 8192

// maximum size of argument representation (including the null terminator)
#define LCE_ARG_REPR_SZ 50

// proc entry
#define LCE_PROCFILE_PATH "lce"

// proc entry permissions
#define LCE_PROCFILE_PERM 0400

// number in which kfifo overflows are logged
// minimum is 1, maximum is UINT32_MAX
#define LCE_OVERFLOW_LOG_CHUNK_SZ 1000

// HOOK TOGGLES
// Comment out the hooks for different switches to remove them from the final
// executable. These options are, generally speaking, categorized by the domain
// the syscall pertains to.

// NOISE LEVELS
// are a subjective baseline you can use to decide which hooks you want on & off
// default is 0
//
// 0: absolutely no noise, doesn't get called very often
// 1: not really any noise, but does get called
// 2: common, medium noise
// 3: very common, high noise
// 4: very common, could make the queue unstable in some configurations

//#define LCE_HOOK_OPEN    // 3
//#define LCE_HOOK_CLOSE // 4
#define LCE_HOOK_UNLINK  // 2
#define LCE_HOOK_RENAME  // 1

//#define LCE_HOOK_READ  // 4
//#define LCE_HOOK_WRITE // 4

#define LCE_HOOK_FORK    // 1
#define LCE_HOOK_EXECVE  // 1
#define LCE_HOOK_KILL    // 0

#define LCE_HOOK_BIND    // 0
#define LCE_HOOK_SENDTO  // 0
#define LCE_HOOK_RECVFROM
#define LCE_HOOK_CONNECT
#define LCE_HOOK_ACCEPT

#define LCE_HOOK_GETUID_FAMILY // getuid, geteuid, getresuid
#define LCE_HOOK_GETGID_FAMILY // getgid, getegid, getpgid, getresgid
#define LCE_HOOK_GETPID_FAMILY // getpid, getppid
#define LCE_HOOK_SETUID_FAMILY // setuid, setresuid, setfsuid
#define LCE_HOOK_SETGID_FAMILY // setgid, setegid, setpgid, setresgid, setfsgid


#define LCE_HOOK_PTRACE
#define LCE_HOOK_CAPGET
#define LCE_HOOK_CAPSET

#define LCE_KEYCTL

#endif
