#include "include/crash_native_demo/crash_native_demo_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstdlib>
#include <cstring>

#include "crash_native_demo_plugin_private.h"

#define CRASH_NATIVE_DEMO_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), crash_native_demo_plugin_get_type(), \
                              CrashNativeDemoPlugin))

struct _CrashNativeDemoPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(CrashNativeDemoPlugin, crash_native_demo_plugin, g_object_get_type())

namespace {

[[noreturn]] void trigger_fatal_crash() {
  // SIGSEGV — terminates the process.
  volatile int* null_pointer = nullptr;
  *const_cast<int*>(null_pointer) = 0;
  std::abort();
}

}  // namespace

static void crash_native_demo_plugin_handle_method_call(
    CrashNativeDemoPlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "getPlatformVersion") == 0) {
    response = get_platform_version();
  } else if (strcmp(method, "crashNative") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
    fl_method_call_respond(method_call, response, nullptr);
    trigger_fatal_crash();
    return;
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

FlMethodResponse* get_platform_version() {
  struct utsname uname_data = {};
  uname(&uname_data);
  g_autofree gchar* version = g_strdup_printf("Linux %s", uname_data.version);
  g_autoptr(FlValue) result = fl_value_new_string(version);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static void crash_native_demo_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(crash_native_demo_plugin_parent_class)->dispose(object);
}

static void crash_native_demo_plugin_class_init(CrashNativeDemoPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = crash_native_demo_plugin_dispose;
}

static void crash_native_demo_plugin_init(CrashNativeDemoPlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  CrashNativeDemoPlugin* plugin = CRASH_NATIVE_DEMO_PLUGIN(user_data);
  crash_native_demo_plugin_handle_method_call(plugin, method_call);
}

void crash_native_demo_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  CrashNativeDemoPlugin* plugin = CRASH_NATIVE_DEMO_PLUGIN(
      g_object_new(crash_native_demo_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "crash_native_demo",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
