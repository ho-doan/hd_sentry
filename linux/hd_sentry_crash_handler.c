#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"
#include "hd_sentry_linux_crash_dump.h"
#include "hd_sentry_linux_stack_trace.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t g_crash_report_written = 0;
static gboolean g_installed = FALSE;

static void signal_handler(int sig, siginfo_t* info, void* context) {
  struct sigaction default_action;
  memset(&default_action, 0, sizeof(default_action));
  default_action.sa_handler = SIG_DFL;
  sigaction(sig, &default_action, NULL);

  if (g_crash_report_written == 0) {
    g_crash_report_written = 1;
    g_autofree gchar* msg =
        g_strdup_printf("Signal %d received", sig);
    g_autofree gchar* stack =
        hd_sentry_linux_stack_trace_capture_skip(2);
    hd_sentry_crash_store_write_native_crash("linux", "signal", msg, stack, sig,
                                              info, context);
  }
  raise(sig);
}

void hd_sentry_crash_handler_install_signals(void) {
  if (g_installed) {
    return;
  }
  g_installed = TRUE;

  hd_sentry_crash_store_configure();
  hd_sentry_linux_crash_dump_configure();

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = signal_handler;
  action.sa_flags = SA_SIGINFO;
  sigemptyset(&action.sa_mask);

  const int signals[] = {SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP};
  for (gsize i = 0; i < G_N_ELEMENTS(signals); ++i) {
    sigaction(signals[i], &action, NULL);
  }
}
