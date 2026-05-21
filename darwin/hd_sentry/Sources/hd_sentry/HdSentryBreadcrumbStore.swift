import Foundation

/// Session breadcrumbs persisted beside crash reports. Used by [HdSentryCrashStore] and other
/// native code in the host app (requires [HdSentryCrashStore.configure] first).
public enum HdSentryBreadcrumbStore {
  public static let breadcrumbFileName = "session_breadcrumbs.log"
  public static let maxBreadcrumbs = 100
  private static let fieldSep = "\t"

  public static func breadcrumbFileURL(in crashDirectory: URL) -> URL {
    crashDirectory.appendingPathComponent(breadcrumbFileName)
  }

  public static func add(
    crashDirectory: URL,
    message: String,
    category: String? = nil,
    data: String? = nil
  ) throws {
    try FileManager.default.createDirectory(at: crashDirectory, withIntermediateDirectories: true)
    let fileURL = breadcrumbFileURL(in: crashDirectory)
    let line = formatLine(message: message, category: category, data: data)
    if FileManager.default.fileExists(atPath: fileURL.path) {
      let handle = try FileHandle(forWritingTo: fileURL)
      defer { try? handle.close() }
      try handle.seekToEnd()
      if let data = line.data(using: .utf8) {
        try handle.write(contentsOf: data)
      }
    } else {
      try line.write(to: fileURL, atomically: true, encoding: .utf8)
    }
    try trimToMaxLines(fileURL: fileURL)
  }

  public static func clear(crashDirectory: URL) throws {
    let fileURL = breadcrumbFileURL(in: crashDirectory)
    if FileManager.default.fileExists(atPath: fileURL.path) {
      try FileManager.default.removeItem(at: fileURL)
    }
  }

  /// Formatted `--- breadcrumbs ---` section for a crash report; clears the log after read.
  public static func consumeFormattedSection(crashDirectory: URL) -> String {
    let fileURL = breadcrumbFileURL(in: crashDirectory)
    guard FileManager.default.fileExists(atPath: fileURL.path),
      let raw = try? String(contentsOf: fileURL, encoding: .utf8)
    else {
      return ""
    }
    try? FileManager.default.removeItem(at: fileURL)
    let lines = raw.split(whereSeparator: \.isNewline).map(String.init).filter { !$0.isEmpty }
    guard !lines.isEmpty else { return "" }
    var body = "\n--- breadcrumbs ---\n"
    for line in lines {
      body += line + "\n"
    }
    return body
  }

  private static func formatLine(message: String, category: String?, data: String?) -> String {
    let formatter = ISO8601DateFormatter()
    formatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
    let timestamp = formatter.string(from: Date())
    let cat = sanitizeField(category ?? "")
    let msg = sanitizeField(message)
    let extra = sanitizeField(data ?? "")
    return "\(timestamp)\(fieldSep)\(cat)\(fieldSep)\(msg)\(fieldSep)\(extra)\n"
  }

  private static func sanitizeField(_ value: String) -> String {
    value
      .replacingOccurrences(of: "\t", with: " ")
      .replacingOccurrences(of: "\n", with: " ")
      .replacingOccurrences(of: "\r", with: " ")
  }

  private static func trimToMaxLines(fileURL: URL) throws {
    guard let raw = try? String(contentsOf: fileURL, encoding: .utf8) else { return }
    var lines = raw.split(whereSeparator: \.isNewline).map(String.init)
    if lines.count <= maxBreadcrumbs { return }
    lines = Array(lines.suffix(maxBreadcrumbs))
    try lines.joined(separator: "\n").appending("\n").write(to: fileURL, atomically: true, encoding: .utf8)
  }
}
