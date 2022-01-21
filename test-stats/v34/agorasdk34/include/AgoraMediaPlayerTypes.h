//
//  Agora Engine SDK
//
//  Created by Sting Feng in 2020-05.
//  Copyright (c) 2017 Agora.io. All rights reserved.

#pragma once  // NOLINT(build/header_guard)

#include <cstring>
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
#include <cstdint>
#endif

/// @cond not-for-mpk-doc
/**
 * set analyze duration for real time stream
 * @example "setPlayerOption(KEY_PLAYER_REAL_TIME_STREAM_ANALYZE_DURATION,1000000)"
 */
#define KEY_PLAYER_REAL_TIME_STREAM_ANALYZE_DURATION    "analyze_duration"

/**
 * make the player to enable audio or not
 * @example  "setPlayerOption(KEY_PLAYER_ENABLE_AUDIO,0)"
 */
#define KEY_PLAYER_ENABLE_AUDIO                  "enable_audio"

/**
 * make the player to enable video or not
 * @example  "setPlayerOption(KEY_PLAYER_ENABLE_VIDEO,0)"
 */
#define KEY_PLAYER_ENABLE_VIDEO                  "enable_video"

/**
 * set the player enable to search metadata
 * @example  "setPlayerOption(KEY_PLAYER_DISABLE_SEARCH_METADATA,0)"
 */
#define KEY_PLAYER_ENABLE_SEARCH_METADATA         "enable_search_metadata"

/**
 * set the player sei filter type
 * @example  "setPlayerOption(KEY_PLAYER_SEI_FILTER_TYPE,"5")"
 */
#define KEY_PLAYER_SEI_FILTER_TYPE         "set_sei_filter_type"
/// @endcond

namespace agora {

namespace media {

namespace base {
static const uint8_t kMaxCharBufferLength = 50;
/**
 * The playback state.
 */
enum MEDIA_PLAYER_STATE {
  /** `0`: Default state. MediaPlayer Kit returns this state code before you open the media
   * resource or after you stop the playback. /
   */
  PLAYER_STATE_IDLE = 0,
  /** Opening the media resource. */
  PLAYER_STATE_OPENING,
  /** Opened the media resource successfully. */
  PLAYER_STATE_OPEN_COMPLETED,
  /** Playing the media resource. */
  PLAYER_STATE_PLAYING,
  /** Pauses the playback. */
  PLAYER_STATE_PAUSED,
  /** The playback is completed. */
  PLAYER_STATE_PLAYBACK_COMPLETED,
  /** The loop playback is completed. */
  PLAYER_STATE_PLAYBACK_ALL_LOOPS_COMPLETED,
  /** The playback is stopped.
   */
  PLAYER_STATE_STOPPED,
  /// @cond not-for-mpk-doc
  /** Player pausing (internal)
   */
  PLAYER_STATE_PAUSING_INTERNAL = 50,
  /** Player stopping (internal)
   */
  PLAYER_STATE_STOPPING_INTERNAL,
  /** Player seeking state (internal)
   */
  PLAYER_STATE_SEEKING_INTERNAL,
  /** Player getting state (internal)
   */
  PLAYER_STATE_GETTING_INTERNAL,
  /** None state for state machine (internal)
   */
  PLAYER_STATE_NONE_INTERNAL,
  /** Do nothing state for state machine (internal)
   */
  PLAYER_STATE_DO_NOTHING_INTERNAL,
  /// @endcond
  /** `100`: Fails to play the media resource. */
  PLAYER_STATE_FAILED = 100,
};
/**
 * The error code.
 */
enum MEDIA_PLAYER_ERROR {
  /** `0`: No error. */
  PLAYER_ERROR_NONE = 0,
  /** `-1`: Invalid arguments. */
  PLAYER_ERROR_INVALID_ARGUMENTS = -1,
  /** `-2`: Internal error. */
  PLAYER_ERROR_INTERNAL = -2,
  /** `-3`: No resource. */
  PLAYER_ERROR_NO_RESOURCE = -3,
  /** `-4`: Invalid media resource. */
  PLAYER_ERROR_INVALID_MEDIA_SOURCE = -4,
  /** `-5`: The type of the media stream is unknown. */
  PLAYER_ERROR_UNKNOWN_STREAM_TYPE = -5,
  /** `-6`: The object is not initialized. */
  PLAYER_ERROR_OBJ_NOT_INITIALIZED = -6,
  /** `-7`: The codec is not supported. */
  PLAYER_ERROR_CODEC_NOT_SUPPORTED = -7,
  /** `-8`: Invalid renderer. */
  PLAYER_ERROR_VIDEO_RENDER_FAILED = -8,
  /** `-9`: Error occurs in the internal state of the player. */
  PLAYER_ERROR_INVALID_STATE = -9,
  /** `-10`: The URL of the media resource can not be found. */
  PLAYER_ERROR_URL_NOT_FOUND = -10,
  /** `-11`: Invalid connection between the player and Agora's Server. */
  PLAYER_ERROR_INVALID_CONNECTION_STATE = -11,
  /** `-12`: The playback buffer is insufficient. */
  PLAYER_ERROR_SRC_BUFFER_UNDERFLOW = -12,
};

/**
 * Report the playback speed.
 */
enum MEDIA_PLAYER_PLAYBACK_SPEED {
  /** `100`: The original playback speed.
   */
  PLAYBACK_SPEED_ORIGINAL = 100,
  /** `50`: The playback speed is 0.50 times the original speed.
   */
  PLAYBACK_SPEED_50_PERCENT = 50,
  /** `75`: The playback speed is 0.75 times the original speed.
   */
  PLAYBACK_SPEED_75_PERCENT = 75,
  /** `125`: The playback speed is 1.25 times the original speed.
   */
  PLAYBACK_SPEED_125_PERCENT = 125,
  /** `150`: The playback speed is 1.50 times the original speed.
   */
  PLAYBACK_SPEED_150_PERCENT = 150,
  /** `200`: The playback speed is 2.00 times the original speed.
   */
  PLAYBACK_SPEED_200_PERCENT = 200,
};

/**
 * The type of the media stream.
 */
enum MEDIA_STREAM_TYPE {
  /** `0`: The type is unknown. */
  STREAM_TYPE_UNKNOWN = 0,
  /** `1`: The video stream. */
  STREAM_TYPE_VIDEO = 1,
  /** `2`: The audio stream. */
  STREAM_TYPE_AUDIO = 2,
  /** `3`: The subtitle stream.  */
  STREAM_TYPE_SUBTITLE = 3,
};

/**
 * The playback event.
 */
enum MEDIA_PLAYER_EVENT {
  /** `0`: Begins to seek to the new playback position. */
  PLAYER_EVENT_SEEK_BEGIN = 0,
  /** `1`: Finishes seeking to the new playback position. */
  PLAYER_EVENT_SEEK_COMPLETE = 1,
  /** `2`: An error occurs when seeking to the new playback position. */
  PLAYER_EVENT_SEEK_ERROR = 2,
  /** `3`: The player publishes a video track.
   */
  PLAYER_EVENT_VIDEO_PUBLISHED = 3,
  /** `4`: The player publishes an audio track.
   */
  PLAYER_EVENT_AUDIO_PUBLISHED = 4,
  /** `5`: The audio track used by the player has been changed. */
  PLAYER_EVENT_AUDIO_TRACK_CHANGED = 5,
  /** player buffer low
   */
  PLAYER_EVENT_BUFFER_LOW = 6,
  /** player buffer recover
   */
  PLAYER_EVENT_BUFFER_RECOVER = 7,
};

/** The whole detailed information of the media stream. */
struct PlayerStreamInfo {
  /** The index of the media stream. */
  int streamIndex;

