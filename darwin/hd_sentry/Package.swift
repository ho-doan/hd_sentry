// swift-tools-version: 5.9

import PackageDescription

let package = Package(
  name: "hd_sentry",
  platforms: [
    .iOS("12.0"),
    .macOS("10.14"),
  ],
  products: [
    .library(name: "hd-sentry", targets: ["hd_sentry"]),
  ],
  targets: [
    .target(
      name: "hd_sentry",
      path: "Sources/hd_sentry",
      resources: [
        .process("PrivacyInfo.xcprivacy"),
      ]
    ),
  ]
)
