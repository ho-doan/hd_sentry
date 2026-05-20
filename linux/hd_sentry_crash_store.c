#include "hd_sentry_crash_store.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define HD_SENTRY_CRASH_DIR_NAME "hd_sentry_crashes"
#define HD_SENTRY_FILE_PREFIX "crash_"
#define HD_SENTRY_FILE_SUFFIX ".txt"

static gchar* g_crash_directory = NULL;

static void ensure_configured(void) {
  hd_sentry_crash_store_configure();
}

static int compare_filenames_desc(const void* a, const void* b) {
  const char* const* sa = a;
  const char* const* sb = b;
  return strcmp(*sb, *sa);
}

static gboolean validate_file_name(const gchar* file_name, GError** error) {
  if (file_name == NULL || file_name[0] == '\0') {
    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Empty file name");
    return FALSE;
  }
  if (strstr(file_name, "/") != NULL || strstr(file_name, "\\") != NULL ||
      strstr(file_name, "..") != NULL) {
    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid crash file name");
    return FALSE;
  }
  if (strncmp(file_name, HD_SENTRY_FILE_PREFIX, strlen(HD_SENTRY_FILE_PREFIX)) != 0) {
    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid crash file name");
    return FALSE;
  }
  const gsize n = strlen(file_name);
  const gboolean ok_txt = n >= 4 && strcmp(file_name + n - 4, ".txt") == 0;
  const gboolean ok_dmp = n >= 4 && strcmp(file_name + n - 4, ".dmp") == 0;
  if (!ok_txt && !ok_dmp) {
    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid crash file name");
    return FALSE;
  }
  return TRUE;
}

static gchar* iso_timestamp(void) {
  const time_t now = time(NULL);
  struct tm utc;
  gmtime_r(&now, &utc);
  return g_strdup_printf("%04d-%02d-%02dT%02d:%02d:%02dZ", utc.tm_year + 1900,
                        utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min,
                        utc.tm_sec);
}

static gchar* make_crash_stem(void) {
  const gint64 millis = g_get_real_time() / 1000;
  return g_strdup_printf("%s%" G_GINT64_FORMAT, HD_SENTRY_FILE_PREFIX, millis);
}

static gboolean write_text_report(const gchar* full_path,
                                const gchar* platform,
                                const gchar* type,
                                const gchar* message,
                                const gchar* stack_trace) {
  g_autofree gchar* timestamp = iso_timestamp();
  FILE* file = fopen(full_path, "w");
  if (file == NULL) {
    return FALSE;
  }
  fprintf(file,
          "=== HD Sentry Native Crash Report ===\n"
          "platform: %s\n"
          "timestamp: %s\n"
          "type: %s\n"
          "message: %s\n\n"
          "--- stack trace ---\n"
          "%s\n",
          platform, timestamp, type, message,
          stack_trace != NULL ? stack_trace : "");
  fclose(file);
  return TRUE;
}

void hd_sentry_crash_store_configure(void) {
  if (g_crash_directory != NULL) {
    return;
  }

  const gchar* data_home = g_getenv("XDG_DATA_HOME");
  if (data_home != NULL && data_home[0] != '\0') {
    g_crash_directory =
        g_build_filename(data_home, HD_SENTRY_CRASH_DIR_NAME, NULL);
  } else {
    const gchar* home = g_getenv("HOME");
    if (home != NULL && home[0] != '\0') {
      g_crash_directory = g_build_filename(home, ".local", "share",
                                           HD_SENTRY_CRASH_DIR_NAME, NULL);
    } else {
      g_crash_directory =
          g_build_filename(g_get_tmp_dir(), HD_SENTRY_CRASH_DIR_NAME, NULL);
    }
  }

  g_mkdir_with_parents(g_crash_directory, 0755);
}

gchar* hd_sentry_crash_store_directory(void) {
  ensure_configured();
  return g_strdup(g_crash_directory);
}

