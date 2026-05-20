#include "include/crash_native_demo/crash_native_demo_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "crash_native_demo_plugin.h"

void CrashNativeDemoPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  crash_native_demo::CrashNativeDemoPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
