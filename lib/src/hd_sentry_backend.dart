import 'models/native_crash_report.dart';

/// Platform backend for crash capture and file access.
abstract class HdSentryBackend {
  Future<void> initialize();

  Future<String> getCrashDirectory();

  Future<List<String>> listCrashFileNames();

  Future<String> readCrashFile(String fileName);

  Future<bool> deleteCrashFile(String fileName);

  Future<void> clearAllCrashFiles();

  Future<List<NativeCrashReport>> listCrashReports() async {
    final names = await listCrashFileNames();
    final reports = <NativeCrashReport>[];
    for (final name in names) {
      final content = await readCrashFile(name);
      reports.add(
        NativeCrashReport.fromContent(fileName: name, content: content),
      );
    }
    reports.sort((a, b) {
      final at = a.timestamp ?? DateTime.fromMillisecondsSinceEpoch(0);
      final bt = b.timestamp ?? DateTime.fromMillisecondsSinceEpoch(0);
      return bt.compareTo(at);
    });
    return reports;
  }
}
