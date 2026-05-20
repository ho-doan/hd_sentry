#include "include/hd_sentry/hd_sentry_plugin.h"

#include <flutter_linux/flutter_linux.h>

#include "hd_sentry_host_api_impl.h"

#define HD_SENTRY_PLUGIN(obj)                                         \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), hd_sentry_plugin_get_type(), \
                               HdSentryPlugin))

struct _HdSentryPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(HdSentryPlugin, hd_sentry_plugin, g_object_get_type())

static void hd_sentry_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(hd_sentry_plugin_parent_class)->dispose(object);
}

static void hd_sentry_plugin_class_init(HdSentryPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = hd_sentry_plugin_dispose;
}

static void hd_sentry_plugin_init(HdSentryPlugin* self) {
  (void)self;
}

void hd_sentry_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  HdSentryPlugin* plugin = HD_SENTRY_PLUGIN(
      g_object_new(hd_sentry_plugin_get_type(), nullptr));

  hd_sentry_host_api_impl_register(fl_plugin_registrar_get_messenger(registrar));

  g_object_unref(plugin);
}
