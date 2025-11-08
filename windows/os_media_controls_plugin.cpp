#include "os_media_controls_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>

#include <memory>
#include <sstream>
#include <codecvt>
#include <locale>

// Additional WinRT headers for SMTC
#include <SystemMediaTransportControlsInterop.h>

using namespace winrt;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Foundation;

namespace os_media_controls {

// static
void OsMediaControlsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto method_channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "com.example.os_media_controls/methods",
          &flutter::StandardMethodCodec::GetInstance());

  auto event_channel =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          registrar->messenger(), "com.example.os_media_controls/events",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<OsMediaControlsPlugin>(registrar);

  method_channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  auto handler = std::make_unique<flutter::StreamHandlerFunctions<>>(
      [plugin_pointer = plugin.get()](
          const flutter::EncodableValue* arguments,
          std::unique_ptr<flutter::EventSink<>>&& events)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        plugin_pointer->event_sink_ = std::move(events);
        return nullptr;
      },
      [plugin_pointer = plugin.get()](const flutter::EncodableValue* arguments)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        plugin_pointer->event_sink_.reset();
        return nullptr;
      });

  event_channel->SetStreamHandler(std::move(handler));

  registrar->AddPlugin(std::move(plugin));
}

OsMediaControlsPlugin::OsMediaControlsPlugin(
    flutter::PluginRegistrarWindows *registrar)
    : registrar_(registrar) {

  // Initialize WinRT apartment
  try {
    winrt::init_apartment();
  } catch (...) {
    // Apartment may already be initialized
  }

  // Get HWND from Flutter window view
  auto view = registrar_->GetView();
  if (view) {
    hwnd_ = view->GetNativeWindow();
    InitializeSMTC(hwnd_);
  }
}

OsMediaControlsPlugin::~OsMediaControlsPlugin() {
  CleanupSMTC();
  try {
    winrt::uninit_apartment();
  } catch (...) {
    // Ignore cleanup errors
  }
}

void OsMediaControlsPlugin::InitializeSMTC(HWND hwnd) {
  if (!hwnd) return;

  try {
    // Get ISystemMediaTransportControlsInterop interface for desktop apps
    auto interop = winrt::get_activation_factory<
        SystemMediaTransportControls,
        ISystemMediaTransportControlsInterop>();

    // Get SMTC instance for this window
    winrt::guid guid = winrt::guid_of<SystemMediaTransportControls>();
    winrt::check_hresult(interop->GetForWindow(
        hwnd,
        guid,
        winrt::put_abi(smtc_)
    ));

    // Enable basic controls by default
    smtc_.IsPlayEnabled(true);
    smtc_.IsPauseEnabled(true);
    smtc_.IsNextEnabled(false);
    smtc_.IsPreviousEnabled(false);
    smtc_.IsStopEnabled(false);

    // Register button pressed event handler
    button_pressed_token_ = smtc_.ButtonPressed(
        [this](SystemMediaTransportControls const&,
               SystemMediaTransportControlsButtonPressedEventArgs const& args) {
          HandleButtonPressed(args.Button());
        });

    // Register playback position change event handler (for seek)
    position_change_token_ = smtc_.PlaybackPositionChangeRequested(
        [this](SystemMediaTransportControls const&,
               PlaybackPositionChangeRequestedEventArgs const& args) {
          // Convert TimeSpan (100-nanosecond units) to seconds
          double positionSeconds = args.RequestedPlaybackPosition().count() / 10000000.0;

          flutter::EncodableMap event;
          event[flutter::EncodableValue("type")] = flutter::EncodableValue("seek");
          event[flutter::EncodableValue("position")] = flutter::EncodableValue(positionSeconds);

          SendEvent(event);
        });

  } catch (winrt::hresult_error const& ex) {
    // Log error but don't crash - SMTC may not be available on all Windows versions
  }
}

void OsMediaControlsPlugin::CleanupSMTC() {
  if (smtc_) {
    try {
      // Unregister event handlers
      smtc_.ButtonPressed(button_pressed_token_);
      smtc_.PlaybackPositionChangeRequested(position_change_token_);

      // Clear display
      smtc_.DisplayUpdater().ClearAll();
      smtc_.DisplayUpdater().Update();

      smtc_ = nullptr;
    } catch (...) {
      // Ignore cleanup errors
    }
  }
}

void OsMediaControlsPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto &method_name = method_call.method_name();

  if (method_name == "setMetadata") {
    SetMetadata(method_call.arguments());
    result->Success(flutter::EncodableValue(nullptr));
  } else if (method_name == "setPlaybackState") {
    SetPlaybackState(method_call.arguments());
    result->Success(flutter::EncodableValue(nullptr));
  } else if (method_name == "enableControls") {
    if (auto args = method_call.arguments()) {
      if (std::holds_alternative<flutter::EncodableList>(*args)) {
        const auto& list = std::get<flutter::EncodableList>(*args);
        for (const auto& item : list) {
          if (std::holds_alternative<std::string>(item)) {
            EnableControl(std::get<std::string>(item));
          }
        }
      }
    }
    result->Success(flutter::EncodableValue(nullptr));
  } else if (method_name == "disableControls") {
    if (auto args = method_call.arguments()) {
      if (std::holds_alternative<flutter::EncodableList>(*args)) {
        const auto& list = std::get<flutter::EncodableList>(*args);
        for (const auto& item : list) {
          if (std::holds_alternative<std::string>(item)) {
            DisableControl(std::get<std::string>(item));
          }
        }
      }
    }
    result->Success(flutter::EncodableValue(nullptr));
  } else if (method_name == "setSkipIntervals") {
    // Windows doesn't support custom skip intervals
    result->Success(flutter::EncodableValue(nullptr));
  } else if (method_name == "setQueueInfo") {
    // Windows doesn't display queue info
    result->Success(flutter::EncodableValue(nullptr));
  } else if (method_name == "clear") {
    if (smtc_) {
      try {
        smtc_.DisplayUpdater().ClearAll();
        smtc_.DisplayUpdater().Update();
        smtc_.PlaybackStatus(MediaPlaybackStatus::Closed);
      } catch (...) {
        // Ignore errors
      }
    }
    result->Success(flutter::EncodableValue(nullptr));
  } else {
    result->NotImplemented();
  }
}

void OsMediaControlsPlugin::SetMetadata(const flutter::EncodableValue *args) {
  if (!args || !std::holds_alternative<flutter::EncodableMap>(*args) || !smtc_) {
    return;
  }

  const auto &map = std::get<flutter::EncodableMap>(*args);

  try {
    auto updater = smtc_.DisplayUpdater();
    updater.Type(MediaPlaybackType::Music);

    // Set music properties
    auto musicProps = updater.MusicProperties();

    auto title = GetStringFromMap(map, "title");
    if (!title.empty()) {
      musicProps.Title(StringToHString(title));
    }

    auto artist = GetStringFromMap(map, "artist");
    if (!artist.empty()) {
      musicProps.Artist(StringToHString(artist));
    }

    auto album = GetStringFromMap(map, "album");
    if (!album.empty()) {
      musicProps.AlbumTitle(StringToHString(album));
    }

    auto albumArtist = GetStringFromMap(map, "albumArtist");
    if (!albumArtist.empty()) {
      musicProps.AlbumArtist(StringToHString(albumArtist));
    }

    // Handle artwork if provided
    auto artwork_it = map.find(flutter::EncodableValue("artwork"));
    if (artwork_it != map.end() &&
        std::holds_alternative<std::vector<uint8_t>>(artwork_it->second)) {
      auto artwork_bytes = std::get<std::vector<uint8_t>>(artwork_it->second);
      if (!artwork_bytes.empty()) {
        auto streamRef = CreateStreamReferenceFromBytes(artwork_bytes);
        if (streamRef) {
          updater.Thumbnail(streamRef);
        }
      }
    }

    // Apply updates
    updater.Update();

  } catch (winrt::hresult_error const& ex) {
    // Ignore metadata update errors
  }
}

void OsMediaControlsPlugin::SetPlaybackState(const flutter::EncodableValue *args) {
  if (!args || !std::holds_alternative<flutter::EncodableMap>(*args) || !smtc_) {
    return;
  }

  const auto &map = std::get<flutter::EncodableMap>(*args);

  try {
    auto state_str = GetStringFromMap(map, "state");
    auto position = GetDoubleFromMap(map, "position");
    auto speed = GetDoubleFromMap(map, "speed");

    // Set playback status
    if (state_str == "playing") {
      smtc_.PlaybackStatus(MediaPlaybackStatus::Playing);
    } else if (state_str == "paused") {
      smtc_.PlaybackStatus(MediaPlaybackStatus::Paused);
    } else if (state_str == "stopped") {
      smtc_.PlaybackStatus(MediaPlaybackStatus::Stopped);
    } else if (state_str == "none") {
      smtc_.PlaybackStatus(MediaPlaybackStatus::Closed);
    }

    // Update timeline properties for seek bar
    SystemMediaTransportControlsTimelineProperties timeline;

    // Position in TimeSpan (100-nanosecond units)
    int64_t positionTicks = static_cast<int64_t>(position * 10000000.0);
    timeline.Position(winrt::Windows::Foundation::TimeSpan(positionTicks));

    timeline.MinSeekTime(winrt::Windows::Foundation::TimeSpan(0));

    // Get duration from map if available
    auto duration = GetDoubleFromMap(map, "duration");
    if (duration > 0) {
      int64_t durationTicks = static_cast<int64_t>(duration * 10000000.0);
      timeline.EndTime(winrt::Windows::Foundation::TimeSpan(durationTicks));
      timeline.MaxSeekTime(timeline.EndTime());
    }

    smtc_.UpdateTimelineProperties(timeline);

    // Set playback rate
    smtc_.PlaybackRate(speed);

  } catch (winrt::hresult_error const& ex) {
    // Ignore playback state update errors
  }
}

