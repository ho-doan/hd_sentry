#
# Shared CocoaPods spec for iOS and macOS (sharedDarwinSource in pubspec.yaml).
# Flutter also resolves this from ios/ and macos/ via thin wrapper podspecs.
#
Pod::Spec.new do |s|
  s.name             = 'hd_sentry'
  s.version          = '0.0.1'
  s.summary          = 'Capture native crashes on mobile, desktop, and web and read crash files from Dart.'
  s.description      = <<-DESC
Capture native crashes on mobile, desktop, and web and read crash files from Dart.
                       DESC
  s.homepage         = 'https://github.com/hodoan/hd_sentry'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Hodoan' => 'hodoan@gmail.com' }
  s.source           = { :path => '.' }
  s.source_files     = 'darwin/hd_sentry/Sources/hd_sentry/**/*'
  s.resource_bundles = { 'hd_sentry_privacy' => ['darwin/hd_sentry/Sources/hd_sentry/PrivacyInfo.xcprivacy'] }

  s.ios.dependency 'Flutter'
  s.osx.dependency 'FlutterMacOS'

  s.ios.deployment_target = '13.0'
  s.osx.deployment_target = '10.14'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386',
  }
  s.swift_version = '5.0'
end
