#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"

#ifdef _WIN32
#include "hd_sentry_win_stack_trace.h"
#endif

#include <exception>
#include <sstream>

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace hd_sentry {
namespace {

#ifdef _WIN32
LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* info) {
  std::ostringstream stack;
  stack << "Exception code: 0x" << std::hex
        << info->ExceptionRecord->ExceptionCode << std::dec << '\n';

  void* frames[64];
  const USHORT captured = CaptureStackBackTrace(0, 64, frames, nullptr);
  stack << WinStackTraceFormatFrames(frames, captured);

  HdSentryCrashStore::WriteWindowsNativeCrash(info, stack.str());
  return EXCEPTION_CONTINUE_SEARCH;
}

void TerminateHandler() {
  try {
    const auto exception = std::current_exception();
    if (exception) {
      std::rethrow_exception(exception);
    }
  } catch (const std::exception& error) {
    HdSentryCrashStore::WriteReport("windows", "std_terminate",
                                    error.what(), "");
  } catch (...) {
    HdSentryCrashStore::WriteReport("windows", "std_terminate", "unknown",
                                    "");
  }
  std::abort();
}
#endif

}  // namespace

void HdSentryCrashHandler::Install() {
  static bool installed = false;
  if (installed) {
    return;
  }
  installed = true;
  HdSentryCrashStore::ConfigureHdSentry();
#ifdef _WIN32
  WinStackTraceEnsureInitialized();
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);
  std::set_terminate(TerminateHandler);
#endif
}

}  // namespace hd_sentry
