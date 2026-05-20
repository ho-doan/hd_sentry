#define _GNU_SOURCE

#include "hd_sentry_linux_stack_trace.h"

#include <dlfcn.h>
#include <execinfo.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define HD_SENTRY_MAX_FRAMES 64

static gchar* format_with_backtrace_symbols(void* const* frames, gint count) {
  gchar** symbols = backtrace_symbols(frames, count);
  if (symbols == NULL) {
    return NULL;
  }

  GString* out = g_string_new(NULL);
  for (gint i = 0; i < count; ++i) {
    if (i > 0) {
      g_string_append_c(out, '\n');
    }
    g_string_append(out, symbols[i]);
  }
  free(symbols);
  return g_string_free(out, FALSE);
}

static gchar* format_with_dladdr(void* const* frames, gint count) {
  GString* out = g_string_new(NULL);
  for (gint i = 0; i < count; ++i) {
    Dl_info info;
    if (dladdr(frames[i], &info) != 0) {
      const uintptr_t frame_addr = (uintptr_t)frames[i];
      const uintptr_t sym_addr =
          info.dli_saddr != NULL ? (uintptr_t)info.dli_saddr : 0;
      const uintptr_t offset =
          sym_addr != 0 ? frame_addr - sym_addr : 0;
      const char* sym_name =
          info.dli_sname != NULL ? info.dli_sname : "??";
      const char* obj_name =
          info.dli_fname != NULL ? info.dli_fname : "??";
      g_string_append_printf(out, "#%d %p %s+0x%" PRIxPTR " (%s)\n", i,
                             frames[i], sym_name, offset, obj_name);
    } else {
      g_string_append_printf(out, "#%d %p ??\n", i, frames[i]);
    }
  }
  return g_string_free(out, FALSE);
}

gchar* hd_sentry_linux_stack_trace_capture_skip(gint skip) {
  void* frames[HD_SENTRY_MAX_FRAMES];
  const gint captured = backtrace(frames, HD_SENTRY_MAX_FRAMES);
  if (captured <= 0) {
    return g_strdup("(stack trace unavailable)");
  }

  const gint start = skip < captured ? skip : captured;
  const gint count = captured - start;
  if (count <= 0) {
    return g_strdup("(stack trace unavailable)");
  }
  void* const* slice = frames + start;

  gchar* symbols = format_with_backtrace_symbols(slice, count);
  if (symbols != NULL && symbols[0] != '\0') {
    return symbols;
  }
  g_free(symbols);

  return format_with_dladdr(slice, count);
}

gchar* hd_sentry_linux_stack_trace_capture(void) {
  return hd_sentry_linux_stack_trace_capture_skip(0);
}
