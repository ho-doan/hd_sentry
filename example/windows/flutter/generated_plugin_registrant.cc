//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <crash_native_demo/crash_native_demo_plugin_c_api.h>
#include <hd_sentry/hd_sentry_plugin_c_api.h>

void RegisterPlugins(flutter::PluginRegistry* registry) {
  CrashNativeDemoPluginCApiRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("CrashNativeDemoPluginCApi"));
  HdSentryPluginCApiRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("HdSentryPluginCApi"));
}
