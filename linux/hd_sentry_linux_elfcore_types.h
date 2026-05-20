#ifndef HD_SENTRY_LINUX_ELFCORE_TYPES_H_
#define HD_SENTRY_LINUX_ELFCORE_TYPES_H_

/**
 * Minimal ELF CORE note types (from Linux uapi linux/elfcore.h).
 * Vendored so we do not require kernel headers such as <linux/elfcore.h>.
 */

#include <sys/time.h>
#include <sys/types.h>

#define HD_SENTRY_NT_PRSTATUS 1
#define HD_SENTRY_NT_SIGINFO 0x53494749
#define HD_SENTRY_NT_FILE 0x46494c45
#define HD_SENTRY_NT_PRPSINFO 3

typedef struct HdSentryElfSiginfo {
  int si_signo;
  int si_code;
  int si_errno;
} HdSentryElfSiginfo;

#if defined(__x86_64__)

#define HD_SENTRY_ELF_NGREG 27
typedef unsigned long HdSentryElfGreg;
typedef HdSentryElfGreg HdSentryElfGregset[HD_SENTRY_ELF_NGREG];

typedef struct HdSentryElfPrstatus {
  struct {
    int si_signo;
    int si_code;
    int si_errno;
  } pr_info;
  short pr_cursig;
  unsigned long pr_sigpend;
  unsigned long pr_sighold;
  pid_t pr_pid;
  pid_t pr_ppid;
  pid_t pr_pgrp;
  pid_t pr_sid;
  struct timeval pr_utime;
  struct timeval pr_stime;
  struct timeval pr_cutime;
  struct timeval pr_cstime;
  HdSentryElfGregset pr_reg;
  int pr_fpvalid;
} HdSentryElfPrstatus;

/* GDB on x86_64 Linux expects pr_reg at byte offset 112 in NT_PRSTATUS. */
#define HD_SENTRY_ELF_PRSTATUS_REG_OFFSET 112

typedef struct HdSentryElfPrpsinfo {
  char pr_state;
  char pr_sname;
  char pr_zomb;
  char pr_nice;
  unsigned long pr_flag;
  unsigned short pr_uid;
  unsigned short pr_gid;
  int pr_pid;
  int pr_ppid;
  int pr_pgrp;
  int pr_sid;
  char pr_fname[16];
  char pr_psargs[80];
} HdSentryElfPrpsinfo;

#elif defined(__aarch64__)

#define HD_SENTRY_ELF_NGREG 34
typedef unsigned long HdSentryElfGreg;
typedef HdSentryElfGreg HdSentryElfGregset[HD_SENTRY_ELF_NGREG];

typedef struct HdSentryElfPrstatus {
  struct {
    int si_signo;
    int si_code;
    int si_errno;
  } pr_info;
  short pr_cursig;
  unsigned long pr_sigpend;
  unsigned long pr_sighold;
  pid_t pr_pid;
  pid_t pr_ppid;
  pid_t pr_pgrp;
  pid_t pr_sid;
  struct timeval pr_utime;
  struct timeval pr_stime;
  struct timeval pr_cutime;
  struct timeval pr_cstime;
  HdSentryElfGregset pr_reg;
  int pr_fpvalid;
} HdSentryElfPrstatus;

#define HD_SENTRY_ELF_PRSTATUS_REG_OFFSET 112

typedef struct HdSentryElfPrpsinfo {
  char pr_state;
  char pr_sname;
  char pr_zomb;
  char pr_nice;
  unsigned long pr_flag;
  unsigned short pr_uid;
  unsigned short pr_gid;
  int pr_pid;
  int pr_ppid;
  int pr_pgrp;
  int pr_sid;
  char pr_fname[16];
  char pr_psargs[80];
} HdSentryElfPrpsinfo;

#endif

#endif  // HD_SENTRY_LINUX_ELFCORE_TYPES_H_
