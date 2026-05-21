import Darwin
import Foundation

enum HdSentryStackTrace {
  static func signalName(_ sig: Int32) -> String {
    switch sig {
    case SIGABRT: return "SIGABRT"
    case SIGBUS: return "SIGBUS"
    case SIGFPE: return "SIGFPE"
    case SIGILL: return "SIGILL"
    case SIGSEGV: return "SIGSEGV"
    case SIGTRAP: return "SIGTRAP"
    default: return "SIGNAL_\(sig)"
    }
  }

  static func signalHint(_ sig: Int32) -> String {
    switch sig {
    case SIGABRT:
      return "abort() / assertion / uncaught ObjC exception (often after Flutter method channel)"
    case SIGSEGV:
      return "invalid memory access (null pointer, use-after-free)"
    case SIGBUS:
      return "misaligned memory access"
    case SIGFPE:
      return "arithmetic exception (e.g. division by zero)"
    case SIGILL:
      return "illegal instruction"
    case SIGTRAP:
      return "trace/breakpoint trap"
    default:
      return "see man signal(3)"
    }
  }

  /// Builds report text: origin summary + cleaned trace + raw trace.
  static func formatForCrashReport() -> String {
    let raw = Thread.callStackSymbols
    let cleaned = raw.filter { !isHdSentryHandlerFrame($0) }
    let originLines = extractOriginLines(from: cleaned)

    var sections: [String] = []

    if let summary = originLines.first.flatMap(originSummary(from:)) {
      sections.append("--- likely origin ---\n\(summary)")
    }

    if !originLines.isEmpty {
      sections.append(
        "--- application frames (handler removed) ---\n"
          + originLines.joined(separator: "\n"))
    }

    sections.append("--- full stack trace ---\n" + raw.joined(separator: "\n"))

    if let first = originLines.first, let cmd = atosHint(from: first) {
      sections.append("--- symbolicate (Terminal) ---\n\(cmd)")
    }

    return sections.joined(separator: "\n\n")
  }

  private static func isHdSentryHandlerFrame(_ line: String) -> Bool {
    line.contains("HdSentryCrashHandler")
      || line.contains("_sigtramp")
  }

  /// Drops abort delivery frames, then returns the first useful frames.
  private static func extractOriginLines(from cleaned: [String]) -> [String] {
    let abortDelivery = ["pthread_kill", " abort", "libsystem_c.dylib", "libsystem_platform.dylib"]
    var lines: [String] = []
    var passedAbortChain = false

    for line in cleaned {
      if !passedAbortChain {
        if abortDelivery.contains(where: { line.contains($0) }) {
          continue
        }
        passedAbortChain = true
      }
      if line.trimmingCharacters(in: .whitespaces).isEmpty {
        continue
      }
      lines.append(line)
    }

    return Array(lines.prefix(8))
  }

  /// Parses a `Thread.callStackSymbols` line: `index image address symbol`.
  private static func originSummary(from line: String) -> String? {
    let trimmed = line.trimmingCharacters(in: .whitespaces)
    let parts = trimmed.split(separator: " ", omittingEmptySubsequences: true)
    guard parts.count >= 3 else { return nil }

    let image = String(parts[1])
    let address = String(parts[2])
    let symbol =
      parts.count > 3 ? parts[3...].joined(separator: " ") : "(no symbol)"

    var summary = "image: \(image)\naddress: \(address)\nsymbol: \(symbol)"

    if symbol.hasPrefix("$s") || symbol.contains("_$") {
      summary += "\n\nDemangle Swift symbol:\n  swift demangle '\(symbol)'"
    }

    return summary
  }

  private static func atosHint(from line: String) -> String? {
    let parts = line.split(separator: " ", omittingEmptySubsequences: true)
    guard parts.count >= 3 else { return nil }
    let image = String(parts[1])
    let address = String(parts[2])
    return """
      # Replace path with the .dylib/.app on disk (Debug build), then run:
      atos -o "<path/to/\(image)>" -arch $(uname -m) \(address)
      # Example (macOS example app):
      # atos -o "build/macos/Build/Products/Debug/hd_sentry_example.app/Contents/MacOS/hd_sentry_example.debug.dylib" -arch arm64 \(address)
      """
  }
}
