//
//  Agora Engine SDK
//
//  Created by Sting Feng in 2017-11.
//  Copyright (c) 2017 Agora.io. All rights reserved.

#pragma once  // NOLINT(build/header_guard)

#include <cstring>
#include <stdint.h>
#include <limits>
#include <stddef.h>

#ifndef OPTIONAL_ENUM_SIZE_T
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
#define OPTIONAL_ENUM_SIZE_T enum : size_t
#else
#define OPTIONAL_ENUM_SIZE_T enum
#endif
#endif

namespace agora {
namespace rtc {

typedef unsigned int uid_t;
typedef unsigned int track_id_t;
typedef unsigned int conn_id_t;

static const unsigned int DEFAULT_CONNECTION_ID = 0;
static const unsigned int DUMMY_CONNECTION_ID = (std::numeric_limits<unsigned int>::max)();

/**
 * Audio routes.
 */
enum AudioRoute
{
  /**
   * -1: The default audio route.
   */
  ROUTE_DEFAULT = -1,
  /**
   * The headset.
   */
  ROUTE_HEADSET,
  /**
   * The earpiece.
   */
  ROUTE_EARPIECE,
  /**
   * The headset with no microphone.
   */
  ROUTE_HEADSETNOMIC,
  /**
   * The speakerphone.
   */
  ROUTE_SPEAKERPHONE,
  /**
   * The loudspeaker.
   */
  ROUTE_LOUDSPEAKER,
  /**
   * The Bluetooth headset.
   */
  ROUTE_HEADSETBLUETOOTH
};

/**
 * Bytes per sample
 */
enum BYTES_PER_SAMPLE {
  /**
   * two bytes per sample
   */
  TWO_BYTES_PER_SAMPLE = 2,
};

struct AudioParameters {
  int sample_rate;
  size_t channels;
  size_t frames_per_buffer;

  AudioParameters()
      : sample_rate(0),
        channels(0),
        frames_per_buffer(0) {}
};

enum RAW_AUDIO_FRAME_OP_MODE_TYPE {
  /** 0: Read-only mode: Users only read the
     agora::media::IAudioFrameObserver::AudioFrame data without modifying
     anything. For example, when users acquire data with the Agora SDK then push
     the RTMP streams. */
  RAW_AUDIO_FRAME_OP_MODE_READ_ONLY = 0,

  /** 2: Read and write mode: Users read the data from AudioFrame, modify it,
     and then play it. For example, when users have their own sound-effect
     processing module and do some voice pre-processing such as a voice change.
   */
  RAW_AUDIO_FRAME_OP_MODE_READ_WRITE = 2,
};

}  // namespace rtc

namespace media {
namespace base {

typedef void* view_t;

typedef const char* user_id_t;
/** The max length (bytes). */
static const uint8_t kMaxCodecNameLength = 50;


/**
      * The maximum metadata size.
      */
    enum MAX_METADATA_SIZE_TYPE
    {
        MAX_METADATA_SIZE_IN_BYTE = 1024
    };



/**
 * The definition of the PacketOptions struct, which contains infomation of the packet
 * in the RTP (Real-time Transport Protocal) header.
 */
struct PacketOptions {
  /**
   * The timestamp of the packet.
   */
  uint32_t timestamp;
  // Audio level indication.
  uint8_t audioLevelIndication;

  PacketOptions()
      : timestamp(0),
        audioLevelIndication(127) {}
};

/**
 * The detailed information of the incoming audio frame in the PCM format.
 */
struct AudioPcmFrame {
  /**
   * The buffer size of the PCM audio frame.
   */
  OPTIONAL_ENUM_SIZE_T {
    // Stereo, 32 kHz, 60 ms (2 * 32 * 60)
    /**
     * The max number of the samples of the data.
     *
     * When the number of audio channel is two, the sample rate is 32 kHZ,
     * the buffer length of the data is 60 ms, the number of the samples of the data is 3840 (2 x 32 x 60).
     */
    kMaxDataSizeSamples = 3840,
    /** The max number of the bytes of the data. */
    kMaxDataSizeBytes = kMaxDataSizeSamples * sizeof(int16_t),
  };

