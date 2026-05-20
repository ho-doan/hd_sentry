#ifndef HD_SENTRY_CRASH_STORE_H_
#define HD_SENTRY_CRASH_STORE_H_

#include <glib.h>

G_BEGIN_DECLS

void hd_sentry_crash_store_configure(void);

/** Caller must g_free the result. */
gchar* hd_sentry_crash_store_directory(void);

/** Sorted newest first. Caller must free each string and the array with g_strfreev. */
gchar** hd_sentry_crash_store_list_file_names(gint* out_count);

/** Caller must g_free the result. Returns NULL and sets @error on failure. */
gchar* hd_sentry_crash_store_read_file(const gchar* file_name, GError** error);

gboolean hd_sentry_crash_store_delete_file(const gchar* file_name,
                                           GError** error);

void hd_sentry_crash_store_clear_all(void);

/** Caller must g_free the returned file name. */
gchar* hd_sentry_crash_store_write_report(const gchar* platform,
                                          const gchar* type,
                                          const gchar* message,
                                          const gchar* stack_trace);

G_END_DECLS

#endif  // HD_SENTRY_CRASH_STORE_H_
