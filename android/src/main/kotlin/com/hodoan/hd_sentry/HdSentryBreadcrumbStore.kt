package com.hodoan.hd_sentry

import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone

/**
 * Session breadcrumbs persisted beside crash reports. Shared by [HdSentryCrashStore] and
 * other native code in the host app (call after [HdSentryCrashStore.configure]).
 */
object HdSentryBreadcrumbStore {
    const val BREADCRUMB_FILE_NAME = "session_breadcrumbs.log"
    const val MAX_BREADCRUMBS = 100
    private const val FIELD_SEP = '\t'

    fun breadcrumbFile(crashDirectory: File): File =
        File(crashDirectory, BREADCRUMB_FILE_NAME)

    fun add(
        crashDirectory: File,
        message: String,
        category: String? = null,
        data: String? = null,
    ) {
        crashDirectory.mkdirs()
        val file = breadcrumbFile(crashDirectory)
        val line = formatLine(message, category, data)
        file.appendText(line)
        trimToMaxLines(file)
    }

    fun clear(crashDirectory: File) {
        breadcrumbFile(crashDirectory).delete()
    }

    /** Formatted `--- breadcrumbs ---` section for a crash report; clears the log after read. */
    fun consumeFormattedSection(crashDirectory: File): String {
        val file = breadcrumbFile(crashDirectory)
        if (!file.isFile) return ""
        val lines = file.readLines().filter { it.isNotBlank() }
        file.delete()
        if (lines.isEmpty()) return ""
        return buildString {
            appendLine()
            appendLine("--- breadcrumbs ---")
            lines.forEach { appendLine(it) }
        }
    }

    private fun formatLine(message: String, category: String?, data: String?): String {
        val timestamp = isoTimestamp()
        val cat = sanitizeField(category.orEmpty())
        val msg = sanitizeField(message)
        val extra = sanitizeField(data.orEmpty())
        return "$timestamp$FIELD_SEP$cat$FIELD_SEP$msg$FIELD_SEP$extra\n"
    }

    private fun sanitizeField(value: String): String =
        value.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ')

    private fun trimToMaxLines(file: File) {
        val lines = file.readLines()
        if (lines.size <= MAX_BREADCRUMBS) return
        file.writeText(lines.takeLast(MAX_BREADCRUMBS).joinToString("\n", postfix = "\n"))
    }

    private fun isoTimestamp(): String {
        val formatter = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US)
        formatter.timeZone = TimeZone.getTimeZone("UTC")
        return formatter.format(Date())
    }
}
