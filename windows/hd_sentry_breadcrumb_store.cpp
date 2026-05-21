#include "hd_sentry_breadcrumb_store.h"

#include "hd_sentry_crash_store.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace hd_sentry {
namespace {

namespace fs = std::filesystem;

fs::path BreadcrumbFilePath() {
  return fs::path(HdSentryCrashStore::Directory()) / kBreadcrumbFileName;
}

std::string IsoTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm utc{};
#ifdef _WIN32
  gmtime_s(&utc, &time);
#else
  gmtime_r(&time, &utc);
#endif
  std::ostringstream stream;
  stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S") << '.'
         << std::setfill('0') << std::setw(3) << (millis % 1000) << 'Z';
  return stream.str();
}

std::string SanitizeField(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (char c : value) {
    if (c == '\t' || c == '\n' || c == '\r') {
      out.push_back(' ');
    } else {
      out.push_back(c);
    }
  }
  return out;
}

void TrimToMaxLines(const fs::path& path) {
  std::ifstream input(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  if (static_cast<int>(lines.size()) <= kMaxBreadcrumbs) {
    return;
  }
  std::ofstream output(path, std::ios::trunc);
  const size_t start = lines.size() - kMaxBreadcrumbs;
  for (size_t i = start; i < lines.size(); ++i) {
    output << lines[i] << '\n';
  }
}

}  // namespace

void HdSentryBreadcrumbStore::Add(const std::string& message,
                                  const std::string* category,
                                  const std::string* data) {
  HdSentryCrashStore::ConfigureHdSentry();
  const fs::path path = BreadcrumbFilePath();
  std::ostringstream line;
  line << IsoTimestamp() << '\t'
       << SanitizeField(category != nullptr ? *category : "") << '\t'
       << SanitizeField(message) << '\t'
       << SanitizeField(data != nullptr ? *data : "") << '\n';
  std::ofstream output(path, std::ios::app);
  output << line.str();
  TrimToMaxLines(path);
}

void HdSentryBreadcrumbStore::Clear() {
  HdSentryCrashStore::ConfigureHdSentry();
  std::error_code ec;
  fs::remove(BreadcrumbFilePath(), ec);
}

std::string HdSentryBreadcrumbStore::ConsumeFormattedSection() {
  HdSentryCrashStore::ConfigureHdSentry();
  const fs::path path = BreadcrumbFilePath();
  std::ifstream input(path);
  if (!input.good()) {
    std::error_code ec;
    fs::remove(path, ec);
    return {};
  }
  std::ostringstream section;
  section << "\n--- breadcrumbs ---\n";
  std::string line;
  bool any = false;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    section << line << '\n';
    any = true;
  }
  std::error_code ec;
  fs::remove(path, ec);
  return any ? section.str() : std::string{};
}

}  // namespace hd_sentry