  /** The timestamp (ms) of the audio frame.
   */
  uint32_t capture_timestamp;
  /** The number of samples per channel.
   */
  size_t samples_per_channel_;
  /** The sample rate (Hz) of the audio data.
   */
  int sample_rate_hz_;
  /** The channel number.
   */
  size_t num_channels_;
  /** The number of bytes per sample.
   */
  rtc::BYTES_PER_SAMPLE bytes_per_sample;
  /** The max number of the bytes of the data. */
  int16_t data_[kMaxDataSizeSamples];

  AudioPcmFrame() :
    capture_timestamp(0),
    samples_per_channel_(0),
    sample_rate_hz_(0),
    num_channels_(0),
    bytes_per_sample(rtc::TWO_BYTES_PER_SAMPLE) {
    memset(data_, 0, sizeof(data_));
  }
};
/** The IAudioFrameObserver class.
 */
class IAudioFrameObserver {
 public:
  /** Occurs when each time the player receives an audio frame.
   *
   * After registering the audio frame observer, the callback occurs when each
   * time the player receives an audio frame, reporting the detailed
   * information of the audio frame.
   * @param frame The detailed information of the audio frame. See
   * AudioPcmFrame
   */
  virtual void onFrame(AudioPcmFrame* frame) = 0;
  virtual ~IAudioFrameObserver() {}
};

/** The pixel format of the video frame.
 */
enum VIDEO_PIXEL_FORMAT {
  /**
   * 0: The format is known.
   */
  VIDEO_PIXEL_UNKNOWN = 0,
  /**
   * 1: I420.
   */
  VIDEO_PIXEL_I420 = 1,
  /**
   * 2: BGRA.
   */
  VIDEO_PIXEL_BGRA = 2,
  /**
   * 3: NV21.
   */
  VIDEO_PIXEL_NV21 = 3,
  /**
   * 4: RGBA.
   */
  VIDEO_PIXEL_RGBA = 4,
  /**
   * 8: NV12.
   */
  VIDEO_PIXEL_NV12 = 8,
  /**
   * 10: GL_TEXTURE_2D
   */
  VIDEO_TEXTURE_2D = 10,
  /**
   * 11: GL_TEXTURE_OES
   */
  VIDEO_TEXTURE_OES = 11,
  /**
   * 16: I422.
   */
  VIDEO_PIXEL_I422 = 16,
};

/**
 * The video display mode.
 */
enum RENDER_MODE_TYPE {
  /**
   * 1: Uniformly scale the video until it fills the visible boundaries
   * (cropped). One dimension of the video may have clipped contents.
   */
  RENDER_MODE_HIDDEN = 1,
  /**
   * 2: Uniformly scale the video until one of its dimension fits the boundary
   * (zoomed to fit). Areas that are not filled due to the disparity in the
   * aspect ratio will be filled with black.
   */
  RENDER_MODE_FIT = 2,
  /**
   * @deprecated
   * 3: This mode is deprecated.
   */
  RENDER_MODE_ADAPTIVE = 3,
};

/**
 * The definition of the ExternalVideoFrame struct.
 */
struct ExternalVideoFrame {
  ExternalVideoFrame()
      : type(VIDEO_BUFFER_RAW_DATA),
        format(VIDEO_PIXEL_UNKNOWN),
        buffer(NULL),
        stride(0),
        height(0),
        cropLeft(0),
        cropTop(0),
        cropRight(0),
        cropBottom(0),
        rotation(0),
        timestamp(0),
        eglContext(NULL),
        eglType(EGL_CONTEXT10),
        textureId(0),
        metadata_buffer(NULL),
        metadata_size(0){}

   /**
   * The EGL context type.
   */
  enum EGL_CONTEXT_TYPE {
    /**
     * 0: When using the OpenGL interface (javax.microedition.khronos.egl.*) defined by Khronos
     */
    EGL_CONTEXT10 = 0,
    /**
     * 0: When using the OpenGL interface (android.opengl.*) defined by Android
     */
    EGL_CONTEXT14 = 1,
  };

