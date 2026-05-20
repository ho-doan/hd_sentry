import 'dart:convert';
import 'dart:js_interop';

import 'package:web/web.dart' as web;

import 'hd_sentry_backend.dart';

const _storageKey = 'hd_sentry_crashes';

HdSentryBackend createBackend() => HdSentryWebBackend();

/// Web crash backend using `localStorage` and JS error hooks.
class HdSentryWebBackend extends HdSentryBackend {
  static bool _handlersInstalled = false;

  static Map<String, String> _loadStore() {
    final raw = web.window.localStorage.getItem(_storageKey);
    if (raw == null || raw.isEmpty) {
      return {};
    }
    try {
      final decoded = jsonDecode(raw) as Map<String, dynamic>;
      return decoded.map((k, v) => MapEntry(k, v as String));
    } catch (_) {
      return {};
    }
  }

  static void _saveStore(Map<String, String> store) {
    web.window.localStorage.setItem(_storageKey, jsonEncode(store));
  }

  static String _formatReport({
    required String type,
    required String message,
    required String stack,
  }) {
    final now = DateTime.now().toUtc().toIso8601String();
    return '''
=== HD Sentry Native Crash Report ===
platform: web
timestamp: $now
type: $type
message: $message

--- stack trace ---
$stack
''';
  }

  void _persistCrash({
    required String type,
    required String message,
    required String stack,
  }) {
    final store = _loadStore();
    final fileName = 'crash_${DateTime.now().millisecondsSinceEpoch}.txt';
    store[fileName] = _formatReport(type: type, message: message, stack: stack);
    _saveStore(store);
  }

  void _installHandlers() {
    if (_handlersInstalled) return;
    _handlersInstalled = true;

    void onError(web.Event event) {
      final errorEvent = event as web.ErrorEvent;
      _persistCrash(
        type: 'window_error',
        message: errorEvent.message,
        stack: [
          'filename: ${errorEvent.filename}',
          'lineno: ${errorEvent.lineno}',
          'colno: ${errorEvent.colno}',
        ].join('\n'),
      );
    }

    void onUnhandledRejection(web.Event event) {
      final rejection = event as web.PromiseRejectionEvent;
      final reason = '${rejection.reason}';
      _persistCrash(
        type: 'unhandled_rejection',
        message: reason,
        stack: reason,
      );
    }

    web.window.addEventListener('error', onError.toJS);
    web.window.addEventListener(
      'unhandledrejection',
      onUnhandledRejection.toJS,
    );
  }

  @override
  Future<void> initialize() async {
    _installHandlers();
  }

  @override
  Future<String> getCrashDirectory() async => 'web://localStorage/$_storageKey';

  @override
  Future<List<String>> listCrashFileNames() async {
    final names = _loadStore().keys.toList();
    names.sort((a, b) => b.compareTo(a));
    return names;
  }

  @override
  Future<String> readCrashFile(String fileName) async {
    final content = _loadStore()[fileName];
    if (content == null) {
      throw StateError('Crash file not found: $fileName');
    }
    return content;
  }

  @override
  Future<bool> deleteCrashFile(String fileName) async {
    final store = _loadStore();
    final removed = store.remove(fileName) != null;
    if (removed) {
      _saveStore(store);
    }
    return removed;
  }

  @override
  Future<void> clearAllCrashFiles() async {
    web.window.localStorage.removeItem(_storageKey);
  }

  @override
  Future<void> captureException(String message, String? stackTrace) async {
    _persistCrash(
      type: 'flutter_error',
      message: message,
      stack: stackTrace ?? '',
    );
  }
}
