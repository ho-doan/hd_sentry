#include "hd_sentry_win_stack_trace.h"

#ifdef _WIN32

#include <dbghelp.h>

#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>

#pragma comment(lib, "dbghelp.lib")

namespace hd_sentry {
namespace {

std::once_flag g_sym_once;

void BasenameOnly(char* path) {
  if (path == nullptr || path[0] == '\0') {
    return;
  }
  char* slash = std::strrchr(path, '\\');
  char* fslash = std::strrchr(path, '/');
  char* base = path;
  if (slash != nullptr && slash + 1 > base) {
    base = slash + 1;
  }
  if (fslash != nullptr && fslash + 1 > base) {
    base = fslash + 1;
  }
  if (base != path) {
    std::memmove(path, base, std::strlen(base) + 1);
  }
}

void AppendSourceLineIfPresent(HANDLE process,
                               void* addr,
                               std::ostringstream& out) {
  IMAGEHLP_LINE64 line = {};
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  DWORD64 line_displacement = 0;
  if (!SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(addr),
                             &line_displacement, &line)) {
    return;
  }
  if (line.FileName == nullptr || line.FileName[0] == '\0') {
    return;
  }
  out << " (" << line.FileName << ":" << line.LineNumber << ")";
}

}  // namespace

void WinStackTraceEnsureInitialized() {
  std::call_once(g_sym_once, []() {
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(GetCurrentProcess(), nullptr, FALSE);
  });
}

std::string WinStackTraceFormatFrames(void* const* frames, USHORT frame_count) {
  WinStackTraceEnsureInitialized();

  HANDLE process = GetCurrentProcess();
  std::ostringstream out;

  constexpr DWORD kSymbolBufferBytes =
      static_cast<DWORD>(sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
  alignas(SYMBOL_INFO) unsigned char symbol_storage[kSymbolBufferBytes];

  for (USHORT i = 0; i < frame_count; ++i) {
    void* addr = frames[i];
    out << '#' << static_cast<int>(i) << ' ';

    std::memset(symbol_storage, 0, sizeof(symbol_storage));
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_storage);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    if (SymFromAddr(process, reinterpret_cast<DWORD64>(addr), &displacement,
                    symbol)) {
      out << symbol->Name;
      if (displacement != 0) {
        out << "+0x" << std::hex << displacement << std::dec;
      }
      AppendSourceLineIfPresent(process, addr, out);
      out << " [0x" << std::hex << std::setw(sizeof(void*) * 2)
          << reinterpret_cast<uintptr_t>(addr) << std::dec << "]\n";
      continue;
    }

    HMODULE module = nullptr;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            static_cast<LPCSTR>(addr), &module) &&
        module != nullptr) {
      char path[MAX_PATH]{};
      if (GetModuleFileNameA(module, path, MAX_PATH) == 0) {
        path[0] = '\0';
      }
      if (path[0] == '\0') {
        static const char kUnknown[] = "(unknown module)";
        size_t j = 0;
        for (; kUnknown[j] != '\0' && j < sizeof(path) - 1; ++j) {
          path[j] = kUnknown[j];
        }
        path[j] = '\0';
      } else {
        BasenameOnly(path);
      }
      const auto base = reinterpret_cast<uintptr_t>(module);
      const auto offset = reinterpret_cast<uintptr_t>(addr) - base;
      out << path << "+0x" << std::hex << offset << std::dec;
      AppendSourceLineIfPresent(process, addr, out);
      out << " [0x" << std::hex << std::setw(sizeof(void*) * 2)
          << reinterpret_cast<uintptr_t>(addr) << std::dec << "]\n";
    } else {
      out << "0x" << std::hex << std::setw(sizeof(void*) * 2)
          << reinterpret_cast<uintptr_t>(addr) << std::dec;
      AppendSourceLineIfPresent(process, addr, out);
      out << '\n';
    }
  }

  return out.str();
}

}  // namespace hd_sentry

#endif  // _WIN32
