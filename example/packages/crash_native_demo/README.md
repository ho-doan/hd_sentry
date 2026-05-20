# crash_native_demo

Helper plugin for **hd_sentry** example — triggers a real fatal crash per platform.

## API

```dart
await CrashNativeDemo().crashNative();
```

## Crash mechanism

| Platform | Behavior |
| ---------- | ---------- |
| **Android** | Uncaught `RuntimeException` on a background thread |
| **iOS / macOS** | `abort()` (SIGABRT) |
| **Windows** | Null pointer dereference (access violation) |
| **Linux** | Null pointer dereference (SIGSEGV) |
| **Web** | Uncaught JS `Error` via `setTimeout` (tab may freeze / show error; cannot kill OS process) |

After a native crash, reopen the app and read reports with `HdSentry.listCrashReports()`.
