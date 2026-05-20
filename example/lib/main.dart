import 'package:flutter/material.dart';
import 'package:hd_sentry/hd_sentry.dart';
import 'package:crash_native_demo/crash_native_demo.dart' as crash_native_demo;

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await HdSentry.initialize();
  FlutterError.onError = (details) {
    HdSentry.captureException(
      details.exception.toString(),
      details.stack?.toString(),
    );
  };
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(home: const HomeScreen());
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  List<NativeCrashReport> _reports = const [];
  String? _crashDirectory;

  @override
  void initState() {
    super.initState();
    _refresh(false);
  }

  Future<void> _refresh([bool isShowDialog = true]) async {
    if (isShowDialog) {
      showDialog(
        context: context,
        builder: (context) => const AlertDialog(title: Text('Refreshing...')),
      );
    }
    try {
      final directory = await HdSentry.getCrashDirectory();
      final reports = await HdSentry.listCrashReports();
      if (!mounted) return;
      setState(() {
        _crashDirectory = directory;
        _reports = reports;
      });
    } catch (e) {
      if (!mounted) return;
      if (isShowDialog) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text(e.toString())));
      }
    } finally {
      if (isShowDialog) {
        Navigator.of(context).pop();
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('hd_sentry example'),
        actions: [
          IconButton(onPressed: _refresh, icon: const Icon(Icons.refresh)),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Crash directory: ${_crashDirectory ?? '...'}'),
            const SizedBox(height: 12),
            Text('Pending reports: ${_reports.length}'),
            const SizedBox(height: 12),
            Expanded(
              child: ListView.builder(
                itemCount: _reports.length,
                itemBuilder: (context, index) {
                  final report = _reports[index];
                  return Card(
                    child: ListTile(
                      title: Text(report.fileName),
                      subtitle: Text(
                        '${report.platform} • ${report.type ?? '-'} • ${report.message ?? '-'}',
                      ),
                      onTap: () {
                        showDialog<void>(
                          context: context,
                          builder:
                              (_) => AlertDialog(
                                title: Text(report.fileName),
                                content: SingleChildScrollView(
                                  child: Text(report.content),
                                ),
                                actions: [
                                  TextButton(
                                    onPressed: () => Navigator.pop(context),
                                    child: const Text('Close'),
                                  ),
                                ],
                              ),
                        );
                      },
                    ),
                  );
                },
              ),
            ),
          ],
        ),
      ),
      floatingActionButton: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.end,
        children: [
          FloatingActionButton.extended(
            heroTag: 'clear',
            onPressed: () async {
              await HdSentry.clearAllCrashFiles();
              await _refresh();
            },
            label: const Text('Clear'),
            icon: const Icon(Icons.delete_outline),
          ),
          const SizedBox(height: 12),
          FloatingActionButton.extended(
            heroTag: 'native_crash',
            onPressed: () => crash_native_demo.CrashNativeDemo().crashNative(),
            label: const Text('Native crash'),
            icon: const Icon(Icons.bug_report),
          ),
          const SizedBox(height: 12),
          FloatingActionButton.extended(
            heroTag: 'crash',
            onPressed: () => throw Exception('Dart test crash'),
            label: const Text('Dart crash'),
            icon: const Icon(Icons.bug_report),
          ),
        ],
      ),
    );
  }
}
