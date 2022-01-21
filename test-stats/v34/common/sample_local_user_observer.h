//  Agora RTC/MEDIA SDK
//
//  Created by Pengfei Han in 2020-03.
//  Copyright (c) 2020 Agora.io. All rights reserved.
//

#pragma once

#include <mutex>

#include "AgoraBase.h"
#include "NGIAgoraAudioTrack.h"
#include "NGIAgoraLocalUser.h"
#include "NGIAgoraMediaNodeFactory.h"
#include "NGIAgoraMediaNode.h"
#include "NGIAgoraVideoTrack.h"

class SampleLocalUserObserver : public agora::rtc::ILocalUserObserver {
 public:
  SampleLocalUserObserver(agora::rtc::ILocalUser* local_user);
  virtual ~SampleLocalUserObserver();

 public:
  agora::rtc::ILocalUser* GetLocalUser();
  void PublishAudioTrack(agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack);
  void PublishVideoTrack(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack);
  void UnpublishAudioTrack(agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack);
  void UnpublishVideoTrack(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack);
  agora::agora_refptr<agora::rtc::IRemoteAudioTrack> GetRemoteAudioTrack() {
    return remote_audio_track_;
  }
  agora::agora_refptr<agora::rtc::IRemoteVideoTrack> GetRemoteVideoTrack() {
    return remote_video_track_;
  }

  void setMediaPacketReceiver(agora::rtc::IMediaPacketReceiver* receiver) {
    std::lock_guard<std::mutex> _(observer_lock_);
    media_packet_receiver_ = receiver;
    if (remote_audio_track_)
      remote_audio_track_->registerMediaPacketReceiver(media_packet_receiver_);

    if (remote_video_track_)
      remote_video_track_->registerMediaPacketReceiver(media_packet_receiver_);
  }

  void setVideoEncodedImageReceiver(agora::rtc::IVideoEncodedImageReceiver* receiver) {
    video_encoded_receiver_ = receiver;
  }

  void setAudioFrameObserver(agora::media::IAudioFrameObserver* observer) {
    audio_frame_observer_ = observer;
  }

  void unsetAudioFrameObserver() {
    if (audio_frame_observer_) {
      local_user_->unregisterAudioFrameObserver(audio_frame_observer_);
    }
  }

  void setVideoFrameObserver(agora::agora_refptr<agora::rtc::IVideoSinkBase> observer) {
    video_frame_observer_ = observer;
  }

  void unsetVideoFrameObserver() {
    if (remote_video_track_ && video_frame_observer_) {
      remote_video_track_->removeRenderer(video_frame_observer_);
    }
  }

 public:
  // inherit from agora::rtc::ILocalUserObserver
  void onAudioTrackPublishSuccess(
      agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) override {}

  void onUserAudioTrackSubscribed(
      agora::user_id_t userId,
      agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack) override;

  void onAudioTrackPublicationFailure(agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack,
                                      agora::ERROR_CODE_TYPE error) override {}

  void onUserAudioTrackStateChanged(agora::user_id_t userId,
                                    agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack,
                                    agora::rtc::REMOTE_AUDIO_STATE state,
                                    agora::rtc::REMOTE_AUDIO_STATE_REASON reason,
                                    int elapsed) override;

  void onLocalAudioTrackStateChanged(agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack,
                                     agora::rtc::LOCAL_AUDIO_STREAM_STATE state,
                                     agora::rtc::LOCAL_AUDIO_STREAM_ERROR errorCode) override {}

  void onVideoTrackPublishSuccess(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) override {}

  void onVideoTrackPublicationFailure(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
                                      agora::ERROR_CODE_TYPE error) override {}

  void onUserVideoTrackSubscribed(
      agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
      agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack) override;

  void onUserVideoTrackStateChanged(agora::user_id_t userId,
                                    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
                                    agora::rtc::REMOTE_VIDEO_STATE state,
                                    agora::rtc::REMOTE_VIDEO_STATE_REASON reason,
                                    int elapsed) override {}

  void onRemoteVideoTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
                                    const agora::rtc::RemoteVideoTrackStats& stats);

  void onLocalVideoTrackStateChanged(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
                                     agora::rtc::LOCAL_VIDEO_STREAM_STATE state,
                                     agora::rtc::LOCAL_VIDEO_STREAM_ERROR errorCode) override {}

  void onLocalVideoTrackStatistics(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
                                   const agora::rtc::LocalVideoTrackStats& stats) override {}

  void onAudioVolumeIndication(const agora::rtc::AudioVolumeInfo* speakers,
                               unsigned int speakerNumber, int totalVolume) override {}

  void onLocalAudioTrackStatistics(const agora::rtc::LocalAudioStats& stats) override {}

  void onRemoteAudioTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack,
                                    const agora::rtc::RemoteAudioTrackStats& stats) override {}

  void onUserInfoUpdated(agora::user_id_t userId, USER_MEDIA_INFO msg, bool val) override;

  void onIntraRequestReceived() override;

  void onAudioSubscribeStateChanged(const char* channel, agora::rtc::uid_t uid,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE oldState,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE newState,
                                    int elapseSinceLastState) override {}

  void onVideoSubscribeStateChanged(const char* channel, agora::rtc::uid_t uid,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE oldState,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE newState,
                                    int elapseSinceLastState) override {}

  void onAudioPublishStateChanged(const char* channel, agora::rtc::STREAM_PUBLISH_STATE oldState,
                                  agora::rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override {}

  void onVideoPublishStateChanged(const char* channel, agora::rtc::STREAM_PUBLISH_STATE oldState,
                                  agora::rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override {}
  virtual void onStreamMessage(agora::user_id_t userId, int streamId, const char* data,
                       size_t length) override;

  void onRemoteVideoStreamInfoUpdated(const agora::rtc::RemoteVideoStreamInfo& info) override {}

 private:
  agora::rtc::ILocalUser* local_user_{nullptr};

  agora::agora_refptr<agora::rtc::IRemoteAudioTrack> remote_audio_track_;
  agora::agora_refptr<agora::rtc::IRemoteVideoTrack> remote_video_track_;

  agora::rtc::IMediaPacketReceiver* media_packet_receiver_{nullptr};
  agora::rtc::IVideoEncodedImageReceiver* video_encoded_receiver_{nullptr};
  agora::media::IAudioFrameObserver* audio_frame_observer_{nullptr};
  agora::agora_refptr<agora::rtc::IVideoSinkBase> video_frame_observer_{nullptr};

  std::mutex observer_lock_;
};
