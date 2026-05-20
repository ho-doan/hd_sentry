import Foundation

enum HdSentryCrashHandler {
  private static var installed = false
  private static var previousExceptionHandler: (@convention(c) (NSException) -> Void)?

  /// Ensures at most one crash report per process lifetime (signal re-entry safe).
  private static var crashReportWritten: Int32 = 0

  #if os(iOS)
  private static let platform = "ios"
  #elseif os(macOS)
  private static let platform = "macos"
  #else
  private static let platform = "unknown"
  #endif

  static func install() {
    if installed { return }
    installed = true
    HdSentryCrashStore.configure()

    previousExceptionHandler = NSGetUncaughtExceptionHandler()
    NSSetUncaughtExceptionHandler(exceptionHandler)

    installSignalHandlers()
  }

  private static func writeReportOnce(
    type: String,
    message: String,
    stackTrace: String,
    threadName: String? = nil
  ) {
    guard OSAtomicCompareAndSwap32Barrier(0, 1, &crashReportWritten) else {
      return
    }
    try? HdSentryCrashStore.writeReport(
      platform: platform,
      type: type,
      message: message,
      stackTrace: stackTrace,
      threadName: threadName
    )
  }

  private static let exceptionHandler: @convention(c) (NSException) -> Void = { exception in
    let stack = exception.callStackSymbols.joined(separator: "\n")
    writeReportOnce(
      type: "uncaught_exception",
      message: exception.reason ?? exception.name.rawValue,
      stackTrace: stack,
      threadName: Thread.current.name
    )
    if let previous = previousExceptionHandler {
      previous(exception)
    }
  }

  private static func installSignalHandlers() {
    let signals: [Int32] = [SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP]
    for sig in signals {
      Darwin.signal(sig, signalHandler)
    }
  }

  private static let signalHandler: @convention(c) (Int32) -> Void = { sig in
    // Restore default handler before re-raising — otherwise our handler is invoked
    // again in an infinite loop (one crash report file per iteration).
    Darwin.signal(sig, SIG_DFL)

    writeReportOnce(
      type: "signal",
      message: "Signal \(sig) received",
      stackTrace: Thread.callStackSymbols.joined(separator: "\n")
    )

    Darwin.raise(sig)
  }
}
