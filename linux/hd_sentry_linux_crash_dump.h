#ifndef HD_SENTRY_LINUX_CRASH_DUMP_H_
#define HD_SENTRY_LINUX_CRASH_DUMP_H_

#include <glib.h>
#include <signal.h>

G_BEGIN_DECLS

/** Enables dumpable + core rlimit (best-effort). Call once at install. */
void hd_sentry_linux_crash_dump_configure(void);

/**
 * Writes companion files next to the crash report:
 *   <stem>.maps — /proc/self/maps snapshot
 *   <stem>.dmp  — ELF core (gdb/lldb) when ucontext is available
 *
 * @crash_dir: absolute crash directory (same as crash store).
 * @stem: file name without suffix, e.g. crash_1712345678901
 */
gboolean hd_sentry_linux_crash_dump_write(const gchar* crash_dir,
                                          const gchar* stem,
                                          gint signo,
                                          const siginfo_t* info,
                                          const void* ucontext);

G_END_DECLS

#endif  // HD_SENTRY_LINUX_CRASH_DUMP_H_
