# hd_sentry

Flutter plugin bắt **native crash** trên Android, iOS, macOS, Windows, Linux và lỗi JS trên **Web**, lưu báo cáo ra file (hoặc `localStorage` trên web) để đọc từ Dart.

## Cài đặt

```yaml
dependencies:
  hd_sentry:
    path: ../hd_sentry
```

## Khởi tạo

Gọi càng sớm càng tốt (trước `runApp`). `initialize()` tự gắn:

- Native crash handlers (signal / uncaught exception)
- `FlutterError.onError` (lỗi framework Flutter)
- `PlatformDispatcher.instance.onError` (lỗi async / isolate)

```dart
import 'package:flutter/material.dart';
import 'package:hd_sentry/hd_sentry.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await HdSentry.initialize();
  runApp(const MyApp());
}
```

Ghi tay lỗi Dart:

```dart
await HdSentry.captureException('something failed', stackTrace);
await HdSentry.captureError(error, stackTrace);
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
| ---------- | ------------------ | --------- |
| Android | `UncaughtExceptionHandler` | `filesDir/hd_sentry_crashes/` |
| iOS / macOS | `NSUncaughtExceptionHandler` + signal handlers | Application Support |
| Windows | `SetUnhandledExceptionFilter` + dbghelp symbolication | `%LOCALAPPDATA%/hd_sentry_crashes/` |
| Linux | Signal handlers | `~/.local/share/hd_sentry_crashes/` |
| Web | `error`, `unhandledrejection` | `localStorage` |

## Định dạng file

```txt
=== HD Sentry Native Crash Report ===
platform: android
timestamp: 2026-05-19T12:00:00.000Z
type: uncaught_exception
message: ...

--- stack trace ---
...
```

## Loại báo cáo (`type`)

| type | Nguồn |
|------|--------|
| `flutter_error` | `captureException` / Flutter error hooks |
| `uncaught_exception` | Native uncaught (Android Java, iOS NSException, …) |
| `signal` | Native signal (iOS/macOS/Linux) |
| `window_error` / `unhandled_rejection` | Web JS |

## Lưu ý

- `initialize()` đã bắt lỗi Flutter; không cần gán `FlutterError.onError` thủ công trừ khi bạn muốn logic riêng (handler cũ vẫn được gọi sau khi ghi file).
- Crash **native** vẫn do handler native bắt; `captureException` dùng cho lỗi Dart/Flutter.
- Sau khi ghi file, crash vẫn được chuyển tiếp cho handler mặc định của hệ điều hành.
- Trên **Windows**, stack trace dùng `SymFromAddr` (tên hàm khi có PDB) hoặc `tên_module.dll+0xoffset` nếu không có symbol; để đủ tên hàm C++ hãy build **Debug** hoặc ship `.pdb` cùng bản phát hành.

## Regenerate Pigeon

```bash
dart run pigeon --input pigeons/messages.dart
```
