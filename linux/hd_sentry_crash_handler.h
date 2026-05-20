#ifndef HD_SENTRY_CRASH_HANDLER_H_
#define HD_SENTRY_CRASH_HANDLER_H_

#include <glib.h>

G_BEGIN_DECLS

void hd_sentry_crash_handler_install(void);

/** Signal handlers only (called from C++ install wrapper). */
void hd_sentry_crash_handler_install_signals(void);

G_END_DECLS

#endif  // HD_SENTRY_CRASH_HANDLER_H_
