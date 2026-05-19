#include "hd_sentry_crash_store.h"

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

void ValidateFileName(const std::string& file_name) {
  if (file_name.find('/') != std::string::npos ||
      file_name.find('\\') != std::string::npos ||
      file_name.find("..") != std::string::npos) {
    throw std::runtime_error("Invalid crash file name");
  }
}

}  // namespace

void HdSentryCrashStore::Configure() {
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
  Configure();
  return CrashDirectory().string();
}

std::vector<std::string> HdSentryCrashStore::ListFileNames() {
  Configure();
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
  ValidateFileName(file_name);
  Configure();
  std::ifstream input(CrashDirectory() / file_name);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool HdSentryCrashStore::DeleteFile(const std::string& file_name) {
  ValidateFileName(file_name);
  Configure();
  std::error_code error;
  return fs::remove(CrashDirectory() / file_name, error);
}

void HdSentryCrashStore::ClearAll() {
  for (const auto& name : ListFileNames()) {
    DeleteFile(name);
  }
}

std::string HdSentryCrashStore::WriteReport(const std::string& platform,
                                            const std::string& type,
                                            const std::string& message,
                                            const std::string& stack_trace) {
  Configure();
  const auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  const std::string file_name =
      std::string(kFilePrefix) + std::to_string(millis) + kFileSuffix;

  std::ostringstream body;
  body << "=== HD Sentry Native Crash Report ===\n"
       << "platform: " << platform << '\n'
       << "timestamp: " << IsoTimestamp() << '\n'
       << "type: " << type << '\n'
       << "message: " << message << "\n\n"
       << "--- stack trace ---\n"
       << stack_trace;

  std::ofstream output(CrashDirectory() / file_name, std::ios::trunc);
  output << body.str();
  return file_name;
}

}  // namespace hd_sentry
