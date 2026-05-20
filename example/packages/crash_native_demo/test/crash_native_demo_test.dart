import 'package:flutter_test/flutter_test.dart';
import 'package:crash_native_demo/crash_native_demo.dart';
import 'package:crash_native_demo/crash_native_demo_platform_interface.dart';
import 'package:crash_native_demo/crash_native_demo_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockCrashNativeDemoPlatform
    with MockPlatformInterfaceMixin
    implements CrashNativeDemoPlatform {
  @override
  Future<String?> getPlatformVersion() => Future.value('42');

  @override
  Future<void> crashNative() {
    // TODO: implement crashNative
    throw UnimplementedError();
  }
}

void main() {
  final CrashNativeDemoPlatform initialPlatform =
      CrashNativeDemoPlatform.instance;

  test('$MethodChannelCrashNativeDemo is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelCrashNativeDemo>());
  });

  test('getPlatformVersion', () async {
    CrashNativeDemo crashNativeDemoPlugin = CrashNativeDemo();
    MockCrashNativeDemoPlatform fakePlatform = MockCrashNativeDemoPlatform();
    CrashNativeDemoPlatform.instance = fakePlatform;

    expect(await crashNativeDemoPlugin.getPlatformVersion(), '42');
  });
}