gchar** hd_sentry_crash_store_list_file_names(gint* out_count) {
  ensure_configured();
  *out_count = 0;

  DIR* dir = opendir(g_crash_directory);
  if (dir == NULL) {
    return NULL;
  }

  GPtrArray* names = g_ptr_array_new_with_free_func(g_free);
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
      continue;
    }
    const gchar* name = entry->d_name;
    const gsize prefix_len = strlen(HD_SENTRY_FILE_PREFIX);
    const gsize suffix_len = strlen(HD_SENTRY_FILE_SUFFIX);
    const gsize name_len = strlen(name);
    if (name_len <= prefix_len + suffix_len) {
      continue;
    }
    if (strncmp(name, HD_SENTRY_FILE_PREFIX, prefix_len) != 0) {
      continue;
    }
    if (strcmp(name + name_len - suffix_len, HD_SENTRY_FILE_SUFFIX) != 0) {
      continue;
    }
    g_ptr_array_add(names, g_strdup(name));
  }
  closedir(dir);

  if (names->len > 1) {
    qsort(names->pdata, names->len, sizeof(gpointer), compare_filenames_desc);
  }

  gsize len = 0;
  gchar** result = (gchar**)g_ptr_array_steal(names, &len);
  g_ptr_array_free(names, TRUE);
  *out_count = (gint)len;
  return result;
}

gchar* hd_sentry_crash_store_read_file(const gchar* file_name, GError** error) {
  if (!validate_file_name(file_name, error)) {
    return NULL;
  }
  const gsize n = strlen(file_name);
  if (n >= 4 && strcmp(file_name + n - 4, ".dmp") == 0) {
    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                ".dmp is binary; open it with a debugger from the crash directory.");
    return NULL;
  }

  ensure_configured();
  g_autofree gchar* path =
      g_build_filename(g_crash_directory, file_name, NULL);

  gchar* contents = NULL;
  gsize length = 0;
  if (!g_file_get_contents(path, &contents, &length, error)) {
    return NULL;
  }
  return contents;
}

gboolean hd_sentry_crash_store_delete_file(const gchar* file_name,
                                           GError** error) {
  if (!validate_file_name(file_name, error)) {
    return FALSE;
  }
  ensure_configured();
  g_autofree gchar* path =
      g_build_filename(g_crash_directory, file_name, NULL);
  if (remove(path) != 0) {
    if (errno == ENOENT) {
      return FALSE;
    }
    g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Failed to delete crash file: %s", strerror(errno));
    return FALSE;
  }
  return TRUE;
}

void hd_sentry_crash_store_clear_all(void) {
  ensure_configured();
  DIR* dir = opendir(g_crash_directory);
  if (dir == NULL) {
    return;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
      continue;
    }
    const gchar* name = entry->d_name;
    if (strncmp(name, HD_SENTRY_FILE_PREFIX, strlen(HD_SENTRY_FILE_PREFIX)) != 0) {
      continue;
    }
    const gsize n = strlen(name);
    const gboolean ok_txt = n >= 4 && strcmp(name + n - 4, ".txt") == 0;
    const gboolean ok_dmp = n >= 4 && strcmp(name + n - 4, ".dmp") == 0;
    if (!ok_txt && !ok_dmp) {
      continue;
    }
    g_autofree gchar* path =
        g_build_filename(g_crash_directory, name, NULL);
    remove(path);
  }
  closedir(dir);
}

gchar* hd_sentry_crash_store_write_report(const gchar* platform,
                                          const gchar* type,
                                          const gchar* message,
                                          const gchar* stack_trace) {
  ensure_configured();
  g_autofree gchar* stem = make_crash_stem();
  g_autofree gchar* file_name =
      g_strconcat(stem, HD_SENTRY_FILE_SUFFIX, NULL);
  g_autofree gchar* full_path =
      g_build_filename(g_crash_directory, file_name, NULL);

  write_text_report(full_path, platform, type, message, stack_trace);
  return g_strdup(file_name);
}
