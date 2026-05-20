#include "crash_native_demo_plugin.h"

#include <windows.h>

#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

namespace crash_native_demo {

namespace {

[[noreturn]] void TriggerFatalCrash() {
  // Access violation — terminates the process immediately.
  volatile int* null_pointer = nullptr;
  *const_cast<int*>(null_pointer) = 0;
  abort();
}

}  // namespace

void CrashNativeDemoPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "crash_native_demo",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<CrashNativeDemoPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

CrashNativeDemoPlugin::CrashNativeDemoPlugin() {}

CrashNativeDemoPlugin::~CrashNativeDemoPlugin() {}

void CrashNativeDemoPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("getPlatformVersion") == 0) {
    std::ostringstream version_stream;
    version_stream << "Windows ";
    if (IsWindows10OrGreater()) {
      version_stream << "10+";
    } else if (IsWindows8OrGreater()) {
      version_stream << "8";
    } else if (IsWindows7OrGreater()) {
      version_stream << "7";
    }
    result->Success(flutter::EncodableValue(version_stream.str()));
  } else if (method_call.method_name().compare("crashNative") == 0) {
    result->Success(flutter::EncodableValue());
    TriggerFatalCrash();
  } else {
    result->NotImplemented();
  }
}

}  // namespace crash_native_demo
