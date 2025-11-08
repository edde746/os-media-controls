import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:os_media_controls/os_media_controls.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'OS Media Controls Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MediaPlayerPage(),
    );
  }
}

class MediaPlayerPage extends StatefulWidget {
  const MediaPlayerPage({super.key});

  @override
  State<MediaPlayerPage> createState() => _MediaPlayerPageState();
}

class _MediaPlayerPageState extends State<MediaPlayerPage> {
  // Mock player state
  bool _isPlaying = false;
  Duration _position = Duration.zero;
  Duration _duration = const Duration(minutes: 3, seconds: 45);
  double _playbackSpeed = 1.0;
  int _currentTrackIndex = 0;

  Timer? _positionTimer;
  final List<String> _eventLog = [];

  final List<Map<String, String>> _playlist = [
    {
      'title': 'Bohemian Rhapsody',
      'artist': 'Queen',
      'album': 'A Night at the Opera',
    },
    {
      'title': 'Stairway to Heaven',
      'artist': 'Led Zeppelin',
      'album': 'Led Zeppelin IV',
    },
    {
      'title': 'Hotel California',
      'artist': 'Eagles',
      'album': 'Hotel California',
    },
  ];

  @override
  void initState() {
    super.initState();
    _setupMediaControls();
    _listenToControlEvents();
    _updateMetadata();
  }

  @override
  void dispose() {
    _positionTimer?.cancel();
    super.dispose();
  }

  void _setupMediaControls() async {
    // Enable all controls
    await OsMediaControls.enableControls([
      MediaControl.play,
      MediaControl.pause,
      MediaControl.next,
      MediaControl.previous,
      MediaControl.seek,
      MediaControl.changeSpeed,
    ]);

    // Set skip intervals for podcasts (15 seconds)
    await OsMediaControls.setSkipIntervals(
      forward: const Duration(seconds: 15),
      backward: const Duration(seconds: 15),
    );

    // Set queue information
    await OsMediaControls.setQueueInfo(
      currentIndex: _currentTrackIndex,
      queueLength: _playlist.length,
    );
  }

  void _listenToControlEvents() {
    OsMediaControls.controlEvents.listen((event) {
      setState(() {
        _eventLog.insert(0, '${DateTime.now().toIso8601String().substring(11, 19)}: $event');
        if (_eventLog.length > 10) {
          _eventLog.removeLast();
        }
      });

      if (event is PlayEvent) {
        _play();
      } else if (event is PauseEvent) {
        _pause();
      } else if (event is NextTrackEvent) {
        _nextTrack();
      } else if (event is PreviousTrackEvent) {
        _previousTrack();
      } else if (event is SeekEvent) {
        _seek(event.position);
      } else if (event is SkipForwardEvent) {
        _skipForward(event.interval ?? const Duration(seconds: 15));
      } else if (event is SkipBackwardEvent) {
        _skipBackward(event.interval ?? const Duration(seconds: 15));
      } else if (event is SetSpeedEvent) {
        _setSpeed(event.speed);
      }
    });
  }

  void _updateMetadata() async {
    final track = _playlist[_currentTrackIndex];

    await OsMediaControls.setMetadata(MediaMetadata(
      title: track['title']!,
      artist: track['artist']!,
      album: track['album']!,
      duration: _duration,
      // In a real app, you would load actual artwork here
      artwork: _generatePlaceholderArtwork(),
    ));

    await OsMediaControls.setQueueInfo(
      currentIndex: _currentTrackIndex,
      queueLength: _playlist.length,
    );
  }

  void _updatePlaybackState() async {
    await OsMediaControls.setPlaybackState(MediaPlaybackState(
      state: _isPlaying ? PlaybackState.playing : PlaybackState.paused,
      position: _position,
      speed: _playbackSpeed,
    ));
  }

  void _play() {
    setState(() {
      _isPlaying = true;
    });
    _updatePlaybackState();
    _startPositionTimer();
  }

  void _pause() {
    setState(() {
      _isPlaying = false;
    });
    _updatePlaybackState();
    _stopPositionTimer();
  }

  void _nextTrack() {
    setState(() {
      _currentTrackIndex = (_currentTrackIndex + 1) % _playlist.length;
      _position = Duration.zero;
    });
    _updateMetadata();
    _updatePlaybackState();
  }

  void _previousTrack() {
    setState(() {
      if (_position.inSeconds > 3) {
        _position = Duration.zero;
      } else {
        _currentTrackIndex = (_currentTrackIndex - 1) % _playlist.length;
        _position = Duration.zero;
      }
    });
    _updateMetadata();
    _updatePlaybackState();
  }

  void _seek(Duration position) {
    setState(() {
      if (position < Duration.zero) {
        _position = Duration.zero;
      } else if (position > _duration) {
        _position = _duration;
      } else {
        _position = position;
      }
    });
    _updatePlaybackState();
  }

  void _skipForward(Duration interval) {
    _seek(_position + interval);
  }

  void _skipBackward(Duration interval) {
    _seek(_position - interval);
  }

  void _setSpeed(double speed) {
    setState(() {
      _playbackSpeed = speed;
    });
    _updatePlaybackState();
  }

