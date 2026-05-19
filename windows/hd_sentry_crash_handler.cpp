#include "hd_sentry_crash_handler.h"

#include "hd_sentry_crash_store.h"

#include <exception>
#include <sstream>

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
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
  for (USHORT index = 0; index < captured; ++index) {
    stack << '#' << index << " " << frames[index] << '\n';
  }

  HdSentryCrashStore::WriteReport("windows", "unhandled_exception",
                                  "Unhandled native exception",
                                  stack.str());
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
  HdSentryCrashStore::Configure();
#ifdef _WIN32
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);
  std::set_terminate(TerminateHandler);
#endif
}

}  // namespace hd_sentry
