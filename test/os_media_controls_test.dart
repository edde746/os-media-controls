import 'package:flutter_test/flutter_test.dart';
import 'package:os_media_controls/os_media_controls.dart';
import 'package:os_media_controls/os_media_controls_platform_interface.dart';
import 'package:os_media_controls/os_media_controls_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockOsMediaControlsPlatform
    with MockPlatformInterfaceMixin
    implements OsMediaControlsPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final OsMediaControlsPlatform initialPlatform = OsMediaControlsPlatform.instance;

  test('$MethodChannelOsMediaControls is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelOsMediaControls>());
  });

  test('getPlatformVersion', () async {
    OsMediaControls osMediaControlsPlugin = OsMediaControls();
    MockOsMediaControlsPlatform fakePlatform = MockOsMediaControlsPlatform();
    OsMediaControlsPlatform.instance = fakePlatform;

    expect(await osMediaControlsPlugin.getPlatformVersion(), '42');
  });
}
