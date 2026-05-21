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
| iOS / macOS | `NSUncaughtExceptionHandler` + signal handlers (shared **`darwin/`**); báo cáo có **likely origin** + gợi ý `atos` / `swift demangle` | Application Support |
| Windows | `SetUnhandledExceptionFilter` + dbghelp + **minidump `.dmp`** | `%LOCALAPPDATA%/hd_sentry_crashes/` |
| Linux | Signal handlers + **ELF core `.dmp`** + `.maps` | `~/.local/share/hd_sentry_crashes/` |
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
| ------ | -------- |
| `flutter_error` | `captureException` / Flutter error hooks |
| `uncaught_exception` | Native uncaught (Android Java, iOS NSException, …) |
| `signal` | Native signal (iOS/macOS/Linux) |
| `window_error` / `unhandled_rejection` | Web JS |

## Lưu ý

- `initialize()` đã bắt lỗi Flutter; không cần gán `FlutterError.onError` thủ công trừ khi bạn muốn logic riêng (handler cũ vẫn được gọi sau khi ghi file).
- Crash **native** vẫn do handler native bắt; `captureException` dùng cho lỗi Dart/Flutter.
- Sau khi ghi file, crash vẫn được chuyển tiếp cho handler mặc định của hệ điều hành.
- **Windows (native crash):** ghi song song **`crash_<millis>.txt`** và **`crash_<millis>.dmp`** (MiniDump, mở bằng WinDbg). API Dart `listCrashFileNames` / `listCrashReports` chỉ trả **`.txt`**; mở `getCrashDirectory()` trong Explorer để thấy `.dmp`. `readCrashFile` không đọc `.dmp` (nhị phân). `clearAllCrashFiles` xóa cả hai loại.
- **Linux (native crash):** tương tự — **`crash_<millis>.txt`**, **`crash_<millis>.dmp`** (ELF core), **`crash_<millis>.maps`**, **`crash_<millis>.gdb`** (script gdb). `listCrashReports` chỉ liệt kê `.txt`; `readCrashFile` từ chối `.dmp`. `clearAllCrashFiles` xóa tất cả các loại trên.

### Windows: lấy `.pdb` khi build Release

- **Runner (app của bạn):** sau `flutter build windows` (hoặc `flutter run --release`), xem thư mục  
  **`build/windows/x64/runner/Release/`** (đường dẫn gốc là project chứa `pubspec.yaml`, ví dụ `example/` nếu bạn build từ app mẫu). Nếu MSVC tạo PDB cho Release thì sẽ có file **`TênApp.exe`** và **`TênApp.pdb`** cạnh nhau.
- **Nếu Release không ra `.pdb`:** mặc định một số cấu hình Release tối ưu hóa và không ghi PDB. Cách thường dùng: build **RelWithDebInfo** trong Visual Studio, hoặc chỉnh `windows/CMakeLists.txt` của **app** để bật sinh PDB (ví dụ cờ biên dịch `/Zi` và linker `/DEBUG` — xem tài liệu CMake MSVC).
- **`flutter_windows.dll`:** symbol engine thường kèm trong cache Flutter, thư mục kiểu  
  **`$FLUTTER_ROOT/bin/cache/artifacts/engine/windows-x64/`**  
  (có thể là `windows-x64-release` tùy bản SDK). Tìm **`flutter_windows.dll.pdb`** (nếu có) và đặt cạnh `flutter_windows.dll` trong bản giao cho người dùng khi cần đọc stack trong engine.

**WinDbg:** `File` → `Open crash dump` → chọn `crash_….dmp` trong thư mục crash; nạp thêm PDB của app/engine nếu cần symbol/file:line.

Trên **Windows**, stack trong file `.txt` dùng **dbghelp** (`SymFromAddr`, `SymGetLineFromAddr64` / bản Unicode tương ứng) khi có PDB đủ bảng dòng (`/Zi`, linker `/DEBUG`).

Trên **Linux** (chưa hoàn thiện), stack native dùng `backtrace` / `dladdr` (glibc). Build **debug** hoặc giữ symbol (`-g`, không strip) để thấy tên hàm thay vì chỉ địa chỉ; có thể dùng `addr2line -e <binary> <addr>` với các dòng trong báo cáo.

**gdb (Linux core):** (chưa hoàn thiện) build **debug** (`flutter run -d linux` / `flutter build linux --debug`). Trong thư mục crash có `crash_<millis>.gdb` — chạy:

```bash
gdb -x ~/.local/share/hd_sentry_crashes/crash_<millis>.gdb \
  example/build/linux/x64/debug/bundle/hd_sentry_example
```

Hoặc thủ công:

```bash
cd example/build/linux/x64/debug/bundle
gdb ./hd_sentry_example ~/.local/share/hd_sentry_crashes/crash_<millis>.dmp
(gdb) set solib-search-path ./lib
(gdb) bt full
(gdb) info symbol $pc
```

`#0 ?? ()` tại địa chỉ `0x7f…` thường là crash trong **.so** (plugin `libcrash_native_demo_plugin.so`, `libflutter_linux_gtk.so`, libc) — cần debug build và `solib-search-path` trỏ tới `bundle/lib`. Địa chỉ trong plugin: `addr2line -e lib/libcrash_native_demo_plugin.so -f -C 0x<offset>`.

## Regenerate Pigeon

```bash
dart run pigeon --input pigeons/messages.dart
```

Swift/ObjC host code cho **iOS + macOS** nằm tại `darwin/hd_sentry/Sources/hd_sentry/` (`sharedDarwinSource: true` trong `pubspec.yaml`). Không chỉnh trực tiếp `ios/` hay `macos/` — hai thư mục đó chỉ còn podspec trỏ về `darwin/`.