  /**
   * Video buffer types.
   */
  enum VIDEO_BUFFER_TYPE {
    /**
     * 1: Raw data.
     */
    VIDEO_BUFFER_RAW_DATA = 1,
    /**
     * 2: The same as VIDEO_BUFFER_RAW_DATA.
     */
    VIDEO_BUFFER_ARRAY = 2,
    /**
     * 3: The video buffer in the format of texture.
     */
    VIDEO_BUFFER_TEXTURE = 3,
  };

  /**
   * The buffer type: #VIDEO_BUFFER_TYPE.
   */
  VIDEO_BUFFER_TYPE type;
  /**
   * The pixel format: #VIDEO_PIXEL_FORMAT
   */
  VIDEO_PIXEL_FORMAT format;
  /**
   * The video buffer.
   */
  void* buffer;
  /**
   * The line spacing of the incoming video frame (px). For
   * texture, it is the width of the texture.
   */
  int stride;
  /**
   * The height of the incoming video frame.
   */
  int height;
  /**
   * [Raw data related parameter] The number of pixels trimmed from the left. The default value is
   * 0.
   */
  int cropLeft;
  /**
   * [Raw data related parameter] The number of pixels trimmed from the top. The default value is
   * 0.
   */
  int cropTop;
  /**
   * [Raw data related parameter] The number of pixels trimmed from the right. The default value is
   * 0.
   */
  int cropRight;
  /**
   * [Raw data related parameter] The number of pixels trimmed from the bottom. The default value
   * is 0.
   */
  int cropBottom;
  /**
   * [Raw data related parameter] The clockwise rotation information of the video frame. You can set the
   * rotation angle as 0, 90, 180, or 270. The default value is 0.
   */
  int rotation;
  /**
   * The timestamp (ms) of the incoming video frame. An incorrect timestamp results in a frame loss or
   * unsynchronized audio and video.
   */
  long long timestamp;
  /**
   * [Texture-related parameter]
   * When using the OpenGL interface (javax.microedition.khronos.egl.*) defined by Khronos, set EGLContext to this field.
   * When using the OpenGL interface (android.opengl.*) defined by Android, set EGLContext to this field.
   */
  void *eglContext;
  /**
   * [Texture related parameter] Texture ID used by the video frame.
   */
  EGL_CONTEXT_TYPE eglType;
  /**
   * [Texture related parameter] Incoming 4 &times; 4 transformational matrix. The typical value is a unit matrix.
   */
  int textureId;
  /**
   * [Texture related parameter] The MetaData buffer.
   *  The default value is NULL
   */
  uint8_t* metadata_buffer;
  /**
   * [Texture related parameter] The MetaData size.
   *  The default value is 0
   */
  int metadata_size;
};

/**
 * The detailed information of the incoming video frame.
 */
struct VideoFrame {
  VideoFrame():
  type(VIDEO_PIXEL_UNKNOWN),
  width(0),
  height(0),
  yStride(0),
  uStride(0),
  vStride(0),
  yBuffer(NULL),
  uBuffer(NULL),
  vBuffer(NULL),
  rotation(0),
  renderTimeMs(0),
  avsync_type(0),
  metadata_buffer(NULL),
  metadata_size(0){}

