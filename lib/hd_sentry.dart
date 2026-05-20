import 'package:flutter/foundation.dart';

import 'src/flutter_error_hooks.dart';
import 'src/hd_sentry_backend.dart';
import 'src/hd_sentry_backend_selector.dart';
import 'src/models/native_crash_report.dart';

export 'src/models/native_crash_report.dart';

/// Captures native crashes and exposes persisted crash reports to Dart.
class HdSentry {
  HdSentry._();

  static final HdSentryBackend _backend = createHdSentryBackend();
  static bool _initialized = false;

  /// Installs native crash handlers and Flutter error hooks. Call before [runApp].
  static Future<void> initialize({bool captureFlutterErrors = true}) async {
    if (_initialized) return;
    await _backend.initialize();
    if (captureFlutterErrors) {
      HdSentryFlutterErrorHooks.install(_backend);
    }
    _initialized = true;
  }

  /// Directory (or web storage location) where crash files are stored.
  static Future<String> getCrashDirectory() => _backend.getCrashDirectory();

  /// Crash file names sorted newest first.
  static Future<List<String>> listCrashFileNames() =>
      _backend.listCrashFileNames();

  /// Parsed crash reports sorted newest first.
  static Future<List<NativeCrashReport>> listCrashReports() =>
      _backend.listCrashReports();

  /// Raw text of a crash report.
  static Future<String> readCrashFile(String fileName) =>
      _backend.readCrashFile(fileName);

  /// Deletes one crash file. Returns whether it existed.
  static Future<bool> deleteCrashFile(String fileName) =>
      _backend.deleteCrashFile(fileName);

  /// Deletes all stored crash reports.
  static Future<void> clearAllCrashFiles() => _backend.clearAllCrashFiles();

  /// Persists a Flutter/Dart error to the same crash store as native reports.
  static Future<void> captureException(String message, [String? stackTrace]) =>
      _backend.captureException(message, stackTrace);

  /// Convenience wrapper for [Object] + [StackTrace].
  static Future<void> captureError(
    Object error, [
    StackTrace? stackTrace,
  ]) =>
      captureException(error.toString(), stackTrace?.toString());

  /// Persists a [FlutterErrorDetails] from [FlutterError.onError].
  static Future<void> captureFlutterErrorDetails(
    FlutterErrorDetails details,
  ) =>
      captureException(
        details.exceptionAsString(),
        details.stack?.toString(),
      );
}
