import Foundation

enum HdSentryCrashHandler {
  private static var installed = false
  private static var previousExceptionHandler: (@convention(c) (NSException) -> Void)?

  private static var crashReportWritten: Int32 = 0

  private static let platform = "macos"

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
    Darwin.signal(sig, SIG_DFL)

    writeReportOnce(
      type: "signal",
      message: "Signal \(sig) received",
      stackTrace: Thread.callStackSymbols.joined(separator: "\n")
    )

    Darwin.raise(sig)
  }
}
