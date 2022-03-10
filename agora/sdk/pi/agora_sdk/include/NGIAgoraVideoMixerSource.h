//
//  Agora SDK
//
//  Copyright (c) 2018 Agora.io. All rights reserved.
//

#pragma once  // NOLINT(build/header_guard)

#include "AgoraBase.h"
#include "AgoraRefPtr.h"
#include "IAgoraService.h"

namespace agora {
namespace rtc {

class IVideoTrack;

struct MixerLayoutConfig {
  uint32_t top;
  uint32_t left;
  uint32_t width;
  uint32_t height;
  int32_t zOrder;
  float alpha;
};

/**
 * The IVideoMixerSource class abstracts a multi-in-multi-out video source which receives video
 * streams from multiple local or remote video tracks and generate mixed video stream in user defined output
 * format. When only one video track is added to the mixer, it simply forwards the incoming video frames
 * to its sinks.
 */
class IVideoMixerSource : public RefCountInterface {
public:
  /**
   * Adds a video track for mixing.
   *
   * @param track The instance of the video track that you want mixer to receive its video stream.
   */
  virtual void addVideoTrack(agora_refptr<IVideoTrack> track) = 0;
  /**
   * Removes the video track.
   * @param track The instance of the video track that you want to remove from the mixer.
   */
  virtual void removeVideoTrack(agora_refptr<IVideoTrack> track) = 0;
  /**
   * Configures the layout of video frames comming from a specific track (indicated by uid)
   * on the mixer canvas.
   *
   * @param uid The instance of the video track that you want to remove from the mixer.
   * @param config The layout configuration
   */
  virtual void setStreamLayout(user_id_t uid, const MixerLayoutConfig& config) = 0;
  /**
   * Add an image source to the mixer with its layout configuration on the mixer canvas.
   *
   * @param url The URL of the image source.
   * @param config The layout configuration.
   */
  virtual void setImageSource(const char* url, const MixerLayoutConfig& config) = 0;
  /**
   * Removes the user layout on the mixer canvas.
   *
   * @param uid The uid of the stream.
   */
  virtual void delStreamLayout(user_id_t uid) = 0;
  /**
   * Refreshes the user layout on the mixer canvas.
   *
   * @param uid The uid of the stream.
   */
  virtual void refresh(user_id_t uid) = 0;
  /**
   * Sets the mixer canvas background to override the default configuration.
   *
   * @param width Width of the canvas.
   * @param height Height of the canvas.
   * @param fps Frame rate (fps) of the mixed video stream.
   * @param color_argb Mixer canvas background color in ARGB format.
   */
  virtual void setBackground(uint32_t width, uint32_t height, int fps, uint32_t color_argb) = 0;
  /**
   * Sets the mixer canvas background to override the default configuration.
   *
   * @param width Width of the canvas.
   * @param height Height of the canvas.
   * @param fps Frame rate (fps) of the mixed video stream.
   * @param url URL of the canvas background image.
   */
  virtual void setBackground(uint32_t width, uint32_t height, int fps, const char* url) = 0;
  /**
   * Sets the rotation of the mixed video stream.
   *
   * @param rotation Specify the following values:
   * - 0:No rotation
   * - 1:90°
   * - 2:180°
   * - 3:270°
   */
  virtual void setRotation(uint8_t rotation) = 0;
};

} //namespace rtc
} // namespace agora
