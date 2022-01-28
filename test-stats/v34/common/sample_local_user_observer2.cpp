//  Agora RTC/MEDIA SDK
//
//  Created by ZZ in 2020-09.
//  Copyright (c) 2020 Agora.io. All rights reserved.
//

#include "sample_local_user_observer2.h"

#include <cstring>
#include <string>

#include "AgoraRefCountedObject.h"
#include "log.h"

#include <iostream>

class VideoFrameObserver : public agora::rtc::IVideoSinkBase {
 public:
  VideoFrameObserver(agora::user_id_t userId, const std::string& outputFilePath)
      : userId_(std::string(userId)), outputFilePath_(outputFilePath), yuvFile_(nullptr) {}

  int onFrame(const agora::media::base::VideoFrame& videoFrame) override;

  virtual ~VideoFrameObserver() = default;

 private:
  std::string userId_;
  std::string outputFilePath_;
  FILE* yuvFile_;
};

int VideoFrameObserver::onFrame(const agora::media::base::VideoFrame& videoFrame) {
  // Create new file to save received YUV frames
  if (!yuvFile_) {
    std::string fileName = outputFilePath_ + "_" + std::string(userId_) + ".yuv";
    if (!(yuvFile_ = fopen(fileName.c_str(), "w+"))) {
      AG_LOG(ERROR, "Failed to create received video file %s", fileName.c_str());
      return -1;
    }
    AG_LOG(INFO, "Created file %s to save received YUV frames", fileName.c_str());
  }

  // Write Y planar
  size_t writeBytes = videoFrame.yStride * videoFrame.height;
  if (fwrite(videoFrame.yBuffer, 1, writeBytes, yuvFile_) != writeBytes) {
    AG_LOG(ERROR, "Error writing decoded video data: %s", std::strerror(errno));
    return -1;
  }

  // Write U planar
  writeBytes = videoFrame.uStride * videoFrame.height / 2;
  if (fwrite(videoFrame.uBuffer, 1, writeBytes, yuvFile_) != writeBytes) {
    AG_LOG(ERROR, "Error writing decoded video data: %s", std::strerror(errno));
    return -1;
  }

  // Write V planar
  writeBytes = videoFrame.vStride * videoFrame.height / 2;
  if (fwrite(videoFrame.vBuffer, 1, writeBytes, yuvFile_) != writeBytes) {
    AG_LOG(ERROR, "Error writing decoded video data: %s", std::strerror(errno));
    return -1;
  }
  return 0;
};

SampleLocalUserObserver2::SampleLocalUserObserver2(agora::rtc::ILocalUser* local_user,std::string filename)
    : local_user_(local_user) ,outputFilePath_(filename){
  local_user_->registerLocalUserObserver(this);
}

SampleLocalUserObserver2::~SampleLocalUserObserver2() {
  local_user_->unregisterLocalUserObserver(this);
}

agora::rtc::ILocalUser* SampleLocalUserObserver2::GetLocalUser() { return local_user_; }

void SampleLocalUserObserver2::PublishAudioTrack(
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) {
  local_user_->publishAudio(audioTrack);
}

void SampleLocalUserObserver2::PublishVideoTrack(
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) {
  local_user_->publishVideo(videoTrack);
}

void SampleLocalUserObserver2::UnpublishAudioTrack(
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) {
  local_user_->unpublishAudio(audioTrack);
}

void SampleLocalUserObserver2::UnpublishVideoTrack(
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) {
  local_user_->unpublishVideo(videoTrack);
}

void SampleLocalUserObserver2::onUserAudioTrackSubscribed(
    agora::user_id_t userId, agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack) {
  std::lock_guard<std::mutex> _(observer_lock_);
  remote_audio_track_ = audioTrack;
  if (remote_audio_track_ && media_packet_receiver_) {
    remote_audio_track_->registerMediaPacketReceiver(media_packet_receiver_);
  }
  if (remote_audio_track_ && audio_frame_observer_) {
    local_user_->registerAudioFrameObserver(audio_frame_observer_);
  }
}

