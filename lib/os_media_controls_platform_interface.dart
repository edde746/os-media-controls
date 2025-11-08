import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'os_media_controls_method_channel.dart';

abstract class OsMediaControlsPlatform extends PlatformInterface {
  /// Constructs a OsMediaControlsPlatform.
  OsMediaControlsPlatform() : super(token: _token);

  static final Object _token = Object();

  static OsMediaControlsPlatform _instance = MethodChannelOsMediaControls();

  /// The default instance of [OsMediaControlsPlatform] to use.
  ///
  /// Defaults to [MethodChannelOsMediaControls].
  static OsMediaControlsPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [OsMediaControlsPlatform] when
  /// they register themselves.
  static set instance(OsMediaControlsPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
