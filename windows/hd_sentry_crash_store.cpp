#include "hd_sentry_crash_store.h"

#include "hd_sentry_win_minidump.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace hd_sentry {
namespace {

namespace fs = std::filesystem;
constexpr char kCrashDirName[] = "hd_sentry_crashes";
constexpr char kFilePrefix[] = "crash_";
constexpr char kFileSuffix[] = ".txt";
constexpr char kDumpSuffix[] = ".dmp";

fs::path& CrashDirectory() {
  static fs::path directory;
  return directory;
}

std::string IsoTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm utc{};
#ifdef _WIN32
  gmtime_s(&utc, &time);
#else
  gmtime_r(&time, &utc);
#endif
  std::ostringstream stream;
  stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << "Z";
  return stream.str();
}

std::string MakeCrashStem() {
  const auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  return std::string(kFilePrefix) + std::to_string(millis);
}

void ValidateCrashFileName(const std::string& file_name) {
  if (file_name.find('/') != std::string::npos ||
      file_name.find('\\') != std::string::npos ||
      file_name.find("..") != std::string::npos) {
    throw std::runtime_error("Invalid crash file name");
  }
  if (file_name.rfind(kFilePrefix, 0) != 0) {
    throw std::runtime_error("Invalid crash file name");
  }
  const size_t n = file_name.size();
  const bool ok_txt =
      n >= 4 && file_name.compare(n - 4, 4, ".txt") == 0;
  const bool ok_dmp =
      n >= 4 && file_name.compare(n - 4, 4, ".dmp") == 0;
  if (!ok_txt && !ok_dmp) {
    throw std::runtime_error("Invalid crash file name");
  }
}

void WriteTextReportFile(const fs::path& full_path,
                         const std::string& platform,
                         const std::string& type,
                         const std::string& message,
                         const std::string& stack_trace,
                         const std::string& extra_footer = {}) {
  std::ostringstream body;
  body << "=== HD Sentry Native Crash Report ===\n"
       << "platform: " << platform << '\n'
       << "timestamp: " << IsoTimestamp() << '\n'
       << "type: " << type << '\n'
       << "message: " << message << "\n\n"
       << "--- stack trace ---\n"
       << stack_trace;
  if (!extra_footer.empty()) {
    body << '\n' << extra_footer;
  }

  std::ofstream output(full_path, std::ios::trunc);
  output << body.str();
}

}  // namespace

void HdSentryCrashStore::ConfigureHdSentry() {
  if (!CrashDirectory().empty()) {
    return;
  }

#ifdef _WIN32
  PWSTR app_data = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr,
                                     &app_data))) {
    CrashDirectory() = fs::path(app_data) / kCrashDirName;
    CoTaskMemFree(app_data);
  } else {
    CrashDirectory() = fs::temp_directory_path() / kCrashDirName;
  }
#else
  if (const char* data_home = std::getenv("XDG_DATA_HOME")) {
    CrashDirectory() = fs::path(data_home) / kCrashDirName;
  } else if (const char* home = std::getenv("HOME")) {
    CrashDirectory() = fs::path(home) / ".local" / "share" / kCrashDirName;
  } else {
    CrashDirectory() = fs::temp_directory_path() / kCrashDirName;
  }
#endif

  std::error_code error;
  fs::create_directories(CrashDirectory(), error);
}

std::string HdSentryCrashStore::Directory() {
  ConfigureHdSentry();
  return CrashDirectory().string();
}

std::vector<std::string> HdSentryCrashStore::ListFileNames() {
  ConfigureHdSentry();
  std::vector<std::string> names;
  std::error_code error;
  for (const auto& entry : fs::directory_iterator(CrashDirectory(), error)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto name = entry.path().filename().string();
    if (name.rfind(kFilePrefix, 0) == 0 &&
        name.size() > strlen(kFileSuffix) &&
        name.compare(name.size() - strlen(kFileSuffix), strlen(kFileSuffix),
                     kFileSuffix) == 0) {
      names.push_back(name);
    }
  }
  std::sort(names.begin(), names.end(), std::greater<>());
  return names;
}

std::string HdSentryCrashStore::ReadFile(const std::string& file_name) {
  ValidateCrashFileName(file_name);
  const size_t n = file_name.size();
  if (n >= 4 && file_name.compare(n - 4, 4, ".dmp") == 0) {
    throw std::runtime_error(
        ".dmp is binary; open it with WinDbg from the crash directory.");
  }
  ConfigureHdSentry();
  std::ifstream input(CrashDirectory() / file_name);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool HdSentryCrashStore::DeleteFileHdSentry(const std::string& file_name) {
  ValidateCrashFileName(file_name);
  ConfigureHdSentry();
  std::error_code error;
  return fs::remove(CrashDirectory() / file_name, error);
}

void HdSentryCrashStore::ClearAll() {
  ConfigureHdSentry();
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(CrashDirectory(), ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto name = entry.path().filename().string();
    if (name.rfind(kFilePrefix, 0) != 0) {
      continue;
    }
    const size_t n = name.size();
    const bool ok_txt =
        n >= 4 && name.compare(n - 4, 4, ".txt") == 0;
    const bool ok_dmp =
        n >= 4 && name.compare(n - 4, 4, ".dmp") == 0;
    if (ok_txt || ok_dmp) {
      fs::remove(entry.path(), ec);
    }
  }
}

std::string HdSentryCrashStore::WriteReport(const std::string& platform,
                                            const std::string& type,
                                            const std::string& message,
                                            const std::string& stack_trace) {
  ConfigureHdSentry();
  const std::string stem = MakeCrashStem();
  const std::string file_name = stem + kFileSuffix;
  WriteTextReportFile(CrashDirectory() / file_name, platform, type, message,
                      stack_trace);
  return file_name;
}

#if defined(_WIN32)
std::string HdSentryCrashStore::WriteWindowsNativeCrash(
    void* exception_pointers,
    const std::string& stack_trace) {
  ConfigureHdSentry();
  const std::string stem = MakeCrashStem();
  const std::string txt_name = stem + kFileSuffix;
  WriteTextReportFile(CrashDirectory() / txt_name, "windows",
                      "unhandled_exception", "Unhandled native exception",
                      stack_trace,
                      std::string("--- minidump (WinDbg) ---\n") + stem +
                          kDumpSuffix);

  const fs::path dump_path = CrashDirectory() / (stem + kDumpSuffix);
  auto* ep = static_cast<EXCEPTION_POINTERS*>(exception_pointers);
  (void)WinWriteMiniDumpFile(dump_path, ep);

  return txt_name;
}
#endif

}  // namespace hd_sentry
