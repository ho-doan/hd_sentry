import 'dart:convert';
import 'dart:js_interop';
import 'dart:js_interop_unsafe';

import 'package:web/web.dart' as web;

import 'hd_sentry_backend.dart';

const _storageKey = 'hd_sentry_crashes';
const _breadcrumbsKey = 'hd_sentry_session_breadcrumbs';
const _maxBreadcrumbs = 100;

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
    String breadcrumbsSection = '',
  }) {
    final now = DateTime.now().toUtc().toIso8601String();
    return '''
=== HD Sentry Native Crash Report ===
platform: web
timestamp: $now
type: $type
message: $message

--- stack trace ---
$stack$breadcrumbsSection''';
  }

  static String _sanitizeField(String value) =>
      value.replaceAll('\t', ' ').replaceAll('\n', ' ').replaceAll('\r', ' ');

  static List<String> _loadBreadcrumbLines() {
    final raw = web.window.localStorage.getItem(_breadcrumbsKey);
    if (raw == null || raw.isEmpty) {
      return [];
    }
    return raw
        .split('\n')
        .map((line) => line.trim())
        .where((line) => line.isNotEmpty)
        .toList();
  }

  static void _saveBreadcrumbLines(List<String> lines) {
    if (lines.isEmpty) {
      web.window.localStorage.removeItem(_breadcrumbsKey);
      return;
    }
    web.window.localStorage.setItem(_breadcrumbsKey, '${lines.join('\n')}\n');
  }

  static String _consumeBreadcrumbsSection() {
    final lines = _loadBreadcrumbLines();
    web.window.localStorage.removeItem(_breadcrumbsKey);
    if (lines.isEmpty) {
      return '';
    }
    return '\n--- breadcrumbs ---\n${lines.join('\n')}\n';
  }

  void _persistCrash({
    required String type,
    required String message,
    required String stack,
  }) {
    final store = _loadStore();
    final fileName = 'crash_${DateTime.now().millisecondsSinceEpoch}.txt';
    store[fileName] = _formatReport(
      type: type,
      message: message,
      stack: stack,
      breadcrumbsSection: _consumeBreadcrumbsSection(),
    );
    _saveStore(store);
  }

  void _installHandlers() {
    if (_handlersInstalled) return;
    _handlersInstalled = true;
    void onError(web.Event event) {
      final errorEvent = event as web.ErrorEvent;
      final detail = _formatErrorEventForReport(errorEvent);
      final message = errorEvent.message.isNotEmpty
          ? errorEvent.message
          : _shortLabelFromThrown(errorEvent.error);
      _persistCrash(
        type: 'window_error',
        message: message,
        stack: detail,
      );
    }

    void onUnhandledRejection(web.Event event) {
      final rejection = event as web.PromiseRejectionEvent;
      final reason = rejection.reason;
      final detail = _describeJsValue(reason);
      final short = _shortLabelFromThrown(reason);
      _persistCrash(
        type: 'unhandled_rejection',
        message: short,
        stack: detail,
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
    web.window.localStorage.removeItem(_breadcrumbsKey);
  }

  @override
  Future<void> captureException(String message, String? stackTrace) async {
    _persistCrash(
      type: 'flutter_error',
      message: message,
      stack: stackTrace ?? '',
    );
  }

  @override
  Future<void> addBreadcrumb(
    String message, {
    String? category,
    String? data,
  }) async {
    final timestamp = DateTime.now().toUtc().toIso8601String();
    final line = [
      timestamp,
      _sanitizeField(category ?? ''),
      _sanitizeField(message),
      _sanitizeField(data ?? ''),
    ].join('\t');
    final lines = _loadBreadcrumbLines()..add(line);
    if (lines.length > _maxBreadcrumbs) {
      lines.removeRange(0, lines.length - _maxBreadcrumbs);
    }
    _saveBreadcrumbLines(lines);
  }

  @override
  Future<void> clearBreadcrumbs() async {
    web.window.localStorage.removeItem(_breadcrumbsKey);
  }
}

String _shortLabelFromThrown(JSAny? value) {
  if (value == null) {
    return '(no error object)';
  }
  if (value.isA<JSString>()) {
    return (value as JSString).toDart;
  }
  if (value.isA<JSObject>()) {
    final o = value as JSObject;
    final msg = _readPropertyAsDartString(o, 'message');
    if (msg.isNotEmpty) return msg;
    final name = _readPropertyAsDartString(o, 'name');
    if (name.isNotEmpty) return name;
  }
  try {
    final d = value.dartify();
    if (d != null) return d.toString();
  } catch (_) {}
  return value.toString();
}

/// Full context for [web.ErrorEvent]: location + thrown value (stack for JS [Error], incl. eval).
String _formatErrorEventForReport(web.ErrorEvent e) {
  final lines = <String>[
    'ErrorEvent.message: ${e.message}',
    'filename: ${e.filename}',
    'lineno: ${e.lineno}',
    'colno: ${e.colno}',
    'event.type: ${e.type}',
  ];
  final err = e.error;
  if (err != null) {
    lines.add('');
    lines.add('--- ErrorEvent.error (thrown value) ---');
    lines.add(_describeJsValue(err));
  }
  return lines.join('\n');
}

String _describeJsValue(JSAny? value) {
  if (value == null) {
    return '(null)';
  }
  if (value.isA<JSString>()) {
    return (value as JSString).toDart;
  }
  if (value.isA<JSNumber>()) {
    return (value as JSNumber).toDartDouble.toString();
  }
  if (value.isA<JSBoolean>()) {
    return (value as JSBoolean).toDart.toString();
  }
  final obj = value as JSObject;
  final stack = _readPropertyAsDartString(obj, 'stack');
  if (stack.isNotEmpty) {
    final buf = StringBuffer();
    final name = _readPropertyAsDartString(obj, 'name');
    final msg = _readPropertyAsDartString(obj, 'message');
    if (name.isNotEmpty) {
      buf.writeln('name: $name');
    }
    if (msg.isNotEmpty) {
      buf.writeln('message: $msg');
    }
    buf.write('stack:\n$stack');
    return buf.toString().trimRight();
  }
  try {
    final d = value.dartify();
    return 'dartify: $d';
  } catch (_) {
    return value.toString();
  }
}

String _readPropertyAsDartString(JSObject o, String key) {
  try {
    final v = o[key];
    if (v == null) {
      return '';
    }
    if (v.isA<JSString>()) {
      return (v as JSString).toDart;
    }
    return v.toString();
  } catch (_) {
    return '';
  }
}
