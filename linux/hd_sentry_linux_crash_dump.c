#define _GNU_SOURCE

#include "hd_sentry_linux_crash_dump.h"
#include "hd_sentry_linux_elfcore_types.h"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define HD_SENTRY_DUMP_SUFFIX ".dmp"
#define HD_SENTRY_MAPS_SUFFIX ".maps"
#define HD_SENTRY_GDB_SUFFIX ".gdb"
#define HD_SENTRY_CORE_BUF_SIZE (512 * 1024)
#define HD_SENTRY_STACK_CAPTURE_BYTES (16 * 1024)
#define HD_SENTRY_NT_FILE_MAX 48
#define HD_SENTRY_NT_FILE_BUF_SIZE (32 * 1024)
static uint8_t g_core_buffer[HD_SENTRY_CORE_BUF_SIZE];

#if defined(__x86_64__) || defined(__aarch64__)
static HdSentryElfPrstatus g_prstatus;
static HdSentryElfPrpsinfo g_prpsinfo;
static uint8_t g_nt_file_buffer[HD_SENTRY_NT_FILE_BUF_SIZE];

typedef char HdSentryPrstatusLayoutCheck
    [(offsetof(HdSentryElfPrstatus, pr_reg) ==
      HD_SENTRY_ELF_PRSTATUS_REG_OFFSET)
         ? 1
         : -1];
#endif

static size_t align4(size_t value) {
  return (value + 3U) & ~((size_t)3U);
}

static gboolean copy_proc_file(const char* src_path, const char* dest_path) {
  const int in_fd = open(src_path, O_RDONLY);
  if (in_fd < 0) {
    return FALSE;
  }

  const int out_fd =
      open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)0644);
  if (out_fd < 0) {
    close(in_fd);
    return FALSE;
  }

  char buffer[4096];
  ssize_t read_bytes = 0;
  while ((read_bytes = read(in_fd, buffer, sizeof(buffer))) > 0) {
    ssize_t written = 0;
    while (written < read_bytes) {
      const ssize_t n =
          write(out_fd, buffer + written, (size_t)(read_bytes - written));
      if (n < 0) {
        close(in_fd);
        close(out_fd);
        return FALSE;
      }
      written += n;
    }
  }

  close(in_fd);
  close(out_fd);
  return read_bytes == 0;
}

void hd_sentry_linux_crash_dump_configure(void) {
  prctl(PR_SET_DUMPABLE, 1);

  struct rlimit core_limit;
  core_limit.rlim_cur = RLIM_INFINITY;
  core_limit.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &core_limit);
}

#if defined(__x86_64__) || defined(__aarch64__)

static gboolean buffer_append(size_t* offset, const void* data, size_t size) {
  if (*offset + size > HD_SENTRY_CORE_BUF_SIZE) {
    return FALSE;
  }
  memcpy(g_core_buffer + *offset, data, size);
  *offset += size;
  return TRUE;
}

static gboolean buffer_append_note(size_t* offset, const char* name, int type,
                                   const void* desc, size_t desc_size) {
  Elf64_Nhdr nhdr;
  nhdr.n_namesz = (Elf64_Word)(strlen(name) + 1);
  nhdr.n_descsz = (Elf64_Word)desc_size;
  nhdr.n_type = (Elf64_Word)type;
  if (!buffer_append(offset, &nhdr, sizeof(nhdr)) ||
      !buffer_append(offset, name, nhdr.n_namesz)) {
    return FALSE;
  }
  *offset = align4(*offset);
  if (!buffer_append(offset, desc, desc_size)) {
    return FALSE;
  }
  *offset = align4(*offset);
  return TRUE;
}

#if defined(__x86_64__)

static gboolean fill_prstatus(const ucontext_t* uc, HdSentryElfPrstatus* pr) {
  memset(pr, 0, sizeof(*pr));
  pr->pr_pid = getpid();
  if (uc == NULL) {
    return FALSE;
  }
  memcpy(&pr->pr_reg, &uc->uc_mcontext.gregs, sizeof(pr->pr_reg));
  return TRUE;
}

#elif defined(__aarch64__)

static gboolean fill_prstatus(const ucontext_t* uc, HdSentryElfPrstatus* pr) {
  memset(pr, 0, sizeof(*pr));
  pr->pr_pid = getpid();
  if (uc == NULL) {
    return FALSE;
  }
  memcpy(&pr->pr_reg, &uc->uc_mcontext, sizeof(pr->pr_reg));
  return TRUE;
}

