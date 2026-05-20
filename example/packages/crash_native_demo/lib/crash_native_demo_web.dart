import 'dart:js_interop';

import 'package:flutter_web_plugins/flutter_web_plugins.dart';
import 'package:web/web.dart' as web;

import 'crash_native_demo_platform_interface.dart';

/// Web cannot SIGSEGV; schedules an uncaught JS error on the main thread.
class CrashNativeDemoWeb extends CrashNativeDemoPlatform {
  static void registerWith(Registrar registrar) {
    CrashNativeDemoPlatform.instance = CrashNativeDemoWeb();
  }

  @override
  Future<String?> getPlatformVersion() async {
    return web.window.navigator.userAgent;
  }

  @override
  Future<void> crashNative() async {
    _eval(
      'setTimeout(function(){ throw new Error("demo native crash (web)"); }, 0)'
          .toJS,
    );
  }
}

@JS('eval')
external void _eval(JSString source);
