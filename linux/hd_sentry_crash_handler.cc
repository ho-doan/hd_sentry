#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"

#include <exception>

namespace {

void TerminateHandler() {
  try {
    const auto exception = std::current_exception();
    if (exception) {
      std::rethrow_exception(exception);
    }
    hd_sentry_crash_store_write_report("linux", "std_terminate", "unknown", "");
  } catch (const std::exception& error) {
    hd_sentry_crash_store_write_report("linux", "std_terminate", error.what(),
                                       "");
  } catch (...) {
    hd_sentry_crash_store_write_report("linux", "std_terminate", "unknown", "");
  }
  std::abort();
}

}  // namespace

extern "C" void hd_sentry_crash_handler_install(void) {
  hd_sentry_crash_handler_install_signals();
  std::set_terminate(TerminateHandler);
}
