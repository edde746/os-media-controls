import Flutter
import UIKit
import AVFoundation

@main
@objc class AppDelegate: FlutterAppDelegate {
  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    // Configure audio session for media playback and Now Playing controls
    do {
      let audioSession = AVAudioSession.sharedInstance()
      try audioSession.setCategory(.playback, mode: .default, options: [])
      try audioSession.setActive(true)
      print("✅ Audio session configured successfully for media playback")
    } catch {
      print("❌ Failed to configure audio session: \(error.localizedDescription)")
    }

    // Enable remote control events for Control Center and lock screen controls
    application.beginReceivingRemoteControlEvents()

    GeneratedPluginRegistrant.register(with: self)
    return super.application(application, didFinishLaunchingWithOptions: launchOptions)
  }

  override func applicationWillTerminate(_ application: UIApplication) {
    // Clean up remote control events
    application.endReceivingRemoteControlEvents()
    super.applicationWillTerminate(application)
  }
}
