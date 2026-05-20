#ifndef HD_SENTRY_CRASH_STORE_H_
#define HD_SENTRY_CRASH_STORE_H_

#include <string>
#include <vector>

namespace hd_sentry {

class HdSentryCrashStore {
 public:
  static void ConfigureHdSentry();
  static std::string Directory();
  static std::vector<std::string> ListFileNames();
  static std::string ReadFile(const std::string& file_name);
  static bool DeleteFileHdSentry(const std::string& file_name);
  static void ClearAll();
  static std::string WriteReport(const std::string& platform,
                                 const std::string& type,
                                 const std::string& message,
                                 const std::string& stack_trace);

#if defined(_WIN32)
  /// Writes paired `crash_<millis>.txt` and `crash_<millis>.dmp` for WinDbg. Returns the .txt name.
  static std::string WriteWindowsNativeCrash(void* exception_pointers,
                                             const std::string& stack_trace);
#endif
};

}  // namespace hd_sentry

#endif  // HD_SENTRY_CRASH_STORE_H_