  void _startPositionTimer() {
    _stopPositionTimer();
    _positionTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (_isPlaying && _position < _duration) {
        setState(() {
          _position = Duration(
            milliseconds: (_position.inMilliseconds +
                    (1000 * _playbackSpeed).round())
                .clamp(0, _duration.inMilliseconds),
          );
        });
        _updatePlaybackState();
      } else if (_position >= _duration) {
        _pause();
      }
    });
  }

  void _stopPositionTimer() {
    _positionTimer?.cancel();
    _positionTimer = null;
  }

  Uint8List _generatePlaceholderArtwork() {
    // Generate a simple 1x1 pixel PNG (transparent)
    // In a real app, you would load actual images
    return Uint8List.fromList([
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // PNG signature
      0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, // IHDR chunk
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, // 1x1 dimensions
      0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
      0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
      0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
      0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
      0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
      0x42, 0x60, 0x82,
    ]);
  }

  String _formatDuration(Duration duration) {
    String twoDigits(int n) => n.toString().padLeft(2, '0');
    final minutes = twoDigits(duration.inMinutes.remainder(60));
    final seconds = twoDigits(duration.inSeconds.remainder(60));
    return '$minutes:$seconds';
  }

  @override
  Widget build(BuildContext context) {
    final track = _playlist[_currentTrackIndex];

    return Scaffold(
      appBar: AppBar(
        title: const Text('OS Media Controls Demo'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Column(
        children: [
          Expanded(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  // Album artwork placeholder
                  Container(
                    width: 250,
                    height: 250,
                    decoration: BoxDecoration(
                      color: Colors.grey[300],
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Icon(
                      Icons.music_note,
                      size: 120,
                      color: Colors.grey[600],
                    ),
                  ),
                  const SizedBox(height: 32),

                  // Track info
                  Text(
                    track['title']!,
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                          fontWeight: FontWeight.bold,
                        ),
                    textAlign: TextAlign.center,
                  ),
                  const SizedBox(height: 8),
                  Text(
                    track['artist']!,
                    style: Theme.of(context).textTheme.titleMedium,
                    textAlign: TextAlign.center,
                  ),
                  Text(
                    track['album']!,
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                          color: Colors.grey[600],
                        ),
                    textAlign: TextAlign.center,
                  ),
                  const SizedBox(height: 32),

                  // Progress bar
                  Slider(
                    value: _position.inMilliseconds.toDouble(),
                    max: _duration.inMilliseconds.toDouble(),
                    onChanged: (value) {
                      _seek(Duration(milliseconds: value.toInt()));
                    },
                  ),
                  Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 16),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(_formatDuration(_position)),
                        Text(_formatDuration(_duration)),
                      ],
                    ),
                  ),
                  const SizedBox(height: 16),

                  // Playback controls
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      IconButton(
                        icon: const Icon(Icons.skip_previous),
                        iconSize: 40,
                        onPressed: _previousTrack,
                      ),
                      const SizedBox(width: 8),
                      IconButton(
                        icon: Icon(Icons.replay_10),
                        iconSize: 32,
                        onPressed: () => _skipBackward(const Duration(seconds: 15)),
                      ),
                      const SizedBox(width: 8),
                      IconButton(
                        icon: Icon(
                          _isPlaying ? Icons.pause_circle : Icons.play_circle,
                        ),
                        iconSize: 64,
                        onPressed: _isPlaying ? _pause : _play,
                      ),
                      const SizedBox(width: 8),
                      IconButton(
                        icon: Icon(Icons.forward_10),
                        iconSize: 32,
                        onPressed: () => _skipForward(const Duration(seconds: 15)),
                      ),
                      const SizedBox(width: 8),
                      IconButton(
                        icon: const Icon(Icons.skip_next),
                        iconSize: 40,
                        onPressed: _nextTrack,
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),

                  // Playback speed
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const Text('Speed: '),
                      DropdownButton<double>(
                        value: _playbackSpeed,
                        items: [0.5, 1.0, 1.5, 2.0].map((speed) {
                          return DropdownMenuItem(
                            value: speed,
                            child: Text('${speed}x'),
                          );
                        }).toList(),
                        onChanged: (value) {
                          if (value != null) {
                            _setSpeed(value);
                          }
                        },
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),

                  // Event log
                  Card(
                    child: Padding(
                      padding: const EdgeInsets.all(16),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'Control Events',
                            style: Theme.of(context).textTheme.titleMedium,
                          ),
                          const SizedBox(height: 8),
                          ..._eventLog.map((event) => Text(
                                event,
                                style: const TextStyle(
                                  fontFamily: 'monospace',
                                  fontSize: 12,
                                ),
                              )),
                          if (_eventLog.isEmpty)
                            const Text(
                              'No events yet. Try using system media controls!',
                              style: TextStyle(
                                fontStyle: FontStyle.italic,
                                color: Colors.grey,
                              ),
                            ),
                        ],
                      ),
                    ),
                  ),
                  const SizedBox(height: 16),

                  // Instructions
                  Card(
                    color: Colors.blue[50],
                    child: Padding(
                      padding: const EdgeInsets.all(16),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'How to test:',
                            style: Theme.of(context).textTheme.titleMedium,
                          ),
                          const SizedBox(height: 8),
                          const Text('• iOS/macOS: Use Control Center or lock screen'),
                          const Text('• Android: Use notification or quick settings'),
                          const Text('• Windows: Use media keys on keyboard'),
                          const Text('• Watch the event log below for activity!'),
                        ],
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}
