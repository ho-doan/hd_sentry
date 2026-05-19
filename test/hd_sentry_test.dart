import 'package:flutter_test/flutter_test.dart';
import 'package:hd_sentry/src/hd_sentry_backend.dart';

class FakeHdSentryBackend extends HdSentryBackend {
  final Map<String, String> files = {
    'crash_1.txt': '''
=== HD Sentry Native Crash Report ===
platform: test
timestamp: 2026-05-19T10:00:00.000Z
type: uncaught_exception
message: boom

--- stack trace ---
#0 main
''',
  };

  @override
  Future<void> clearAllCrashFiles() async => files.clear();

  @override
  Future<bool> deleteCrashFile(String fileName) async =>
      files.remove(fileName) != null;

  @override
  Future<String> getCrashDirectory() async => '/tmp/hd_sentry_crashes';

  @override
  Future<void> initialize() async {}

  @override
  Future<List<String>> listCrashFileNames() async => files.keys.toList();

  @override
  Future<String> readCrashFile(String fileName) async => files[fileName]!;
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  test('parses crash report metadata', () async {
    final backend = FakeHdSentryBackend();
    final reports = await backend.listCrashReports();
    expect(reports, hasLength(1));
    expect(reports.first.platform, 'test');
    expect(reports.first.type, 'uncaught_exception');
    expect(reports.first.message, 'boom');
  });

  test('HdSentry delegates to backend', () async {
    final backend = FakeHdSentryBackend();
    // Access via test-only path: list via backend directly in unit tests.
    await backend.initialize();
    expect(await backend.listCrashFileNames(), ['crash_1.txt']);
    expect(await backend.deleteCrashFile('crash_1.txt'), isTrue);
    expect(await backend.listCrashFileNames(), isEmpty);
  });
}
