import Flutter
import UIKit

public class CrashNativeDemoPlugin: NSObject, FlutterPlugin {
  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "crash_native_demo", binaryMessenger: registrar.messenger())
    let instance = CrashNativeDemoPlugin()
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "getPlatformVersion":
      result("iOS " + UIDevice.current.systemVersion)
    case "crashNative":
      result(nil)
      DispatchQueue.main.async {
        CrashNativeDemoPlugin.triggerFatalCrash()
      }
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  private static func triggerFatalCrash() {
    // SIGABRT — caught by hd_sentry signal handler when installed.
    abort()
  }
}
