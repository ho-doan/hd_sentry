#include "hd_sentry_plugin.h"

#include <flutter/plugin_registrar_windows.h>

#include "hd_sentry_host_api_impl.h"
#include "messages.g.h"

namespace hd_sentry {

void HdSentryPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  HdSentryHostApi::SetUp(registrar->messenger(), new HdSentryHostApiImpl());
}

HdSentryPlugin::HdSentryPlugin() {}

HdSentryPlugin::~HdSentryPlugin() {}

}  // namespace hd_sentry
