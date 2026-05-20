//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <crash_native_demo/crash_native_demo_plugin.h>
#include <hd_sentry/hd_sentry_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) crash_native_demo_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "CrashNativeDemoPlugin");
  crash_native_demo_plugin_register_with_registrar(crash_native_demo_registrar);
  g_autoptr(FlPluginRegistrar) hd_sentry_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "HdSentryPlugin");
  hd_sentry_plugin_register_with_registrar(hd_sentry_registrar);
}
