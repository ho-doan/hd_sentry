import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(
  PigeonOptions(
    dartOut: 'lib/src/pigeon/messages.g.dart',
    dartPackageName: 'hd_sentry',
    dartOptions: DartOptions(),
    kotlinOut: 'android/src/main/kotlin/com/hodoan/hd_sentry/Messages.g.kt',
    kotlinOptions: KotlinOptions(package: 'com.hodoan.hd_sentry'),
    // sharedDarwinSource: ios + macos compile from darwin/
    swiftOut: 'darwin/hd_sentry/Sources/hd_sentry/Messages.g.swift',
    swiftOptions: SwiftOptions(),
    // objcHeaderOut: 'ios/hd_sentry/Sources/hd_sentry/Messages.g.h',
    // objcSourceOut: 'ios/hd_sentry/Sources/hd_sentry/Messages.g.m',
    cppHeaderOut: 'windows/messages.g.h',
    cppSourceOut: 'windows/messages.g.cpp',
    cppOptions: CppOptions(namespace: 'hd_sentry'),
    gobjectHeaderOut: 'linux/messages.g.h',
    // GObject codegen uses nullptr — must compile as C++ (.cc), not .c.
    gobjectSourceOut: 'linux/messages.g.cc',
    gobjectOptions: GObjectOptions(module: 'hd_sentry'),
    // cppHeaderOutWindows: 'windows/messages.g.h',
    // cppSourceOutWindows: 'windows/messages.g.cpp',
    // cppHeaderOutLinux: 'linux/messages.g.h',
    // cppSourceOutLinux: 'linux/messages.g.cpp',
    // cppOptionsLinux: CppOptions(namespace: 'hd_sentry'),
  ),
)
@HostApi()
abstract class HdSentryHostApi {
  /// Installs native crash handlers. Call as early as possible (e.g. before [runApp]).
  void initialize();

  /// Absolute path to the directory where crash reports are stored.
  String getCrashDirectory();

  /// File names (not full paths) of pending crash reports.
  List<String> listCrashFileNames();

  /// Full text content of a crash report file.
  String readCrashFile(String fileName);

  /// Deletes a single crash report. Returns true if the file existed.
  bool deleteCrashFile(String fileName);

  /// Deletes all crash reports in [getCrashDirectory].
  void clearAllCrashFiles();

  /// Captures an exception.
  void captureException(String message, [String? stackTrace]);

  /// Appends a breadcrumb to the session log (persisted on disk for native crashes).
  void addBreadcrumb(String message, String? category, String? data);

  /// Clears persisted session breadcrumbs.
  void clearBreadcrumbs();
}
