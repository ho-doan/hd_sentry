#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"
#include "hd_sentry_linux_stack_trace.h"

#include <exception>

namespace {

void TerminateHandler() {
  try {
    const auto exception = std::current_exception();
    if (exception) {
      std::rethrow_exception(exception);
    }
    g_autofree gchar* stack = hd_sentry_linux_stack_trace_capture_skip(1);
    hd_sentry_crash_store_write_native_crash("linux", "std_terminate", "unknown",
                                             stack, SIGABRT, NULL, NULL);
  } catch (const std::exception& error) {
    g_autofree gchar* stack = hd_sentry_linux_stack_trace_capture_skip(1);
    hd_sentry_crash_store_write_native_crash("linux", "std_terminate",
                                             error.what(), stack, SIGABRT, NULL,
                                             NULL);
  } catch (...) {
    g_autofree gchar* stack = hd_sentry_linux_stack_trace_capture_skip(1);
    hd_sentry_crash_store_write_native_crash("linux", "std_terminate", "unknown",
                                             stack, SIGABRT, NULL, NULL);
  }
  std::abort();
}

}  // namespace

extern "C" void hd_sentry_crash_handler_install(void) {
  hd_sentry_crash_handler_install_signals();
  std::set_terminate(TerminateHandler);
}
