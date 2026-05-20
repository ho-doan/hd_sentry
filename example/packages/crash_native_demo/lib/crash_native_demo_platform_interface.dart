import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'crash_native_demo_method_channel.dart';

abstract class CrashNativeDemoPlatform extends PlatformInterface {
  CrashNativeDemoPlatform() : super(token: _token);

  static final Object _token = Object();

  static CrashNativeDemoPlatform _instance = MethodChannelCrashNativeDemo();

  static CrashNativeDemoPlatform get instance => _instance;

  static set instance(CrashNativeDemoPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('getPlatformVersion() has not been implemented.');
  }

  /// Triggers a fatal native crash that terminates the host process (best-effort on web).
  Future<void> crashNative() {
    throw UnimplementedError('crashNative() has not been implemented.');
  }
}
