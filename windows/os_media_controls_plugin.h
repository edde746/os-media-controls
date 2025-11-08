#ifndef FLUTTER_PLUGIN_OS_MEDIA_CONTROLS_PLUGIN_H_
#define FLUTTER_PLUGIN_OS_MEDIA_CONTROLS_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>

#include <memory>
#include <string>
#include <vector>

// C++/WinRT headers for SystemMediaTransportControls
#include <winrt/base.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.h>

namespace os_media_controls {

class OsMediaControlsPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  OsMediaControlsPlugin(flutter::PluginRegistrarWindows *registrar);

  virtual ~OsMediaControlsPlugin();

  // Disallow copy and assign.
  OsMediaControlsPlugin(const OsMediaControlsPlugin&) = delete;
  OsMediaControlsPlugin& operator=(const OsMediaControlsPlugin&) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // Event sink for sending events to Dart
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;

  // SystemMediaTransportControls instance
  winrt::Windows::Media::SystemMediaTransportControls smtc_{nullptr};

  // Event tokens for SMTC event handlers
  winrt::event_token button_pressed_token_;
  winrt::event_token position_change_token_;

  // Flutter window HWND (needed for SMTC initialization)
  HWND hwnd_{nullptr};

  // Store registrar reference
  flutter::PluginRegistrarWindows* registrar_;

  // Helper methods for plugin functionality
  void SetMetadata(const flutter::EncodableValue *args);
  void SetPlaybackState(const flutter::EncodableValue *args);
  void SendEvent(const flutter::EncodableMap &event);
  std::string GetStringFromMap(const flutter::EncodableMap &map, const std::string &key);
  double GetDoubleFromMap(const flutter::EncodableMap &map, const std::string &key);

  // SMTC-specific helper methods
  void InitializeSMTC(HWND hwnd);
  void CleanupSMTC();
  void HandleButtonPressed(winrt::Windows::Media::SystemMediaTransportControlsButton button);
  void EnableControl(const std::string& control);
  void DisableControl(const std::string& control);

  // WinRT conversion helpers
  winrt::hstring StringToHString(const std::string& str);
  winrt::Windows::Storage::Streams::RandomAccessStreamReference
      CreateStreamReferenceFromBytes(const std::vector<uint8_t>& bytes);
};

}  // namespace os_media_controls

#endif  // FLUTTER_PLUGIN_OS_MEDIA_CONTROLS_PLUGIN_H_
