#include "include/hd_sentry/hd_sentry_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "hd_sentry_plugin.h"

void HdSentryPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  hd_sentry::HdSentryPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
