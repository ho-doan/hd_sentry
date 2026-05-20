#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"

#include <csignal>
#include <cstring>
#include <exception>

namespace hd_sentry {
namespace {

volatile sig_atomic_t g_crash_report_written = 0;

void SignalHandler(int signal) {
  std::signal(signal, SIG_DFL);
  if (g_crash_report_written == 0) {
    g_crash_report_written = 1;
    HdSentryCrashStore::WriteReport(
        "linux", "signal", std::string("Signal ") + std::to_string(signal),
        "");
  }
  std::raise(signal);
}

void TerminateHandler() {
  try {
    const auto exception = std::current_exception();
    if (exception) {
      std::rethrow_exception(exception);
    }
  } catch (const std::exception& error) {
    HdSentryCrashStore::WriteReport("linux", "std_terminate", error.what(),
                                    "");
  } catch (...) {
    HdSentryCrashStore::WriteReport("linux", "std_terminate", "unknown", "");
  }
  std::abort();
}

}  // namespace

void HdSentryCrashHandler::Install() {
  static bool installed = false;
  if (installed) {
    return;
  }
  installed = true;
  HdSentryCrashStore::ConfigureHdSentry();

  const int signals[] = {SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP};
  for (const int signal : signals) {
    std::signal(signal, SignalHandler);
  }
  std::set_terminate(TerminateHandler);
}

}  // namespace hd_sentry
