import Foundation

enum HdSentryCrashHandler {
  private static var installed = false
  private static var previousExceptionHandler: (@convention(c) (NSException) -> Void)?

  private static let platform = "macos"

  static func install() {
    if installed { return }
    installed = true
    HdSentryCrashStore.configure()

    previousExceptionHandler = NSGetUncaughtExceptionHandler()
    NSSetUncaughtExceptionHandler(exceptionHandler)

    installSignalHandlers()
  }

  private static let exceptionHandler: @convention(c) (NSException) -> Void = { exception in
    let stack = exception.callStackSymbols.joined(separator: "\n")
    try? HdSentryCrashStore.writeReport(
      platform: platform,
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
    for signal in signals {
      Darwin.signal(signal, signalHandler)
    }
  }

  private static let signalHandler: @convention(c) (Int32) -> Void = { signal in
    let stack = Thread.callStackSymbols.joined(separator: "\n")
    try? HdSentryCrashStore.writeReport(
      platform: platform,
      type: "signal",
      message: "Signal \(signal) received",
      stackTrace: stack
    )
    Darwin.raise(signal)
  }
}
