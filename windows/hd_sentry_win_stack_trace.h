#ifndef HD_SENTRY_WIN_STACK_TRACE_H_
#define HD_SENTRY_WIN_STACK_TRACE_H_

#ifdef _WIN32

#include <string>

#include <windows.h>

namespace hd_sentry {

/// One-time dbghelp init for the current process (safe to call from Install).
void WinStackTraceEnsureInitialized();

/// Resolves raw return addresses to symbol + module+offset (UTF-8 text).
std::string WinStackTraceFormatFrames(void* const* frames, USHORT frame_count);

}  // namespace hd_sentry

#endif  // _WIN32

#endif  // HD_SENTRY_WIN_STACK_TRACE_H_
