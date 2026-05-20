package com.hodoan.hd_sentry

import io.flutter.embedding.engine.plugins.FlutterPlugin

class HdSentryPlugin : FlutterPlugin, HdSentryHostApi {
    private var applicationContext: android.content.Context? = null

    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        applicationContext = binding.applicationContext
        HdSentryHostApi.setUp(binding.binaryMessenger, this)
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        HdSentryHostApi.setUp(binding.binaryMessenger, null)
        applicationContext = null
    }

    override fun initialize() {
        val context = applicationContext
            ?: throw IllegalStateException("HdSentryPlugin is not attached")
        HdSentryCrashHandler.install(context)
    }

    override fun getCrashDirectory(): String {
        ensureStoreConfigured()
        return HdSentryCrashStore.directory().absolutePath
    }

    override fun listCrashFileNames(): List<String> = HdSentryCrashStore.listFileNames()

    override fun readCrashFile(fileName: String): String = HdSentryCrashStore.readFile(fileName)

    override fun deleteCrashFile(fileName: String): Boolean = HdSentryCrashStore.deleteFile(fileName)

    override fun clearAllCrashFiles() {
        HdSentryCrashStore.clearAll()
    }

    override fun captureException(message: String, stackTrace: String?) {
        ensureStoreConfigured()
        HdSentryCrashStore.writeReport(
            platform = "android",
            type = "flutter_error",
            message = message,
            stackTrace = stackTrace.orEmpty(),
        )
    }

    private fun ensureStoreConfigured() {
        if (applicationContext != null) {
            HdSentryCrashStore.configure(applicationContext!!)
        }
    }
}
