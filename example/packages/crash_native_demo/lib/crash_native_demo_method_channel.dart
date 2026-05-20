import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'crash_native_demo_platform_interface.dart';

class MethodChannelCrashNativeDemo extends CrashNativeDemoPlatform {
  @visibleForTesting
  final methodChannel = const MethodChannel('crash_native_demo');

  @override
  Future<String?> getPlatformVersion() async {
    return methodChannel.invokeMethod<String>('getPlatformVersion');
  }

  @override
  Future<void> crashNative() async {
    await methodChannel.invokeMethod<void>('crashNative');
  }
}
