#include "hd_sentry_host_api_impl.h"

#include "hd_sentry_breadcrumb_store.h"
#include "hd_sentry_crash_handler.h"
#include "hd_sentry_crash_store.h"
#include "messages.g.h"

static hd_sentryHdSentryHostApiInitializeResponse* host_api_initialize(
    gpointer user_data) {
  (void)user_data;
  hd_sentry_crash_handler_install();
  return hd_sentry_hd_sentry_host_api_initialize_response_new();
}

static hd_sentryHdSentryHostApiGetCrashDirectoryResponse* host_api_get_crash_directory(
    gpointer user_data) {
  (void)user_data;
  g_autofree gchar* directory = hd_sentry_crash_store_directory();
  return hd_sentry_hd_sentry_host_api_get_crash_directory_response_new(directory);
}

static hd_sentryHdSentryHostApiListCrashFileNamesResponse* host_api_list_crash_file_names(
    gpointer user_data) {
  (void)user_data;
  gint count = 0;
  gchar** names = hd_sentry_crash_store_list_file_names(&count);

  g_autoptr(FlValue) list = fl_value_new_list();
  if (names != NULL) {
    for (gint i = 0; i < count; ++i) {
      fl_value_append_take(list, fl_value_new_string(names[i]));
    }
    g_strfreev(names);
  }
  return hd_sentry_hd_sentry_host_api_list_crash_file_names_response_new(list);
}

static hd_sentryHdSentryHostApiReadCrashFileResponse* host_api_read_crash_file(
    const gchar* file_name,
    gpointer user_data) {
  (void)user_data;
  GError* error = NULL;
  gchar* contents = hd_sentry_crash_store_read_file(file_name, &error);
  if (contents == NULL) {
    return hd_sentry_hd_sentry_host_api_read_crash_file_response_new_error(
        "read_error", error != NULL ? error->message : "Failed to read crash file",
        NULL);
  }
  g_clear_error(&error);
  hd_sentryHdSentryHostApiReadCrashFileResponse* response =
      hd_sentry_hd_sentry_host_api_read_crash_file_response_new(contents);
  g_free(contents);
  return response;
}

static hd_sentryHdSentryHostApiDeleteCrashFileResponse* host_api_delete_crash_file(
    const gchar* file_name,
    gpointer user_data) {
  (void)user_data;
  GError* error = NULL;
  const gboolean deleted =
      hd_sentry_crash_store_delete_file(file_name, &error);
  if (error != NULL) {
    hd_sentryHdSentryHostApiDeleteCrashFileResponse* response =
        hd_sentry_hd_sentry_host_api_delete_crash_file_response_new_error(
            "delete_error", error->message, NULL);
    g_clear_error(&error);
    return response;
  }
  return hd_sentry_hd_sentry_host_api_delete_crash_file_response_new(deleted);
}

static hd_sentryHdSentryHostApiClearAllCrashFilesResponse* host_api_clear_all_crash_files(
    gpointer user_data) {
  (void)user_data;
  hd_sentry_crash_store_clear_all();
  return hd_sentry_hd_sentry_host_api_clear_all_crash_files_response_new();
}

static hd_sentryHdSentryHostApiCaptureExceptionResponse* host_api_capture_exception(
    const gchar* message,
    const gchar* stack_trace,
    gpointer user_data) {
  (void)user_data;
  hd_sentry_crash_store_write_report(
      "linux", "flutter_error", message != NULL ? message : "",
      stack_trace != NULL ? stack_trace : "");
  return hd_sentry_hd_sentry_host_api_capture_exception_response_new();
}

static hd_sentryHdSentryHostApiAddBreadcrumbResponse* host_api_add_breadcrumb(
    const gchar* message,
    const gchar* category,
    const gchar* data,
    gpointer user_data) {
  (void)user_data;
  hd_sentry_breadcrumb_store_add(message, category, data);
  return hd_sentry_hd_sentry_host_api_add_breadcrumb_response_new();
}

static hd_sentryHdSentryHostApiClearBreadcrumbsResponse* host_api_clear_breadcrumbs(
    gpointer user_data) {
  (void)user_data;
  hd_sentry_breadcrumb_store_clear();
  return hd_sentry_hd_sentry_host_api_clear_breadcrumbs_response_new();
}

static const hd_sentryHdSentryHostApiVTable kHostApiVTable = {
    .initialize = host_api_initialize,
    .get_crash_directory = host_api_get_crash_directory,
    .list_crash_file_names = host_api_list_crash_file_names,
    .read_crash_file = host_api_read_crash_file,
    .delete_crash_file = host_api_delete_crash_file,
    .clear_all_crash_files = host_api_clear_all_crash_files,
    .capture_exception = host_api_capture_exception,
    .add_breadcrumb = host_api_add_breadcrumb,
    .clear_breadcrumbs = host_api_clear_breadcrumbs,
};

void hd_sentry_host_api_impl_register(FlBinaryMessenger* messenger) {
  hd_sentry_hd_sentry_host_api_set_method_handlers(messenger, NULL, &kHostApiVTable,
                                                   NULL, NULL);
}
