import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'os_media_controls_platform_interface.dart';

/// An implementation of [OsMediaControlsPlatform] that uses method channels.
class MethodChannelOsMediaControls extends OsMediaControlsPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('os_media_controls');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>('getPlatformVersion');
    return version;
  }
}
