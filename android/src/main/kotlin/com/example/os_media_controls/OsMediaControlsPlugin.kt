package com.example.os_media_controls

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel

/** OsMediaControlsPlugin */
class OsMediaControlsPlugin: FlutterPlugin, MethodChannel.MethodCallHandler,
    EventChannel.StreamHandler {

    private lateinit var context: Context
    private lateinit var methodChannel: MethodChannel
    private lateinit var eventChannel: EventChannel
    private var eventSink: EventChannel.EventSink? = null

    private lateinit var mediaSession: MediaSessionCompat

    private var currentState: Int = PlaybackStateCompat.STATE_NONE
    private var currentPosition: Long = 0
    private var currentSpeed: Float = 1.0f

    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        context = binding.applicationContext

        methodChannel = MethodChannel(
            binding.binaryMessenger,
            "com.example.os_media_controls/methods"
        )
        methodChannel.setMethodCallHandler(this)

        eventChannel = EventChannel(
            binding.binaryMessenger,
            "com.example.os_media_controls/events"
        )
        eventChannel.setStreamHandler(this)

        setupMediaSession()
    }

    private fun setupMediaSession() {
        mediaSession = MediaSessionCompat(context, "OsMediaControls").apply {
            setCallback(object : MediaSessionCompat.Callback() {
                override fun onPlay() {
                    sendEvent(mapOf("type" to "play"))
                }

                override fun onPause() {
                    sendEvent(mapOf("type" to "pause"))
                }

                override fun onStop() {
                    sendEvent(mapOf("type" to "stop"))
                }

                override fun onSkipToNext() {
                    sendEvent(mapOf("type" to "next"))
                }

                override fun onSkipToPrevious() {
                    sendEvent(mapOf("type" to "previous"))
                }

                override fun onSeekTo(position: Long) {
                    sendEvent(mapOf(
                        "type" to "seek",
                        "position" to position / 1000.0 // Convert to seconds
                    ))
                }

                override fun onSetPlaybackSpeed(speed: Float) {
                    sendEvent(mapOf(
                        "type" to "setSpeed",
                        "speed" to speed.toDouble()
                    ))
                }

                override fun onFastForward() {
                    sendEvent(mapOf(
                        "type" to "skipForward",
                        "interval" to 15.0
                    ))
                }

                override fun onRewind() {
                    sendEvent(mapOf(
                        "type" to "skipBackward",
                        "interval" to 15.0
                    ))
                }
            })

            setFlags(
                MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS or
                MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS
            )

            isActive = true
        }
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "setMetadata" -> {
                setMetadata(call.arguments as? Map<String, Any>)
                result.success(null)
            }
            "setPlaybackState" -> {
                setPlaybackState(call.arguments as? Map<String, Any>)
                result.success(null)
            }
            "enableControls" -> {
                // Android enables controls via PlaybackState actions
                // This is handled in setPlaybackState
                result.success(null)
            }
            "disableControls" -> {
                // Android disables controls via PlaybackState actions
                // This would require tracking which controls to exclude
                result.success(null)
            }
            "setSkipIntervals" -> {
                // Android doesn't support custom skip intervals directly
                // Fast forward/rewind callbacks are used instead
                result.success(null)
            }
            "setQueueInfo" -> {
                // Android doesn't display queue info in the same way as iOS/macOS
                // Could be added to metadata as custom fields if needed
                result.success(null)
            }
            "clear" -> {
                clear()
                result.success(null)
            }
            else -> result.notImplemented()
        }
    }

    private fun setMetadata(arguments: Map<String, Any>?) {
        arguments ?: return

        val builder = MediaMetadataCompat.Builder()

        arguments["title"]?.let {
            builder.putString(MediaMetadataCompat.METADATA_KEY_TITLE, it as String)
        }
        arguments["artist"]?.let {
            builder.putString(MediaMetadataCompat.METADATA_KEY_ARTIST, it as String)
        }
        arguments["album"]?.let {
            builder.putString(MediaMetadataCompat.METADATA_KEY_ALBUM, it as String)
        }
        arguments["albumArtist"]?.let {
            builder.putString(MediaMetadataCompat.METADATA_KEY_ALBUM_ARTIST, it as String)
        }
        arguments["duration"]?.let {
            val durationSeconds = (it as Number).toDouble()
            builder.putLong(
                MediaMetadataCompat.METADATA_KEY_DURATION,
                (durationSeconds * 1000).toLong() // Convert to milliseconds
            )
        }
        arguments["artwork"]?.let {
            try {
                val bytes = it as ByteArray
                val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                if (bitmap != null) {
                    // Downsample if too large to avoid memory issues
                    val scaledBitmap = if (bitmap.width > 1024 || bitmap.height > 1024) {
                        Bitmap.createScaledBitmap(bitmap, 1024, 1024, true)
                    } else {
                        bitmap
                    }
                    builder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, scaledBitmap)
                    builder.putBitmap(MediaMetadataCompat.METADATA_KEY_ALBUM_ART, scaledBitmap)
                }
            } catch (e: Exception) {
                // Ignore artwork errors
            }
        }

        mediaSession.setMetadata(builder.build())
    }

    private fun setPlaybackState(arguments: Map<String, Any>?) {
        arguments ?: return

        val stateString = arguments["state"] as? String ?: "none"
        val positionSeconds = (arguments["position"] as? Number)?.toDouble() ?: 0.0
        val speed = (arguments["speed"] as? Number)?.toFloat() ?: 1.0f

        currentState = when (stateString) {
            "playing" -> PlaybackStateCompat.STATE_PLAYING
            "paused" -> PlaybackStateCompat.STATE_PAUSED
            "stopped" -> PlaybackStateCompat.STATE_STOPPED
            "buffering" -> PlaybackStateCompat.STATE_BUFFERING
            else -> PlaybackStateCompat.STATE_NONE
        }

        currentPosition = (positionSeconds * 1000).toLong() // Convert to milliseconds
        currentSpeed = speed

        val actions = PlaybackStateCompat.ACTION_PLAY or
                PlaybackStateCompat.ACTION_PAUSE or
                PlaybackStateCompat.ACTION_PLAY_PAUSE or
                PlaybackStateCompat.ACTION_STOP or
                PlaybackStateCompat.ACTION_SKIP_TO_NEXT or
                PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS or
                PlaybackStateCompat.ACTION_SEEK_TO or
                PlaybackStateCompat.ACTION_SET_PLAYBACK_SPEED or
                PlaybackStateCompat.ACTION_FAST_FORWARD or
                PlaybackStateCompat.ACTION_REWIND

        val playbackState = PlaybackStateCompat.Builder()
            .setState(currentState, currentPosition, currentSpeed)
            .setActions(actions)
            .build()

        mediaSession.setPlaybackState(playbackState)
    }

    private fun clear() {
        mediaSession.setMetadata(null)
        val playbackState = PlaybackStateCompat.Builder()
            .setState(PlaybackStateCompat.STATE_NONE, 0, 0.0f)
            .setActions(0)
            .build()
        mediaSession.setPlaybackState(playbackState)
    }

    private fun sendEvent(event: Map<String, Any>) {
        eventSink?.success(event)
    }

    // EventChannel.StreamHandler

    override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
        eventSink = events
    }

    override fun onCancel(arguments: Any?) {
        eventSink = null
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        methodChannel.setMethodCallHandler(null)
        eventChannel.setStreamHandler(null)
        mediaSession.release()
    }
}
