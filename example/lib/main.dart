import 'package:flutter/material.dart';
import 'package:hd_sentry/hd_sentry.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await HdSentry.initialize();
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  List<NativeCrashReport> _reports = const [];
  String? _crashDirectory;

  @override
  void initState() {
    super.initState();
    _refresh();
  }

  Future<void> _refresh() async {
    final directory = await HdSentry.getCrashDirectory();
    final reports = await HdSentry.listCrashReports();
    if (!mounted) return;
    setState(() {
      _crashDirectory = directory;
      _reports = reports;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('hd_sentry example'),
          actions: [
            IconButton(
              onPressed: _refresh,
              icon: const Icon(Icons.refresh),
            ),
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
                            builder: (_) => AlertDialog(
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
              heroTag: 'crash',
              onPressed: () => throw Exception('Dart test crash'),
              label: const Text('Dart crash'),
              icon: const Icon(Icons.bug_report),
            ),
          ],
        ),
      ),
    );
  }
}
