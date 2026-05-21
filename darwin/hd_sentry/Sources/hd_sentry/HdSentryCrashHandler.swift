import Darwin
import Foundation

enum HdSentryCrashHandler {
  private static var installed = false
  private static var previousExceptionHandler: (@convention(c) (NSException) -> Void)?
  private static let reportLock = NSLock()
  private static var crashReportWritten = false

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
    reportLock.lock()
    defer { reportLock.unlock() }
    guard !crashReportWritten else { return }
    crashReportWritten = true

    try? HdSentryCrashStore.writeReport(
      platform: HdSentryPlatform.current,
      type: type,
      message: message,
      stackTrace: stackTrace,
      threadName: threadName
    )
  }

  private static let exceptionHandler: @convention(c) (NSException) -> Void = { exception in
    let reason = exception.reason ?? exception.name.rawValue
    let stack =
      "--- exception ---\n\(exception.name.rawValue): \(reason)\n\n"
      + HdSentryStackTrace.formatForCrashReport()
    writeReportOnce(
      type: "uncaught_exception",
      message: reason,
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

    let name = HdSentryStackTrace.signalName(sig)
    let hint = HdSentryStackTrace.signalHint(sig)
    writeReportOnce(
      type: "signal",
      message: "Signal \(sig) (\(name)): \(hint)",
      stackTrace: HdSentryStackTrace.formatForCrashReport()
    )

    Darwin.raise(sig)
  }
}
