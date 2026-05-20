import 'crash_native_demo_platform_interface.dart';

class CrashNativeDemo {
  Future<String?> getPlatformVersion() {
    return CrashNativeDemoPlatform.instance.getPlatformVersion();
  }

  /// Kills the host app via a platform-native fatal crash (see [crash_native_demo] README).
  Future<void> crashNative() {
    return CrashNativeDemoPlatform.instance.crashNative();
  }
}
