#include "hd_sentry_win_minidump.h"

#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>

#include <filesystem>

#pragma comment(lib, "dbghelp.lib")

namespace hd_sentry {

bool WinWriteMiniDumpFile(const std::filesystem::path& dump_path,
                          EXCEPTION_POINTERS* exception_info) {
  const std::wstring wpath = dump_path.wstring();
  HANDLE file = CreateFileW(wpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  MINIDUMP_EXCEPTION_INFORMATION mei{};
  MINIDUMP_EXCEPTION_INFORMATION* mei_ptr = nullptr;
  if (exception_info != nullptr) {
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = exception_info;
    mei.ClientPointers = FALSE;
    mei_ptr = &mei;
  }

  // Enough for WinDbg stacks + modules; avoids full-memory dumps.
  const MINIDUMP_TYPE dump_type = static_cast<MINIDUMP_TYPE>(
      MiniDumpWithPrivateReadWriteMemory | MiniDumpWithDataSegs |
      MiniDumpWithHandleData | MiniDumpWithThreadInfo |
      MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory);

  const BOOL ok = MiniDumpWriteDump(
      GetCurrentProcess(), GetCurrentProcessId(), file, dump_type, mei_ptr,
      nullptr, nullptr);
  CloseHandle(file);
  if (!ok) {
    DeleteFileW(wpath.c_str());
    return false;
  }
  return true;
}

}  // namespace hd_sentry

#endif  // _WIN32
