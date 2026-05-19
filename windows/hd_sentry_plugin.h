#ifndef FLUTTER_PLUGIN_HD_SENTRY_PLUGIN_H_
#define FLUTTER_PLUGIN_HD_SENTRY_PLUGIN_H_

#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace hd_sentry {

class HdSentryPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  HdSentryPlugin();
  virtual ~HdSentryPlugin();
};

}  // namespace hd_sentry

#endif  // FLUTTER_PLUGIN_HD_SENTRY_PLUGIN_H_
