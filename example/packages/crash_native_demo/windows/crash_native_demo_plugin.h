#ifndef FLUTTER_PLUGIN_CRASH_NATIVE_DEMO_PLUGIN_H_
#define FLUTTER_PLUGIN_CRASH_NATIVE_DEMO_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace crash_native_demo {

class CrashNativeDemoPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  CrashNativeDemoPlugin();

  virtual ~CrashNativeDemoPlugin();

  // Disallow copy and assign.
  CrashNativeDemoPlugin(const CrashNativeDemoPlugin&) = delete;
  CrashNativeDemoPlugin& operator=(const CrashNativeDemoPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace crash_native_demo

#endif  // FLUTTER_PLUGIN_CRASH_NATIVE_DEMO_PLUGIN_H_
