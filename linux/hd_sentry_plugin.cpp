#include "include/hd_sentry/hd_sentry_plugin.h"

#include <flutter_linux/flutter_linux.h>

#include "hd_sentry_host_api_impl.h"
#include "messages.g.h"

void hd_sentry_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  static hd_sentry::HdSentryHostApiImpl api;
  hd_sentry::HdSentryHostApi::SetUp(
      fl_plugin_registrar_get_messenger(registrar), &api);
}
