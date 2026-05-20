import 'dart:async';

import 'package:flutter/foundation.dart';

import 'hd_sentry_backend.dart';

/// Installs Flutter framework and async error handlers that persist via [backend].
class HdSentryFlutterErrorHooks {
  static FlutterExceptionHandler? _previousFlutterOnError;
  static bool Function(Object, StackTrace)? _previousPlatformOnError;
  static bool _installed = false;

  static void install(HdSentryBackend backend) {
    if (_installed) return;
    _installed = true;

    _previousFlutterOnError = FlutterError.onError;
    FlutterError.onError = (FlutterErrorDetails details) {
      unawaited(_captureFlutterError(backend, details));
      _previousFlutterOnError?.call(details);
    };

    _previousPlatformOnError = PlatformDispatcher.instance.onError;
    PlatformDispatcher.instance.onError = (Object error, StackTrace stack) {
      unawaited(
        backend.captureException(error.toString(), stack.toString()),
      );
      return _previousPlatformOnError?.call(error, stack) ?? false;
    };
  }

  static Future<void> _captureFlutterError(
    HdSentryBackend backend,
    FlutterErrorDetails details,
  ) {
    return backend.captureException(
      details.exceptionAsString(),
      details.stack?.toString(),
    );
  }
}