  /**
   * The pixel format of the video frame. See #VIDEO_PIXEL_FORMAT.
   */
  VIDEO_PIXEL_FORMAT type;
  /**
   * The width (pixel) of the video.
   */
  int width;
  /**
   * The height (pixel) of the video.
   */
  int height;
  /**
   * The line stride of the Y buffer in the YUV data.
   */
  int yStride;
  /**
   * The line stride of the U buffer in the YUV data.
   */
  int uStride;
  /**
   * The line stride of the V buffer in the YUV data.
   */
  int vStride;
  /**
   * The Y buffer in the YUV data.
   */
  uint8_t* yBuffer;
  /**
   * The U buffer in the YUV data.
   */
  uint8_t* uBuffer;
  /**
   * The V buffer in the YUV data.
   */
  uint8_t* vBuffer;
  /** Sets the rotation angle of the video frame before rendering.
   *
   * You can set the angle as 0, 90, 180, or 270.
   */
  int rotation;
  /** The timestamp of the incoming video frame.
   *
   * @note
   * - You must set this parameter.
   * - The timestamp restores the order of the captured audio frame. You can
   * use it to synchronize the audio and video frames in the scenarios, which
   * are related to video and external video sources.
   */
  int64_t renderTimeMs;
  /**
   * The type of audio-video synchronization.
   */
  int avsync_type;
  /**
   * [Texture related parameter] The MetaData buffer.
   *  The default value is NULL
   */
  uint8_t* metadata_buffer;
  /**
   * [Texture related parameter] The MetaData size.
   *  The default value is 0
   */
  int metadata_size;
};
/** The IVideoFrameObserver class.
 */
class IVideoFrameObserver {
 public:
  /** Occurs when each time the player receives a video frame.
   *
   * After registering the video frame observer, the callback occurs when each
   * time the player receives a video frame, reporting the detailed
   * information of the video frame.
   * @param frame The detailed information of the video frame. See
   * VideoFrame
   */
  virtual void onFrame(const VideoFrame* frame) = 0;
  virtual ~IVideoFrameObserver() {}
  virtual bool isExternal() { return true; }
};
/**
 * The type of the media player.
 */
enum MEDIA_PLAYER_SOURCE_TYPE {
  /**
   * The real type of media player when use MEDIA_PLAYER_SOURCE_DEFAULT is decided by the
   * type of SDK package. It is full feature media player in full-featured SDK, or simple
   * media player in others.
   */
  MEDIA_PLAYER_SOURCE_DEFAULT,
  /**
   * Full featured media player is designed to support more codecs and media format, which
   * requires more package size than simple player. If you need this player enabled, you
   * might need to download a full-featured SDK.
   */
  MEDIA_PLAYER_SOURCE_FULL_FEATURED,
  /**
   * Simple media player with limit codec supported, which requires minimal package size
   * requirement and is enabled by default
   */
  MEDIA_PLAYER_SOURCE_SIMPLE,
};

enum VIDEO_MODULE_POSITION {
  POSITION_POST_CAPTURER = 1 << 0,
  POSITION_PRE_RENDERER = 1 << 1,
  POSITION_PRE_ENCODER = 1 << 2,
  POSITION_POST_FILTERS = 1 << 3,
};

}  // namespace base

/**
 * The IAudioFrameObserver class.
 */
class IAudioFrameObserver {
 public:
  /**
   * Audio frame types.
   */
  enum AUDIO_FRAME_TYPE {
    /**
     * 0: 16-bit PCM.
     */
    FRAME_TYPE_PCM16 = 0,
  };
  /**
   * The definition of the AudioFrame struct.
   */
  struct AudioFrame {
    /**
     * The audio frame type: #AUDIO_FRAME_TYPE.
     */
    AUDIO_FRAME_TYPE type;
    /**
     * The number of samples per channel in this frame.
     */
    int samplesPerChannel;
    /**
     * The number of bytes per sample.
     */
    agora::rtc::BYTES_PER_SAMPLE bytesPerSample;
    /**
     * The number of audio channels (data is interleaved, if stereo).
     */
    int channels;
    /**
     * The sample rate of the audio frame.
     */
    int samplesPerSec;
    /**
     * The pointer to the audio data buffer.
     */
    void* buffer;
    /**
     * The timestamp to render the audio data. Use this member for audio-video synchronization when
     * rendering the audio.
     *
     * @note
     * This parameter is the timestamp for audio rendering. Set it as 0.
     */
    int64_t renderTimeMs;
    int avsync_type;

    AudioFrame() : type(FRAME_TYPE_PCM16),
                   samplesPerChannel(0),
                   bytesPerSample(rtc::TWO_BYTES_PER_SAMPLE),
                   channels(0),
                   samplesPerSec(0),
                   buffer(NULL),
                   renderTimeMs(0),
                   avsync_type(0) {}
  };

