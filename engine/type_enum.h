// type_num.h
// holds the syscall types (and their associated strings) that are used by both
// the LCE & by the event thread

#pragma once

// trying my damndest to avoid collisions
static const char *LCE_LINE_SEP  = "NEXTEVENT";
static const char *LCE_FIELD_SEP = "NEXTFIELD";

enum lce_event_type
{
  LCE_EVENT_INVALID,

  LCE_EVENT_OPEN,
  LCE_EVENT_OPENAT,
  LCE_EVENT_OPENAT2,
  LCE_EVENT_CLOSE,
  LCE_EVENT_UNLINK,
  LCE_EVENT_RENAME,

  LCE_EVENT_READ,
  LCE_EVENT_PREAD,
  LCE_EVENT_WRITE,
  LCE_EVENT_PWRITE,

  LCE_EVENT_FORK,
  LCE_EVENT_EXECVE,
  LCE_EVENT_KILL,

  LCE_EVENT_BIND,
  LCE_EVENT_SENDTO,
  LCE_EVENT_RECVFROM,
  LCE_EVENT_CONNECT,
  LCE_EVENT_ACCEPT,

  LCE_EVENT_SETUID,
  LCE_EVENT_SETFSUID,
  LCE_EVENT_SETRESUID,
  LCE_EVENT_SETGID,
  LCE_EVENT_SETEGID,
  LCE_EVENT_SETFSGID,
  LCE_EVENT_SETRESGID,
  
  LCE_EVENT_GETPID,
  LCE_EVENT_GETPPID,

  LCE_EVENT_GETUID,
  LCE_EVENT_GETEUID,
  LCE_EVENT_GETRESUID,
  LCE_EVENT_GETGID,
  LCE_EVENT_GETEGID,
  LCE_EVENT_GETPGID,
  LCE_EVENT_GETRESGID,

  LCE_EVENT_PTRACE,

  /*
  LCE_EVENT_CAPGET,
  LCE_EVENT_CAPSET,
  */

  LCE_EVENT_KEYCTL
};

#ifndef __KERNEL__

#define LCE_SEP_SZ sizeof(LCE_LINE_SEP) - 1

static const char *LCE_EVENT_REPR[] = {
  "invalid",

  "open",
  "openat",
  "openat2",
  "close",
  "unlink",
  "rename",

  "read",
  "pread",
  "write",
  "pwrite",

  "fork",
  "execve",
  "kill",

  "bind",
  "sendto",
  "recfrom",
  "connect",
  "accept",

  "setuid",
  "setfsuid",
  "setresuid",
  "setgid",
  "setegid",
  "setfsgid",
  "setresgid",

  "getpid",
  "getppid",
  
  "getuid",
  "geteuid",
  "getresuid",
  "getgid",
  "getegid",
  "getpgid",
  "getresgid",

  "ptrace",

  /*
  "capget",
  "capset",
  */

  "keyctl",
};

#endif /* __KERNEL */
