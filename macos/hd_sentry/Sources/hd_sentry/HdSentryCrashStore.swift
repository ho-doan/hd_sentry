import Foundation

enum HdSentryCrashStore {
  private static let crashDirectoryName = "hd_sentry_crashes"
  private static let filePrefix = "crash_"
  private static let fileSuffix = ".txt"

  private static var crashDirectory: URL?

  static func configure() {
    let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
      ?? FileManager.default.temporaryDirectory
    let directory = base.appendingPathComponent(crashDirectoryName, isDirectory: true)
    try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    crashDirectory = directory
  }

  static func directory() throws -> URL {
    if crashDirectory == nil {
      configure()
    }
    guard let crashDirectory else {
      throw NSError(domain: "HdSentry", code: 1, userInfo: [NSLocalizedDescriptionKey: "Crash directory unavailable"])
    }
    return crashDirectory
  }

  static func listFileNames() throws -> [String] {
    let directory = try directory()
    let files = try FileManager.default.contentsOfDirectory(atPath: directory.path)
    return files
      .filter { $0.hasSuffix(fileSuffix) }
      .sorted(by: >)
  }

  static func readFile(fileName: String) throws -> String {
    try validate(fileName: fileName)
    let fileURL = try directory().appendingPathComponent(fileName)
    return try String(contentsOf: fileURL, encoding: .utf8)
  }

  @discardableResult
  static func deleteFile(fileName: String) throws -> Bool {
    try validate(fileName: fileName)
    let fileURL = try directory().appendingPathComponent(fileName)
    if FileManager.default.fileExists(atPath: fileURL.path) {
      try FileManager.default.removeItem(at: fileURL)
      return true
    }
    return false
  }

  static func clearAll() throws {
    let names = try listFileNames()
    for name in names {
      _ = try deleteFile(fileName: name)
    }
  }

  @discardableResult
  static func writeReport(
    platform: String,
    type: String,
    message: String,
    stackTrace: String,
    threadName: String? = nil
  ) throws -> String {
    let formatter = ISO8601DateFormatter()
    formatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
    let timestamp = formatter.string(from: Date())
    let fileName = "\(filePrefix)\(Int(Date().timeIntervalSince1970 * 1000))\(fileSuffix)"
    var body = """
    === HD Sentry Native Crash Report ===
    platform: \(platform)
    timestamp: \(timestamp)
    type: \(type)
    message: \(message)
    """
    if let threadName {
      body += "\nthread: \(threadName)"
    }
    body += """


    --- stack trace ---
    \(stackTrace)
    """
    let fileURL = try directory().appendingPathComponent(fileName)
    try body.write(to: fileURL, atomically: true, encoding: .utf8)
    return fileName
  }

  private static func validate(fileName: String) throws {
    if fileName.contains("/") || fileName.contains("..") {
      throw NSError(domain: "HdSentry", code: 2, userInfo: [NSLocalizedDescriptionKey: "Invalid crash file name"])
    }
  }
}
