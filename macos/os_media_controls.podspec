#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint os_media_controls.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'os_media_controls'
  s.version          = '0.0.1'
  s.summary          = 'OS-level media controls for Flutter'
  s.description      = <<-DESC
A Flutter plugin for integrating with OS-level media controls across iOS, macOS, Android, and Windows.
                       DESC
  s.homepage         = 'http://example.com'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Your Company' => 'email@example.com' }
  s.source           = { :path => '.' }
  s.source_files = 'Classes/**/*'
  s.dependency 'FlutterMacOS'
  s.platform = :osx, '10.14'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'
end
