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

### Windows: lấy `.pdb` khi build Release

- **Runner (app của bạn):** sau `flutter build windows` (hoặc `flutter run --release`), xem thư mục  
  **`build/windows/x64/runner/Release/`** (đường dẫn gốc là project chứa `pubspec.yaml`, ví dụ `example/` nếu bạn build từ app mẫu). Nếu MSVC tạo PDB cho Release thì sẽ có file **`TênApp.exe`** và **`TênApp.pdb`** cạnh nhau.
- **Nếu Release không ra `.pdb`:** mặc định một số cấu hình Release tối ưu hóa và không ghi PDB. Cách thường dùng: build **RelWithDebInfo** trong Visual Studio, hoặc chỉnh `windows/CMakeLists.txt` của **app** để bật sinh PDB (ví dụ cờ biên dịch `/Zi` và linker `/DEBUG` — xem tài liệu CMake MSVC).
- **`flutter_windows.dll`:** symbol engine thường kèm trong cache Flutter, thư mục kiểu  
  **`$FLUTTER_ROOT/bin/cache/artifacts/engine/windows-x64/`**  
  (có thể là `windows-x64-release` tùy bản SDK). Tìm **`flutter_windows.dll.pdb`** (nếu có) và đặt cạnh `flutter_windows.dll` trong bản giao cho người dùng khi cần đọc stack trong engine.

Trên **Windows**, stack trace dùng **dbghelp**: `SymFromAddr` (tên hàm) và **`SymGetLineFromAddr64`** để thêm **`(đường_dẫn\file.cpp:123)`** khi PDB có **bảng dòng** (không chỉ export). PDB chỉ có tên hàm mà không có file/line thường do build không bật thông tin dòng (`/Zi` hoặc tương đương) hoặc dùng PDB “tối giản”. Ưu tiên **RelWithDebInfo** / Release có **`/Zi`** + linker **`/DEBUG`**; copy đủ `.pdb` (app + `flutter_windows.dll.pdb`) cạnh file chạy.

## Regenerate Pigeon

```bash
dart run pigeon --input pigeons/messages.dart
```
