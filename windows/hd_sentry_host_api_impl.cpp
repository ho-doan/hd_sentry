#include "hd_sentry_host_api_impl.h"

#include "hd_sentry_breadcrumb_store.h"
#include "hd_sentry_crash_handler.h"
#include "hd_sentry_crash_store.h"

namespace hd_sentry {

std::optional<FlutterError> HdSentryHostApiImpl::Initialize() {
  HdSentryCrashHandler::Install();
  return std::nullopt;
}

ErrorOr<std::string> HdSentryHostApiImpl::GetCrashDirectory() {
  return HdSentryCrashStore::Directory();
}

ErrorOr<flutter::EncodableList> HdSentryHostApiImpl::ListCrashFileNames() {
  flutter::EncodableList names;
  for (const auto& name : HdSentryCrashStore::ListFileNames()) {
    names.emplace_back(name);
  }
  return names;
}

ErrorOr<std::string> HdSentryHostApiImpl::ReadCrashFile(
    const std::string& file_name) {
  return HdSentryCrashStore::ReadFile(file_name);
}

ErrorOr<bool> HdSentryHostApiImpl::DeleteCrashFile(
    const std::string& file_name) {
  return HdSentryCrashStore::DeleteFileHdSentry(file_name);
}

std::optional<FlutterError> HdSentryHostApiImpl::ClearAllCrashFiles() {
  HdSentryCrashStore::ClearAll();
  return std::nullopt;
}

std::optional<FlutterError> HdSentryHostApiImpl::CaptureException(
    const std::string& message,
    const std::string* stack_trace) {
  HdSentryCrashStore::WriteReport(
      "windows", "flutter_error", message,
      stack_trace != nullptr ? *stack_trace : "");
  return std::nullopt;
}

std::optional<FlutterError> HdSentryHostApiImpl::AddBreadcrumb(
    const std::string& message,
    const std::string* category,
    const std::string* data) {
  HdSentryBreadcrumbStore::Add(message, category, data);
  return std::nullopt;
}

std::optional<FlutterError> HdSentryHostApiImpl::ClearBreadcrumbs() {
  HdSentryBreadcrumbStore::Clear();
  return std::nullopt;
}

}  // namespace hd_sentry
