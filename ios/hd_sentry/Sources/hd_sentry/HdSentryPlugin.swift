import Flutter
import UIKit

public class HdSentryPlugin: NSObject, FlutterPlugin, HdSentryHostApi {
  public static func register(with registrar: FlutterPluginRegistrar) {
    let instance = HdSentryPlugin()
    HdSentryHostApiSetup.setUp(binaryMessenger: registrar.messenger(), api: instance)
  }

  public func initialize() throws {
    HdSentryCrashHandler.install()
  }

  public func getCrashDirectory() throws -> String {
    return try HdSentryCrashStore.directory().path
  }

  public func listCrashFileNames() throws -> [String] {
    return try HdSentryCrashStore.listFileNames()
  }

  public func readCrashFile(fileName: String) throws -> String {
    return try HdSentryCrashStore.readFile(fileName: fileName)
  }

  public func deleteCrashFile(fileName: String) throws -> Bool {
    return try HdSentryCrashStore.deleteFile(fileName: fileName)
  }

  public func clearAllCrashFiles() throws {
    try HdSentryCrashStore.clearAll()
  }

  public func captureException(message: String, stackTrace: String?) throws {
    try HdSentryCrashStore.writeReport(
      platform: "ios",
      type: "flutter_error",
      message: message,
      stackTrace: stackTrace ?? ""
    )
  }
}
