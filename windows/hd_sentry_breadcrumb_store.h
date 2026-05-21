#ifndef HD_SENTRY_BREADCRUMB_STORE_H_
#define HD_SENTRY_BREADCRUMB_STORE_H_

#include <string>

namespace hd_sentry {

class HdSentryBreadcrumbStore {
 public:
  static constexpr const char* kBreadcrumbFileName = "session_breadcrumbs.log";
  static constexpr int kMaxBreadcrumbs = 100;

  static void Add(const std::string& message,
                  const std::string* category,
                  const std::string* data);
  static void Clear();
  /** Formatted `--- breadcrumbs ---` section; clears the log after read. */
  static std::string ConsumeFormattedSection();
};

}  // namespace hd_sentry

#endif  // HD_SENTRY_BREADCRUMB_STORE_H_
