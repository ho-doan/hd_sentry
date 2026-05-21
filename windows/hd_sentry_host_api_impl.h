#ifndef HD_SENTRY_HOST_API_IMPL_H_
#define HD_SENTRY_HOST_API_IMPL_H_

#include "messages.g.h"

namespace hd_sentry {

class HdSentryHostApiImpl : public HdSentryHostApi {
 public:
  std::optional<FlutterError> Initialize() override;
  ErrorOr<std::string> GetCrashDirectory() override;
  ErrorOr<flutter::EncodableList> ListCrashFileNames() override;
  ErrorOr<std::string> ReadCrashFile(const std::string& file_name) override;
  ErrorOr<bool> DeleteCrashFile(const std::string& file_name) override;
  std::optional<FlutterError> ClearAllCrashFiles() override;
  std::optional<FlutterError> CaptureException(
      const std::string& message,
      const std::string* stack_trace) override;
  std::optional<FlutterError> AddBreadcrumb(
      const std::string& message,
      const std::string* category,
      const std::string* data) override;
  std::optional<FlutterError> ClearBreadcrumbs() override;
};

}  // namespace hd_sentry

#endif  // HD_SENTRY_HOST_API_IMPL_H_