void OsMediaControlsPlugin::HandleButtonPressed(
    SystemMediaTransportControlsButton button) {

  flutter::EncodableMap event;

  switch (button) {
    case SystemMediaTransportControlsButton::Play:
      event[flutter::EncodableValue("type")] = flutter::EncodableValue("play");
      break;

    case SystemMediaTransportControlsButton::Pause:
      event[flutter::EncodableValue("type")] = flutter::EncodableValue("pause");
      break;

    case SystemMediaTransportControlsButton::Stop:
      event[flutter::EncodableValue("type")] = flutter::EncodableValue("stop");
      break;

    case SystemMediaTransportControlsButton::Next:
      event[flutter::EncodableValue("type")] = flutter::EncodableValue("next");
      break;

    case SystemMediaTransportControlsButton::Previous:
      event[flutter::EncodableValue("type")] = flutter::EncodableValue("previous");
      break;

    default:
      return; // Unknown button
  }

  SendEvent(event);
}

void OsMediaControlsPlugin::EnableControl(const std::string& control) {
  if (!smtc_) return;

  try {
    if (control == "play") {
      smtc_.IsPlayEnabled(true);
    } else if (control == "pause") {
      smtc_.IsPauseEnabled(true);
    } else if (control == "stop") {
      smtc_.IsStopEnabled(true);
    } else if (control == "next") {
      smtc_.IsNextEnabled(true);
    } else if (control == "previous") {
      smtc_.IsPreviousEnabled(true);
    }
    // Note: Windows doesn't have separate seek control enable
  } catch (winrt::hresult_error const& ex) {
    // Ignore enable errors
  }
}

void OsMediaControlsPlugin::DisableControl(const std::string& control) {
  if (!smtc_) return;

  try {
    if (control == "play") {
      smtc_.IsPlayEnabled(false);
    } else if (control == "pause") {
      smtc_.IsPauseEnabled(false);
    } else if (control == "stop") {
      smtc_.IsStopEnabled(false);
    } else if (control == "next") {
      smtc_.IsNextEnabled(false);
    } else if (control == "previous") {
      smtc_.IsPreviousEnabled(false);
    }
  } catch (winrt::hresult_error const& ex) {
    // Ignore disable errors
  }
}

winrt::hstring OsMediaControlsPlugin::StringToHString(const std::string& str) {
  if (str.empty()) return winrt::hstring();

  // Convert UTF-8 std::string to wide string
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring wide = converter.from_bytes(str);

  return winrt::hstring(wide);
}

RandomAccessStreamReference OsMediaControlsPlugin::CreateStreamReferenceFromBytes(
    const std::vector<uint8_t>& bytes) {

  if (bytes.empty()) {
    return nullptr;
  }

  try {
    // Create in-memory stream
    InMemoryRandomAccessStream stream;
    DataWriter writer(stream);

    // Write bytes to stream
    writer.WriteBytes(winrt::array_view<const uint8_t>(
        bytes.data(),
        bytes.data() + bytes.size()
    ));

    // Store the data (synchronous call - use .get() to wait)
    writer.StoreAsync().get();
    writer.DetachStream();

    // Seek to beginning
    stream.Seek(0);

    // Create stream reference
    return RandomAccessStreamReference::CreateFromStream(stream);

  } catch (winrt::hresult_error const& ex) {
    return nullptr;
  }
}

void OsMediaControlsPlugin::SendEvent(const flutter::EncodableMap &event) {
  if (event_sink_) {
    event_sink_->Success(flutter::EncodableValue(event));
  }
}

std::string OsMediaControlsPlugin::GetStringFromMap(
    const flutter::EncodableMap &map, const std::string &key) {
  auto it = map.find(flutter::EncodableValue(key));
  if (it != map.end() && std::holds_alternative<std::string>(it->second)) {
    return std::get<std::string>(it->second);
  }
  return "";
}

double OsMediaControlsPlugin::GetDoubleFromMap(
    const flutter::EncodableMap &map, const std::string &key) {
  auto it = map.find(flutter::EncodableValue(key));
  if (it != map.end() && std::holds_alternative<double>(it->second)) {
    return std::get<double>(it->second);
  }
  return 0.0;
}

}  // namespace os_media_controls
