#ifndef HD_SENTRY_LINUX_STACK_TRACE_H_
#define HD_SENTRY_LINUX_STACK_TRACE_H_

#include <glib.h>

G_BEGIN_DECLS

/** Caller must g_free the result. */
gchar* hd_sentry_linux_stack_trace_capture(void);

/** Like [hd_sentry_linux_stack_trace_capture] but skips the first @skip frames. */
gchar* hd_sentry_linux_stack_trace_capture_skip(gint skip);

G_END_DECLS

#endif  // HD_SENTRY_LINUX_STACK_TRACE_H_