  /** The type of the media stream. See #MEDIA_STREAM_TYPE. */
  MEDIA_STREAM_TYPE streamType;

  /** The codec of the media stream. */
  char codecName[kMaxCharBufferLength];

  /** The language of the media stream. */
  char language[kMaxCharBufferLength];

  /** The frame rate (fps) if the stream is video. */
  int videoFrameRate;

  /** The video bitrate (bps) if the stream is video. */
  int videoBitRate;

  /** The video width (pixel) if the stream is video. */
  int videoWidth;

  /** The video height (pixel) if the stream is video. */
  int videoHeight;

  /** The rotation angle if the steam is video. */
  int videoRotation;

  /** The sample rate (Hz) if the stream is audio. */
  int audioSampleRate;

  /** The number of audio channels if the stream is audio. */
  int audioChannels;

  /** The number of bits per sample if the stream is audio. */
  int audioBitsPerSample;

  /** The total duration (second) of the media stream. */
  int64_t duration;

  PlayerStreamInfo() : streamIndex(0),
                       streamType(STREAM_TYPE_UNKNOWN),
                       videoFrameRate(0),
                       videoBitRate(0),
                       videoWidth(0),
                       videoHeight(0),
                       videoRotation(0),
                       audioSampleRate(0),
                       audioChannels(0),
                       audioBitsPerSample(0),
                       duration(0) {
    memset(codecName, 0, sizeof(codecName));
    memset(language, 0, sizeof(language));
  }
};

/**
 * The type of the media metadata.
 */
enum MEDIA_PLAYER_METADATA_TYPE {
  /** `0`: The type is unknown. */
  PLAYER_METADATA_TYPE_UNKNOWN = 0,
  /** `1`: The type is SEI. */
  PLAYER_METADATA_TYPE_SEI = 1,
};

}  // namespace base
}  // namespace media
}  // namespace agora
