#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static volatile sig_atomic_t g_crash_report_written = 0;
static gboolean g_installed = FALSE;

static void signal_handler(int sig) {
  signal(sig, SIG_DFL);
  if (g_crash_report_written == 0) {
    g_crash_report_written = 1;
    g_autofree gchar* msg =
        g_strdup_printf("Signal %d received", sig);
    hd_sentry_crash_store_write_report("linux", "signal", msg, "");
  }
  raise(sig);
}

void hd_sentry_crash_handler_install_signals(void) {
  if (g_installed) {
    return;
  }
  g_installed = TRUE;

  hd_sentry_crash_store_configure();

  const int signals[] = {SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP};
  for (gsize i = 0; i < G_N_ELEMENTS(signals); ++i) {
    signal(signals[i], signal_handler);
  }
}
