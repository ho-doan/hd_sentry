import Foundation

enum HdSentryPlatform {
  static var current: String {
    #if os(iOS)
    return "ios"
    #elseif os(macOS)
    return "macos"
    #else
    return "darwin"
    #endif
  }
}
