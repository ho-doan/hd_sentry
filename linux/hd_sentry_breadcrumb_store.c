#include "hd_sentry_breadcrumb_store.h"

#include "hd_sentry_crash_store.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static gchar* breadcrumb_file_path(void) {
  g_autofree gchar* directory = hd_sentry_crash_store_directory();
  return g_build_filename(directory, HD_SENTRY_BREADCRUMB_FILE_NAME, NULL);
}

static gchar* iso_timestamp(void) {
  const time_t now = time(NULL);
  struct tm utc;
  gmtime_r(&now, &utc);
  return g_strdup_printf("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                        utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                        utc.tm_hour, utc.tm_min, utc.tm_sec,
                        (int)((g_get_real_time() / 1000) % 1000));
}

static gchar* sanitize_field(const gchar* value) {
  if (value == NULL) {
    return g_strdup("");
  }
  GString* out = g_string_new(NULL);
  for (const gchar* p = value; *p != '\0'; ++p) {
    if (*p == '\t' || *p == '\n' || *p == '\r') {
      g_string_append_c(out, ' ');
    } else {
      g_string_append_c(out, *p);
    }
  }
  return g_string_free(out, FALSE);
}

static void trim_to_max_lines(const gchar* path) {
  g_autofree gchar* contents = NULL;
  gsize length = 0;
  if (!g_file_get_contents(path, &contents, &length, NULL)) {
    return;
  }
  gchar** lines = g_strsplit(contents, "\n", -1);
  gint count = g_strv_length(lines);
  gint non_empty = 0;
  for (gint i = 0; i < count; ++i) {
    if (lines[i][0] != '\0') {
      non_empty++;
    }
  }
  if (non_empty <= HD_SENTRY_MAX_BREADCRUMBS) {
    g_strfreev(lines);
    return;
  }
  GString* trimmed = g_string_new(NULL);
  gint kept = 0;
  for (gint i = count - 1; i >= 0 && kept < HD_SENTRY_MAX_BREADCRUMBS; --i) {
    if (lines[i][0] == '\0') {
      continue;
    }
    if (trimmed->len > 0) {
      g_string_prepend_c(trimmed, '\n');
    }
    g_string_prepend(trimmed, lines[i]);
    kept++;
  }
  g_string_append_c(trimmed, '\n');
  g_file_set_contents(path, trimmed->str, -1, NULL);
  g_strfreev(lines);
}

void hd_sentry_breadcrumb_store_add(const gchar* message,
                                   const gchar* category,
                                   const gchar* data) {
  hd_sentry_crash_store_configure();
  g_autofree gchar* path = breadcrumb_file_path();
  g_autofree gchar* timestamp = iso_timestamp();
  g_autofree gchar* cat = sanitize_field(category);
  g_autofree gchar* msg = sanitize_field(message != NULL ? message : "");
  g_autofree gchar* extra = sanitize_field(data);
  g_autofree gchar* line =
      g_strdup_printf("%s\t%s\t%s\t%s\n", timestamp, cat, msg, extra);

  FILE* file = fopen(path, "a");
  if (file == NULL) {
    return;
  }
  fputs(line, file);
  fclose(file);
  trim_to_max_lines(path);
}

void hd_sentry_breadcrumb_store_clear(void) {
  hd_sentry_crash_store_configure();
  g_autofree gchar* path = breadcrumb_file_path();
  remove(path);
}

gchar* hd_sentry_breadcrumb_store_consume_formatted_section(void) {
  hd_sentry_crash_store_configure();
  g_autofree gchar* path = breadcrumb_file_path();
  g_autofree gchar* contents = NULL;
  gsize length = 0;
  if (!g_file_get_contents(path, &contents, &length, NULL) || length == 0) {
    remove(path);
    return g_strdup("");
  }
  remove(path);

  GString* section = g_string_new(NULL);
  g_string_append(section, "\n--- breadcrumbs ---\n");
  gchar** lines = g_strsplit(contents, "\n", -1);
  for (gint i = 0; lines[i] != NULL; ++i) {
    if (lines[i][0] == '\0') {
      continue;
    }
    g_string_append(section, lines[i]);
    g_string_append_c(section, '\n');
  }
  g_strfreev(lines);
  return g_string_free(section, FALSE);
}
