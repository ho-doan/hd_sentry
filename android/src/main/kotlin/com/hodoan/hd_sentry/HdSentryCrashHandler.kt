package com.hodoan.hd_sentry

import android.content.Context
import android.util.Log

internal object HdSentryCrashHandler {
    private const val TAG = "HdSentry"

    @Volatile
    private var installed = false

    private var defaultHandler: Thread.UncaughtExceptionHandler? = null

    fun install(context: Context) {
        if (installed) return
        installed = true

        HdSentryCrashStore.configure(context.applicationContext)
        defaultHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            try {
                HdSentryCrashStore.writeReport(
                    platform = "android",
                    type = "uncaught_exception",
                    message = throwable.message ?: throwable.javaClass.name,
                    stackTrace = Log.getStackTraceString(throwable),
                    threadName = thread.name,
                )
            } catch (error: Throwable) {
                Log.e(TAG, "Failed to persist crash report", error)
            }
            defaultHandler?.uncaughtException(thread, throwable)
        }
    }
}