 public:
  virtual ~IAudioFrameObserver() {}
  /** Retrieves the local user's raw audio data.
   *
   * The SDK periodically triggers this callback according to the sample interval
   * set by the \ref rtc::IRtcEngine::setRecordingAudioFrameParameters "setRecordingAudioFrameParameters" method.
   * You can retrieve the captured audio data of the local user from this callback.
   *
   * @param audioFrame The audio frame: AudioFrame.
   *
   * @return
   * - true: The audio frame is valid and sent back to the SDK.
   * - false: The audio frame is invalid and discarded.
   */
  virtual bool onRecordAudioFrame(AudioFrame& audioFrame) = 0;
  /** Retrieves all the remote users' raw audio data.
   *
   * The SDK periodically triggers this callback according to the sample interval
   * set by the \ref rtc::IRtcEngine::setPlaybackAudioFrameParameters "setRecordingAudioFrameParameters" method.
   * You can retrieve the audio playback data of all the remote users from this callback.
   *
   * @param audioFrame The audio frame: AudioFrame.
   *
   * @return
   * - true: The audio frame is valid and sent back to the SDK.
   * - false: The audio frame is invalid and discarded.
   */
  virtual bool onPlaybackAudioFrame(AudioFrame& audioFrame) = 0;
  /** Retrieves the raw audio data of the local user and all the remote users.
   *
   * The SDK periodically triggers this callback according to the sample interval
   * set by the \ref rtc::IRtcEngine::setMixedAudioFrameParameters "setMixedAudioFrameParameters" method.
   * You can retrieve the mixed audio data of the local and remote users from this callback.
   *
   * @param audioFrame The audio frame: AudioFrame.
   *
   * @return
   * - true: The audio frame is valid and sent back to the SDK.
   * - false: The audio frame is invalid and discarded.
   */
  virtual bool onMixedAudioFrame(AudioFrame& audioFrame) = 0;
  /** Retrieves the raw audio data of a specific remote user.
   *
   * You can retrieve the audio playback data of a specific remote user from this callback.
   *
   * @param uid The ID of the remote user.
   * @param audioFrame The audio frame: AudioFrame.
   * @return
   * - true: The audio frame is valid and sent back to the SDK.
   * - false: The audio frame is invalid and discarded.
   */
  virtual bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioFrame& audioFrame) = 0;
};
/**
 * The IVideoFrameObserver class.
 */
class IVideoFrameObserver {
 public:
  typedef media::base::VideoFrame VideoFrame;

/** The process mode of the video frame.
 */
  enum VIDEO_FRAME_PROCESS_MODE {
    /** Only reads the video frame.
     *
     * The video observer works as a pure render, and you do not modify the original video frame.
     */
    PROCESS_MODE_READ_ONLY, // Observer works as a pure renderer and will not modify the original frame.
    /** Reads and writes the video frame.
     *
     * The video observer works as a video filter, and you will process the original video frame.
     */
    PROCESS_MODE_READ_WRITE, // Observer works as a filter that will process the video frame and affect the following frame processing in SDK.
  };
 public:
  virtual ~IVideoFrameObserver() {}
  /**
   * Occurs each time the SDK receives a video frame captured by the local camera.
   *
   * After you successfully register the video frame observer, the SDK triggers this callback each time
   * a video frame is received. In this callback, you can get the video data captured by the local
   * camera. You can then pre-process the data according to your scenarios.
   *
   * After pre-processing, you can send the processed video data back to the SDK by setting the
   * `videoFrame` parameter in this callback.
   *
   * @note
   * - This callback does not support sending processed RGBA video data back to the SDK.
   * - The video data that this callback gets has not been pre-processed, without the watermark, the cropped content,
   * the rotation, and the image enhancement.
   *
   * @param videoFrame A pointer to the video frame: VideoFrame
   * @return Determines whether to ignore the current video frame if the pre-processing fails:
   * - true: Do not ignore.
   * - false: Ignore, in which case this method does not sent the current video frame to the SDK.
   */
  virtual bool onCaptureVideoFrame(VideoFrame& videoFrame) = 0;

  virtual bool onSecondaryCameraCaptureVideoFrame(VideoFrame& videoFrame) = 0;