void SampleLocalUserObserver2::onUserVideoTrackSubscribed(
    agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack) {
  AG_LOG(INFO,
         "onUserVideoTrackSubscribed: userId %s, codecType %d, "
         "encodedFrameOnly %d",
         userId, trackInfo.codecType, trackInfo.encodedFrameOnly);
  std::lock_guard<std::mutex> _(observer_lock_);
  remote_video_track_ = videoTrack;
  if (remote_video_track_ && video_encoded_receiver_) {
    remote_video_track_->registerVideoEncodedImageReceiver(video_encoded_receiver_);
  }
  if (remote_video_track_ && media_packet_receiver_) {
    remote_video_track_->registerMediaPacketReceiver(media_packet_receiver_);
  }
  if (videoTrack) {
    agora::agora_refptr<agora::rtc::IVideoSinkBase> videoFrameObserver =
        new agora::RefCountedObject<VideoFrameObserver>(userId, outputFilePath_);
    videoTrack->addRenderer(videoFrameObserver);
    AG_LOG(INFO, "Add video frame observer corresponding to user id %s", userId);
    videoFrameObservers_.emplace(std::string(userId), videoFrameObserver);
  }
}

void SampleLocalUserObserver2::onUserInfoUpdated(agora::user_id_t userId,
                                                 ILocalUserObserver::USER_MEDIA_INFO msg,
                                                 bool val) {
  AG_LOG(INFO, "onUserInfoUpdated: userId %s, msg %d, val %d", userId, msg, val);
}

void SampleLocalUserObserver2::onUserAudioTrackStateChanged(
    agora::user_id_t userId, agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack,
    agora::rtc::REMOTE_AUDIO_STATE state, agora::rtc::REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) {
  AG_LOG(INFO, "onUserAudioTrackStateChanged: userId %s, state %d, reason %d", userId, state,
         reason);
}

void SampleLocalUserObserver2::onIntraRequestReceived() { AG_LOG(INFO, "onIntraRequestReceived"); }

void SampleLocalUserObserver2::onUserVideoTrackStateChanged(
    agora::user_id_t userId, agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
    agora::rtc::REMOTE_VIDEO_STATE state, agora::rtc::REMOTE_VIDEO_STATE_REASON reason,
    int elapsed) {
  if (state == agora::rtc::REMOTE_VIDEO_STATE_STOPPED) {
    auto it = videoFrameObservers_.find(std::string(userId));
    if (it == videoFrameObservers_.end()) {
      AG_LOG(INFO, "onUserVideoTrackStateChanged : No find userId %s", userId);
      return;
    }
    AG_LOG(INFO, "Remove video frame observer corresponding to user id %s", userId);
    agora::agora_refptr<agora::rtc::IVideoSinkBase> videoFrameObserver =
        videoFrameObservers_[userId];
  }
}

void SampleLocalUserObserver2::onStreamMessage(agora::user_id_t userId, int streamId, const char* data,size_t length){
  AG_LOG(INFO,"onStreamMessage,userId %s, streamId %d\n",userId,streamId);
}

void SampleLocalUserObserver2::onRemoteVideoTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
                                    const agora::rtc::RemoteVideoTrackStats& stats){

    std::cout<<"*****************onRemoteVideoTrackStatistics*********************\n";
    std::cout<<"receivedBitrate: "<<stats.receivedBitrate<<std::endl;
    std::cout<<"decoderOutputFrameRate: "<<stats.decoderOutputFrameRate<<std::endl;
    std::cout<<"rendererOutputFrameRate: "<<stats.rendererOutputFrameRate<<std::endl;
    std::cout<<"frameLossRate: "<<stats.frameLossRate<<std::endl;
    std::cout<<"packetLossRate: "<<stats.packetLossRate<<std::endl;
    std::cout<<"rxStreamType: "<<stats.rxStreamType<<std::endl;

    std::cout<<"totalFrozenTime: "<<stats.totalFrozenTime<<std::endl;
    std::cout<<"frozenRate: "<<stats.frozenRate<<std::endl;
    std::cout<<"totalDecodedFrames: "<<stats.totalDecodedFrames<<std::endl;

    std::cout<<"avSyncTimeMs: "<<stats.avSyncTimeMs<<std::endl;
    std::cout<<"downlink_process_time_ms: "<<stats.downlink_process_time_ms<<std::endl;
    std::cout<<"frame_render_delay_ms: "<<stats.frame_render_delay_ms<<std::endl;

    std::cout<<"******************************************************";
 }