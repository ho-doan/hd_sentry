#define _GNU_SOURCE

#include "hd_sentry_linux_crash_dump.h"
#include "hd_sentry_linux_elfcore_types.h"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define HD_SENTRY_DUMP_SUFFIX ".dmp"
#define HD_SENTRY_MAPS_SUFFIX ".maps"
#define HD_SENTRY_CORE_BUF_SIZE (512 * 1024)
static uint8_t g_core_buffer[HD_SENTRY_CORE_BUF_SIZE];

#if defined(__x86_64__) || defined(__aarch64__)
static HdSentryElfPrstatus g_prstatus;
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

  HdSentryElfSiginfo elf_siginfo;
  memset(&elf_siginfo, 0, sizeof(elf_siginfo));
  if (info != NULL) {
    elf_siginfo.si_signo = info->si_signo;
    elf_siginfo.si_code = info->si_code;
    elf_siginfo.si_errno = info->si_errno;
  } else if (signo > 0) {
    elf_siginfo.si_signo = signo;
  }

  const size_t ehdr_size = sizeof(Elf64_Ehdr);
  const size_t phdr_size = sizeof(Elf64_Phdr);
  const size_t notes_offset = ehdr_size + phdr_size;

  size_t cursor = notes_offset;
  if (!buffer_append_note(&cursor, "CORE", HD_SENTRY_NT_PRSTATUS, &g_prstatus,
                          sizeof(g_prstatus)) ||
      !buffer_append_note(&cursor, "CORE", HD_SENTRY_NT_SIGINFO, &elf_siginfo,
                          sizeof(elf_siginfo))) {
    return FALSE;
  }

  const size_t file_size = cursor;
  const size_t notes_size = file_size - notes_offset;

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
  ehdr.e_phnum = 1;

  Elf64_Phdr note_phdr;
  memset(&note_phdr, 0, sizeof(note_phdr));
  note_phdr.p_type = PT_NOTE;
  note_phdr.p_offset = notes_offset;
  note_phdr.p_filesz = notes_size;
  note_phdr.p_align = 4;

  memcpy(g_core_buffer, &ehdr, sizeof(ehdr));
  memcpy(g_core_buffer + ehdr_size, &note_phdr, sizeof(note_phdr));

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

  return maps_ok || core_ok;
}