  /**
   * Occurs each time the SDK receives a video frame captured by the screen.
   *
   * After you successfully register the video frame observer, the SDK triggers this callback each time
   * a video frame is received. In this callback, you can get the video data for screen sharing.
   * You can then pre-process the data according to your scenarios.
   *
   * After pre-processing, you can send the processed video data back to the SDK by setting the
   * `videoFrame` parameter in this callback.
   *
   * @note
   * - This callback does not support sending processed RGBA video data back to the SDK.
   * - The video data that this callback gets has not been pre-processed, without the watermark, the cropped content,
   * the rotation, and the image enhancement.
   *
   * @param videoFrame A pointer to the video frame: VideoFrame
   * @return Determines whether to ignore the current video frame if the pre-processing fails:
   * - true: Do not ignore.
   * - false: Ignore, in which case this method does not sent the current video frame to the SDK.
   */
  virtual bool onScreenCaptureVideoFrame(VideoFrame& videoFrame) = 0;
  /**
   * Occurs each time the SDK receives a video frame decoded by the media player.
   *
   * After you successfully register the video frame observer, the SDK triggers this callback each
   * time a video frame is decoded. In this callback, you can get the video data decoded by the
   * media player. You can then pre-process the data according to your scenarios.
   *
   * After pre-processing, you can send the processed video data back to the SDK by setting the
   * `videoFrame` parameter in this callback.
   *
   * @param videoFrame A pointer to the video frame: VideoFrame
   * @param mediaPlayerId The ID of the media player.
   * @return Determines whether to ignore the current video frame if the pre-processing fails:
   * - true: Do not ignore.
   * - false: Ignore, in which case this method does not sent the current video frame to the SDK.
   */
  virtual bool onMediaPlayerVideoFrame(VideoFrame& videoFrame, int mediaPlayerId) = 0;

  virtual bool onSecondaryScreenCaptureVideoFrame(VideoFrame& videoFrame) = 0;

  /**
   * Occurs each time the SDK receives a video frame sent by the remote user.
   *
   * After you successfully register the video frame observer, the SDK triggers this callback each time a
   * video frame is received. In this callback, you can get the video data sent by the remote user. You
   * can then post-process the data according to your scenarios.
   *
   * After post-processing, you can send the processed data back to the SDK by setting the `videoFrame`
   * parameter in this callback.
   *
   * @param uid ID of the remote user who sends the current video frame.
   * @param connectionId ID of the connection.
   * @param videoFrame A pointer to the video frame: VideoFrame
   * @return Determines whether to ignore the current video frame if the post-processing fails:
   * - true: Do not ignore.
   * - false: Ignore, in which case this method does not sent the current video frame to the SDK.
   */
  virtual bool onRenderVideoFrame(rtc::uid_t uid, rtc::conn_id_t connectionId,
                                  VideoFrame& videoFrame) = 0;
  /// @cond
  virtual bool onTranscodedVideoFrame(VideoFrame& videoFrame) = 0;
  /// @endcond

  /** Occurs each time the SDK receives a video frame and prompts you to set the process mode of the video frame.
   *
   * After you successfully register the video frame observer, the SDK triggers this callback
   * each time it receives a video frame. You need to set your preferred process mode
   * in the return value of this callback.
   *
   * @return #VIDEO_FRAME_PROCESS_MODE
   * - #PROCESS_MODE_READ_ONLY (Default)
   * - #PROCESS_MODE_READ_WRITE
   */
  virtual VIDEO_FRAME_PROCESS_MODE getVideoFrameProcessMode() {
    return PROCESS_MODE_READ_ONLY;
  }

  /*
   * Occurs each time needs to get preference video frame type.
   *
   * @return preference video pixel format.
   */
  virtual base::VIDEO_PIXEL_FORMAT getVideoPixelFormatPreference() { return base::VIDEO_PIXEL_I420; }

  /**
   * Occurs each time needs to get rotation angle.
   *
   * @return rotation angle.
   */
  virtual int getRotationApplied() { return 0; }

  /**
   * Occurs each time needs to get whether mirror is applied or not.
   *
   * @return Determines whether to mirror.
   * - true: need to mirror.
   * - false: no mirror.
   */
  virtual bool getMirrorApplied() { return false; }

  /**
   * Indicate if the observer is for internal use.
   * Note: Never override this function
   * @return
   * - true: the observer is for external use
   * - false: the observer is for internal use
   */
  virtual bool isExternal() { return true; }
};

}  // namespace media
}  // namespace agora
