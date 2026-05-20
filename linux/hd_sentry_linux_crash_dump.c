#define _GNU_SOURCE

#include "hd_sentry_linux_crash_dump.h"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/elfcore.h>
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
#define HD_SENTRY_STACK_DUMP_BYTES (64 * 1024)

static uint8_t g_core_buffer[HD_SENTRY_CORE_BUF_SIZE];

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

typedef struct {
  uint8_t* base;
  size_t capacity;
  size_t offset;
} HdSentryCoreWriter;

static void writer_init(HdSentryCoreWriter* w) {
  w->base = g_core_buffer;
  w->capacity = HD_SENTRY_CORE_BUF_SIZE;
  w->offset = 0;
}

static gboolean writer_append(HdSentryCoreWriter* w, const void* data,
                              size_t size) {
  if (w->offset + size > w->capacity) {
    return FALSE;
  }
  memcpy(w->base + w->offset, data, size);
  w->offset += size;
  return TRUE;
}

static size_t writer_align4(HdSentryCoreWriter* w) {
  const size_t padding = (4 - (w->offset % 4)) % 4;
  if (padding == 0) {
    return w->offset;
  }
  static const uint8_t zero_pad[3] = {0, 0, 0};
  if (!writer_append(w, zero_pad, padding)) {
    return 0;
  }
  return w->offset;
}

static gboolean writer_note(HdSentryCoreWriter* w, const char* name, int type,
                            const void* desc, size_t desc_size) {
  Elf64_Nhdr nhdr;
  nhdr.n_namesz = (Elf64_Word)(strlen(name) + 1);
  nhdr.n_descsz = (Elf64_Word)desc_size;
  nhdr.n_type = (Elf64_Word)type;
  if (!writer_append(w, &nhdr, sizeof(nhdr)) ||
      !writer_append(w, name, nhdr.n_namesz)) {
    return FALSE;
  }
  writer_align4(w);
  if (!writer_append(w, desc, desc_size)) {
    return FALSE;
  }
  writer_align4(w);
  return TRUE;
}

#if defined(__x86_64__)

static gboolean fill_prstatus(const ucontext_t* uc, struct elf_prstatus* pr) {
  memset(pr, 0, sizeof(*pr));
  pr->pr_info.si_signo = 0;
  pr->pr_cursig = 0;
  pr->pr_sigpend = 0;
  pr->pr_sighold = 0;
  pr->pr_pid = getpid();
  pr->pr_ppid = getppid();
  pr->pr_pgrp = getpgrp();
  pr->pr_sid = getsid(0);
  memcpy(&pr->pr_reg, &uc->uc_mcontext.gregs, sizeof(pr->pr_reg));
  return TRUE;
}

static gboolean stack_region_from_uc(const ucontext_t* uc, uintptr_t* start,
                                     size_t* length) {
  const uintptr_t sp = (uintptr_t)uc->uc_mcontext.gregs[REG_RSP];
  if (sp == 0) {
    return FALSE;
  }
  const uintptr_t end = sp + HD_SENTRY_STACK_DUMP_BYTES;
  *start = sp & ~(uintptr_t)0xFFF;
  *length = end - *start;
  if (*length > HD_SENTRY_STACK_DUMP_BYTES + 4096) {
    *length = HD_SENTRY_STACK_DUMP_BYTES + 4096;
  }
  return TRUE;
}

#elif defined(__aarch64__)

static gboolean fill_prstatus(const ucontext_t* uc, struct elf_prstatus* pr) {
  memset(pr, 0, sizeof(*pr));
  pr->pr_pid = getpid();
  pr->pr_ppid = getppid();
  pr->pr_pgrp = getpgrp();
  pr->pr_sid = getsid(0);
  memcpy(&pr->pr_reg, &uc->uc_mcontext.regs, sizeof(pr->pr_reg));
  return TRUE;
}

static gboolean stack_region_from_uc(const ucontext_t* uc, uintptr_t* start,
                                     size_t* length) {
  const uintptr_t sp = (uintptr_t)uc->uc_mcontext.sp;
  if (sp == 0) {
    return FALSE;
  }
  const uintptr_t end = sp + HD_SENTRY_STACK_DUMP_BYTES;
  *start = sp & ~(uintptr_t)0xFFF;
  *length = end - *start;
  if (*length > HD_SENTRY_STACK_DUMP_BYTES + 4096) {
    *length = HD_SENTRY_STACK_DUMP_BYTES + 4096;
  }
  return TRUE;
}

#endif

static gboolean write_elf_core(const char* path, gint signo,
                               const siginfo_t* info, const ucontext_t* uc) {
  uintptr_t stack_start = 0;
  size_t stack_length = 0;
  const gboolean has_stack =
      uc != NULL && stack_region_from_uc(uc, &stack_start, &stack_length);

  struct elf_prstatus prstatus;
  if (uc == NULL || !fill_prstatus(uc, &prstatus)) {
  memset(&prstatus, 0, sizeof(prstatus));
  prstatus.pr_pid = getpid();
  }
  if (signo > 0) {
    prstatus.pr_cursig = (short)signo;
    prstatus.pr_info.si_signo = signo;
  }

  elf_siginfo_t elf_siginfo;
  memset(&elf_siginfo, 0, sizeof(elf_siginfo));
  if (info != NULL) {
    elf_siginfo.si_signo = info->si_signo;
    elf_siginfo.si_code = info->si_code;
    elf_siginfo.si_errno = info->si_errno;
  } else if (signo > 0) {
    elf_siginfo.si_signo = signo;
  }

  const int phdr_count = has_stack ? 2 : 1;
  const size_t ehdr_size = sizeof(Elf64_Ehdr);
  const size_t phdrs_size = (size_t)phdr_count * sizeof(Elf64_Phdr);
  const size_t notes_offset = ehdr_size + phdrs_size;

  HdSentryCoreWriter writer;
  writer_init(&writer);
  writer.offset = notes_offset;

  if (!writer_note(&writer, "CORE", NT_PRSTATUS, &prstatus, sizeof(prstatus)) ||
      !writer_note(&writer, "CORE", NT_SIGINFO, &elf_siginfo,
                   sizeof(elf_siginfo))) {
    return FALSE;
  }

  const size_t notes_size = writer.offset - notes_offset;
  size_t load_offset = writer.offset;
  if (has_stack) {
    if (load_offset + stack_length > writer.capacity) {
      return FALSE;
    }
    memcpy(writer.base + load_offset, (const void*)stack_start, stack_length);
    writer.offset = load_offset + stack_length;
  }

  const size_t file_size = writer.offset;

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
  ehdr.e_phentsize = sizeof(Elf64_Phdr);
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
    memcpy(g_core_buffer + ehdr_size + sizeof(note_phdr), &load_phdr,
           sizeof(load_phdr));
  }

  const int out_fd =
      open(path, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)0644);
  if (out_fd < 0) {
    return FALSE;
  }

  ssize_t written_total = 0;
  while ((size_t)written_total < file_size) {
    const ssize_t n = write(out_fd, g_core_buffer + written_total,
                            file_size - (size_t)written_total);
    if (n < 0) {
      close(out_fd);
      unlink(path);
      return FALSE;
    }
    written_total += n;
  }
  close(out_fd);
  return TRUE;
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
  if (!core_ok) {
    unlink(dump_path);
  }

  return maps_ok || core_ok;
}
