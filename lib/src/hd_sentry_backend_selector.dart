import 'hd_sentry_backend.dart';
import 'hd_sentry_native_backend.dart'
    if (dart.library.js_interop) 'hd_sentry_web_backend.dart';

HdSentryBackend createHdSentryBackend() => createBackend();
