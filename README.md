# hd_sentry

Flutter plugin bắt **native crash** trên Android, iOS, macOS, Windows, Linux và lỗi JS trên **Web**, lưu báo cáo ra file (hoặc `localStorage` trên web) để đọc từ Dart.

## Cài đặt

```yaml
dependencies:
  hd_sentry:
    path: ../hd_sentry
```

## Khởi tạo

Gọi càng sớm càng tốt (trước `runApp`):

```dart
import 'package:flutter/material.dart';
import 'package:hd_sentry/hd_sentry.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await HdSentry.initialize();
  runApp(const MyApp());
}
```

## Đọc crash từ Dart

```dart
final directory = await HdSentry.getCrashDirectory();
final fileNames = await HdSentry.listCrashFileNames();
final reports = await HdSentry.listCrashReports();

for (final report in reports) {
  print(report.fileName);
  print(report.platform);
  print(report.message);
  print(report.content);
}

await HdSentry.deleteCrashFile(fileNames.first);
await HdSentry.clearAllCrashFiles();
```

## Nền tảng được hỗ trợ

| Nền tảng | Cơ chế bắt crash | Lưu trữ |
|----------|------------------|---------|
| Android | `UncaughtExceptionHandler` | `filesDir/hd_sentry_crashes/` |
| iOS / macOS | `NSUncaughtExceptionHandler` + signal handlers | Application Support |
| Windows | `SetUnhandledExceptionFilter` | `%LOCALAPPDATA%/hd_sentry_crashes/` |
| Linux | Signal handlers | `~/.local/share/hd_sentry_crashes/` |
| Web | `error`, `unhandledrejection` | `localStorage` |

## Định dạng file

```
=== HD Sentry Native Crash Report ===
platform: android
timestamp: 2026-05-19T12:00:00.000Z
type: uncaught_exception
message: ...

--- stack trace ---
...
```

## Lưu ý

- Plugin bắt crash **native / JS**, không thay thế bắt lỗi Dart (`FlutterError`, `runZonedGuarded`, Sentry Flutter SDK, …).
- Sau khi ghi file, crash vẫn được chuyển tiếp cho handler mặc định của hệ điều hành.
- Trên iOS/macOS, signal handler chỉ ghi báo cáo tối thiểu rồi re-raise signal.

## Regenerate Pigeon

```bash
dart run pigeon --input pigeons/messages.dart
```
