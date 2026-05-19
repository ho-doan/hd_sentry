/// A native crash report persisted on disk (or in web storage).
class NativeCrashReport {
  const NativeCrashReport({
    required this.fileName,
    required this.content,
    required this.platform,
    this.timestamp,
    this.type,
    this.message,
  });

  /// Crash file name (not a full path).
  final String fileName;

  /// Raw report body written by the native layer.
  final String content;

  /// Platform identifier, e.g. `android`, `ios`, `web`.
  final String platform;

  /// Parsed from the report header when available.
  final DateTime? timestamp;

  /// Crash kind, e.g. `uncaught_exception`, `signal`.
  final String? type;

  /// Short error message when available.
  final String? message;

  factory NativeCrashReport.fromContent({
    required String fileName,
    required String content,
  }) {
    String? platform;
    String? type;
    String? message;
    DateTime? timestamp;

    for (final line in content.split('\n')) {
      final trimmed = line.trim();
      if (trimmed.startsWith('platform:')) {
        platform = trimmed.substring('platform:'.length).trim();
      } else if (trimmed.startsWith('type:')) {
        type = trimmed.substring('type:'.length).trim();
      } else if (trimmed.startsWith('message:')) {
        message = trimmed.substring('message:'.length).trim();
      } else if (trimmed.startsWith('timestamp:')) {
        timestamp = DateTime.tryParse(
          trimmed.substring('timestamp:'.length).trim(),
        );
      }
    }

    return NativeCrashReport(
      fileName: fileName,
      content: content,
      platform: platform ?? 'unknown',
      timestamp: timestamp,
      type: type,
      message: message,
    );
  }
}
