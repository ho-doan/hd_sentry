import 'package:flutter_test/flutter_test.dart';
import 'package:hd_sentry/hd_sentry.dart';
import 'package:integration_test/integration_test.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('initialize and read crash directory', (tester) async {
    await HdSentry.initialize();
    final directory = await HdSentry.getCrashDirectory();
    expect(directory.isNotEmpty, isTrue);
    final reports = await HdSentry.listCrashReports();
    expect(reports, isA<List<NativeCrashReport>>());
  });
}
