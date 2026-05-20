#ifndef HD_SENTRY_WIN_MINIDUMP_H_
#define HD_SENTRY_WIN_MINIDUMP_H_

#ifdef _WIN32

#include <filesystem>

struct _EXCEPTION_POINTERS;

namespace hd_sentry {

/// Writes a WinDbg-compatible minidump. [exception_info] may be nullptr (limited context).
bool WinWriteMiniDumpFile(const std::filesystem::path& dump_path,
                          struct _EXCEPTION_POINTERS* exception_info);

}  // namespace hd_sentry

#endif  // _WIN32

#endif  // HD_SENTRY_WIN_MINIDUMP_H_