#endif

static void fill_prpsinfo(void) {
  memset(&g_prpsinfo, 0, sizeof(g_prpsinfo));
  g_prpsinfo.pr_pid = getpid();
  g_prpsinfo.pr_ppid = getppid();
  g_prpsinfo.pr_pgrp = getpgrp();
  g_prpsinfo.pr_sid = getsid(0);

  char exe_path[PATH_MAX];
  const ssize_t exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (exe_len > 0) {
    exe_path[exe_len] = '\0';
    const char* base = strrchr(exe_path, '/');
    base = base != NULL ? base + 1 : exe_path;
    snprintf(g_prpsinfo.pr_fname, sizeof(g_prpsinfo.pr_fname), "%s", base);
    snprintf(g_prpsinfo.pr_psargs, sizeof(g_prpsinfo.pr_psargs), "%s", exe_path);
  }
}

static size_t build_nt_file_note(void) {
  const int maps_fd = open("/proc/self/maps", O_RDONLY);
  if (maps_fd < 0) {
    return 0;
  }

  typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t file_ofs;
    char path[256];
  } MapEntry;

  MapEntry entries[HD_SENTRY_NT_FILE_MAX];
  size_t entry_count = 0;

  char line[512];
  size_t line_len = 0;
  char ch = '\0';
  while (entry_count < HD_SENTRY_NT_FILE_MAX &&
         read(maps_fd, &ch, 1) == 1) {
    if (ch != '\n') {
      if (line_len + 1 < sizeof(line)) {
        line[line_len++] = ch;
      }
      continue;
    }
    line[line_len] = '\0';
    line_len = 0;

    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t file_ofs = 0;
    char perms[8] = {0};
    char path[256] = {0};
    unsigned long long start_ull = 0;
    unsigned long long end_ull = 0;
    unsigned long long file_ofs_ull = 0;
    const int parsed =
        sscanf(line, "%llx-%llx %7s %llx %*s %*s %255[^\n]", &start_ull,
               &end_ull, perms, &file_ofs_ull, path);
    start = (uint64_t)start_ull;
    end = (uint64_t)end_ull;
    file_ofs = (uint64_t)file_ofs_ull;
    if (parsed < 4 || path[0] == '\0') {
      continue;
    }

    entries[entry_count].start = start;
    entries[entry_count].end = end;
    entries[entry_count].file_ofs = file_ofs >> 12;
    snprintf(entries[entry_count].path, sizeof(entries[entry_count].path), "%s",
             path);
    entry_count++;
  }
  close(maps_fd);

  if (entry_count == 0) {
    return 0;
  }

  size_t names_size = 0;
  for (size_t i = 0; i < entry_count; ++i) {
    names_size += strlen(entries[i].path) + 1;
  }

  const size_t header_size =
      sizeof(uint64_t) + entry_count * 3 * sizeof(uint64_t);
  const size_t total = header_size + names_size;
  if (total > HD_SENTRY_NT_FILE_BUF_SIZE) {
    return 0;
  }

  memset(g_nt_file_buffer, 0, total);
  uint8_t* cursor = g_nt_file_buffer;
  const uint64_t count = (uint64_t)entry_count;
  memcpy(cursor, &count, sizeof(count));
  cursor += sizeof(count);

  for (size_t i = 0; i < entry_count; ++i) {
    memcpy(cursor, &entries[i].start, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
    memcpy(cursor, &entries[i].end, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
    memcpy(cursor, &entries[i].file_ofs, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
  }

  for (size_t i = 0; i < entry_count; ++i) {
    const size_t path_len = strlen(entries[i].path) + 1;
    memcpy(cursor, entries[i].path, path_len);
    cursor += path_len;
  }

  return total;
}

#if defined(__x86_64__)

static gboolean stack_capture_from_uc(const ucontext_t* uc, uintptr_t* start,
                                      size_t* length) {
  const uintptr_t sp = (uintptr_t)uc->uc_mcontext.gregs[REG_RSP];
  if (sp == 0) {
    return FALSE;
  }
  const uintptr_t capture_start =
      sp > HD_SENTRY_STACK_CAPTURE_BYTES / 2
          ? sp - HD_SENTRY_STACK_CAPTURE_BYTES / 2
          : 0;
  *start = capture_start;
  *length = HD_SENTRY_STACK_CAPTURE_BYTES;
  return TRUE;
}

#elif defined(__aarch64__)

static gboolean stack_capture_from_uc(const ucontext_t* uc, uintptr_t* start,
                                      size_t* length) {
  const uintptr_t sp = (uintptr_t)uc->uc_mcontext.sp;
  if (sp == 0) {
    return FALSE;
  }
  const uintptr_t capture_start =
      sp > HD_SENTRY_STACK_CAPTURE_BYTES / 2
          ? sp - HD_SENTRY_STACK_CAPTURE_BYTES / 2
          : 0;
  *start = capture_start;
  *length = HD_SENTRY_STACK_CAPTURE_BYTES;
  return TRUE;
}

#endif

static gboolean write_gdb_script(const char* crash_dir, const char* stem,
                                 const char* dump_file_name) {
  char exe_path[PATH_MAX];
  const ssize_t exe_len =
      readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (exe_len <= 0) {
    return FALSE;
  }
  exe_path[exe_len] = '\0';

  char bundle_dir[PATH_MAX];
  snprintf(bundle_dir, sizeof(bundle_dir), "%s", exe_path);
  char* last_slash = strrchr(bundle_dir, '/');
  if (last_slash != NULL) {
    *last_slash = '\0';
  }

  g_autofree gchar* script_path =
      g_strdup_printf("%s/%s%s", crash_dir, stem, HD_SENTRY_GDB_SUFFIX);
  FILE* script = fopen(script_path, "w");
  if (script == NULL) {
    return FALSE;
  }

  fprintf(script,
          "# Auto-generated by hd_sentry — run: gdb -x \"%s\" \"%s\"\n"
          "set pagination off\n"
          "set print pretty on\n"
          "set solib-search-path %s/lib:%s\n"
          "info proc mappings\n"
          "bt full\n"
          "info registers\n"
          "x/8i $pc\n",
          script_path, exe_path, bundle_dir, bundle_dir);
  if (dump_file_name != NULL) {
    g_autofree gchar* core_path =
        g_strdup_printf("%s/%s", crash_dir, dump_file_name);
    fprintf(script, "# core already loaded via: gdb %s %s\n", exe_path,
            core_path);
  }
  fclose(script);
  return TRUE;
}

static gboolean write_file_bytes(const char* path, const void* data,
                                 size_t size) {
  const int out_fd =
      open(path, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)0644);
  if (out_fd < 0) {
    return FALSE;
  }

  const uint8_t* bytes = data;
  size_t written_total = 0;
  while (written_total < size) {
    const ssize_t n =
        write(out_fd, bytes + written_total, size - written_total);
    if (n < 0) {
      close(out_fd);
      unlink(path);
      return FALSE;
    }
    written_total += (size_t)n;
  }
  close(out_fd);
  return TRUE;
}

static gboolean write_elf_core(const char* path, gint signo,
                               const siginfo_t* info, const ucontext_t* uc) {
  memset(&g_prstatus, 0, sizeof(g_prstatus));
  g_prstatus.pr_pid = getpid();
  (void)fill_prstatus(uc, &g_prstatus);
  if (signo > 0) {
    g_prstatus.pr_cursig = (short)signo;
    g_prstatus.pr_info.si_signo = signo;
  }

  fill_prpsinfo();

  HdSentryElfSiginfo elf_siginfo;
  memset(&elf_siginfo, 0, sizeof(elf_siginfo));
  if (info != NULL) {
    elf_siginfo.si_signo = info->si_signo;
    elf_siginfo.si_code = info->si_code;
    elf_siginfo.si_errno = info->si_errno;
  } else if (signo > 0) {
    elf_siginfo.si_signo = signo;
  }

  const size_t nt_file_size = build_nt_file_note();

  uintptr_t stack_start = 0;
  size_t stack_length = 0;
  const gboolean has_stack =
      uc != NULL && stack_capture_from_uc(uc, &stack_start, &stack_length);

  const size_t ehdr_size = sizeof(Elf64_Ehdr);
  const size_t phdr_size = sizeof(Elf64_Phdr);
  const int phdr_count = has_stack ? 2 : 1;
  const size_t notes_offset = ehdr_size + (size_t)phdr_count * phdr_size;

  size_t cursor = notes_offset;
  if (!buffer_append_note(&cursor, "CORE", HD_SENTRY_NT_PRSTATUS, &g_prstatus,
                          sizeof(g_prstatus)) ||
      !buffer_append_note(&cursor, "CORE", HD_SENTRY_NT_SIGINFO, &elf_siginfo,
                          sizeof(elf_siginfo)) ||
      !buffer_append_note(&cursor, "CORE", HD_SENTRY_NT_PRPSINFO, &g_prpsinfo,
                          sizeof(g_prpsinfo))) {
    return FALSE;
  }

  if (nt_file_size > 0 &&
      !buffer_append_note(&cursor, "CORE", HD_SENTRY_NT_FILE, g_nt_file_buffer,
                          nt_file_size)) {
    return FALSE;
  }

  const size_t notes_end = cursor;
  size_t load_offset = notes_end;
  if (has_stack) {
    if (load_offset + stack_length > HD_SENTRY_CORE_BUF_SIZE) {
      return FALSE;
    }
    memcpy(g_core_buffer + load_offset, (const void*)stack_start, stack_length);
    cursor = load_offset + stack_length;
  }

  const size_t file_size = cursor;
  const size_t notes_size = notes_end - notes_offset;

  Elf64_Ehdr ehdr;
  memset(&ehdr, 0, sizeof(ehdr));
  memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
  ehdr.e_ident[EI_CLASS] = ELFCLASS64;
  ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr.e_ident[EI_VERSION] = EV_CURRENT;
  ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
  ehdr.e_type = ET_CORE;
#if defined(__x86_64__)
  ehdr.e_machine = EM_X86_64;
#elif defined(__aarch64__)
  ehdr.e_machine = EM_AARCH64;
#else
  ehdr.e_machine = EM_NONE;
#endif
  ehdr.e_version = EV_CURRENT;
  ehdr.e_phoff = ehdr_size;
  ehdr.e_ehsize = (Elf64_Half)ehdr_size;
  ehdr.e_phentsize = (Elf64_Half)phdr_size;
  ehdr.e_phnum = (Elf64_Half)phdr_count;

  Elf64_Phdr note_phdr;
  memset(&note_phdr, 0, sizeof(note_phdr));
  note_phdr.p_type = PT_NOTE;
  note_phdr.p_offset = notes_offset;
  note_phdr.p_filesz = notes_size;
  note_phdr.p_align = 4;

  Elf64_Phdr load_phdr;
  if (has_stack) {
    memset(&load_phdr, 0, sizeof(load_phdr));
    load_phdr.p_type = PT_LOAD;
    load_phdr.p_flags = PF_R | PF_W;
    load_phdr.p_offset = load_offset;
    load_phdr.p_vaddr = stack_start;
    load_phdr.p_paddr = stack_start;
    load_phdr.p_filesz = stack_length;
    load_phdr.p_memsz = stack_length;
    load_phdr.p_align = 4096;
  }

  memcpy(g_core_buffer, &ehdr, sizeof(ehdr));
  memcpy(g_core_buffer + ehdr_size, &note_phdr, sizeof(note_phdr));
  if (has_stack) {
    memcpy(g_core_buffer + ehdr_size + phdr_size, &load_phdr,
           sizeof(load_phdr));
  }

  return write_file_bytes(path, g_core_buffer, file_size);
}

#else

static gboolean write_elf_core(const char* path, gint signo,
                               const siginfo_t* info, const ucontext_t* uc) {
  (void)path;
  (void)signo;
  (void)info;
  (void)uc;
  return FALSE;
}

#endif

gboolean hd_sentry_linux_crash_dump_write(const gchar* crash_dir,
                                          const gchar* stem,
                                          gint signo,
                                          const siginfo_t* info,
                                          const void* ucontext) {
  if (crash_dir == NULL || stem == NULL) {
    return FALSE;
  }

  g_autofree gchar* maps_path =
      g_strdup_printf("%s/%s%s", crash_dir, stem, HD_SENTRY_MAPS_SUFFIX);
  const gboolean maps_ok = copy_proc_file("/proc/self/maps", maps_path);

  g_autofree gchar* dump_path =
      g_strdup_printf("%s/%s%s", crash_dir, stem, HD_SENTRY_DUMP_SUFFIX);
  const ucontext_t* uc = (const ucontext_t*)ucontext;
  const gboolean core_ok = write_elf_core(dump_path, signo, info, uc);

  if (core_ok) {
    g_autofree gchar* dump_name =
        g_strdup_printf("%s%s", stem, HD_SENTRY_DUMP_SUFFIX);
    (void)write_gdb_script(crash_dir, stem, dump_name);
  }

  return maps_ok || core_ok;
}
