import 'pigeon/messages.g.dart';

import 'hd_sentry_backend.dart';

HdSentryBackend createBackend() => HdSentryNativeBackend();

/// Pigeon host implementation for mobile and desktop platforms.
class HdSentryNativeBackend extends HdSentryBackend {
  HdSentryNativeBackend({HdSentryHostApi? host})
    : _host = host ?? HdSentryHostApi();

  final HdSentryHostApi _host;

  @override
  Future<void> initialize() => _host.initialize();

  @override
  Future<String> getCrashDirectory() => _host.getCrashDirectory();

  @override
  Future<List<String>> listCrashFileNames() => _host.listCrashFileNames();

  @override
  Future<String> readCrashFile(String fileName) =>
      _host.readCrashFile(fileName);

  @override
  Future<bool> deleteCrashFile(String fileName) =>
      _host.deleteCrashFile(fileName);

  @override
  Future<void> clearAllCrashFiles() => _host.clearAllCrashFiles();

  @override
  Future<void> captureException(String message, String? stackTrace) =>
      _host.captureException(message, stackTrace);

  @override
  Future<void> addBreadcrumb(
    String message, {
    String? category,
    String? data,
  }) =>
      _host.addBreadcrumb(message, category, data);

  @override
  Future<void> clearBreadcrumbs() => _host.clearBreadcrumbs();
}
