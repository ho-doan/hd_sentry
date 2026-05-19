package com.hodoan.hd_sentry

import android.content.Context
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone

internal object HdSentryCrashStore {
    private const val CRASH_DIR = "hd_sentry_crashes"
    private const val FILE_PREFIX = "crash_"
    private const val FILE_SUFFIX = ".txt"

    @Volatile
    private var crashDirectory: File? = null

    fun configure(context: Context) {
        crashDirectory = File(context.filesDir, CRASH_DIR).apply { mkdirs() }
    }

    fun directory(): File {
        return crashDirectory
            ?: throw IllegalStateException("HdSentryCrashStore is not configured")
    }

    fun listFileNames(): List<String> {
        return directory()
            .listFiles { file -> file.isFile && file.name.endsWith(FILE_SUFFIX) }
            ?.map { it.name }
            ?.sortedDescending()
            ?: emptyList()
    }

    fun readFile(fileName: String): String {
        validateFileName(fileName)
        return File(directory(), fileName).readText()
    }

    fun deleteFile(fileName: String): Boolean {
        validateFileName(fileName)
        return File(directory(), fileName).delete()
    }

    fun clearAll() {
        directory().listFiles()?.forEach { file ->
            if (file.isFile) {
                file.delete()
            }
        }
    }

    fun writeReport(
        platform: String,
        type: String,
        message: String,
        stackTrace: String,
        threadName: String? = null,
    ): String {
        val timestamp = isoTimestamp()
        val fileName = "${FILE_PREFIX}${System.currentTimeMillis()}$FILE_SUFFIX"
        val body = buildString {
            appendLine("=== HD Sentry Native Crash Report ===")
            appendLine("platform: $platform")
            appendLine("timestamp: $timestamp")
            appendLine("type: $type")
            appendLine("message: $message")
            if (threadName != null) {
                appendLine("thread: $threadName")
            }
            appendLine()
            appendLine("--- stack trace ---")
            append(stackTrace)
        }
        File(directory(), fileName).writeText(body)
        return fileName
    }

    private fun validateFileName(fileName: String) {
        require(!fileName.contains("/") && !fileName.contains("..")) {
            "Invalid crash file name"
        }
    }

    private fun isoTimestamp(): String {
        val formatter = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US)
        formatter.timeZone = TimeZone.getTimeZone("UTC")
        return formatter.format(Date())
    }
}
