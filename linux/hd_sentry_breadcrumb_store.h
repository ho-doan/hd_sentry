#ifndef HD_SENTRY_BREADCRUMB_STORE_H_
#define HD_SENTRY_BREADCRUMB_STORE_H_

#include <glib.h>

G_BEGIN_DECLS

#define HD_SENTRY_BREADCRUMB_FILE_NAME "session_breadcrumbs.log"
#define HD_SENTRY_MAX_BREADCRUMBS 100

void hd_sentry_breadcrumb_store_add(const gchar* message,
                                   const gchar* category,
                                   const gchar* data);

void hd_sentry_breadcrumb_store_clear(void);

/**
 * Returns formatted `--- breadcrumbs ---` section (may be empty).
 * Clears the breadcrumb log after read. Caller must g_free the result.
 */
gchar* hd_sentry_breadcrumb_store_consume_formatted_section(void);

G_END_DECLS

#endif  // HD_SENTRY_BREADCRUMB_STORE_H_
