//  Agora RTC/MEDIA SDK
//
//  Created by Pengfei Han in 2020-03.
//  Copyright (c) 2020 Agora.io. All rights reserved.
//

#include "sample_local_user_observer.h"

#include "log.h"

#include <iostream>

SampleLocalUserObserver::SampleLocalUserObserver(agora::rtc::ILocalUser* local_user)
    : local_user_(local_user) {
  local_user_->registerLocalUserObserver(this);
}

SampleLocalUserObserver::~SampleLocalUserObserver() {
  local_user_->unregisterLocalUserObserver(this);
}

agora::rtc::ILocalUser* SampleLocalUserObserver::GetLocalUser() { return local_user_; }

void SampleLocalUserObserver::PublishAudioTrack(
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) {
  local_user_->publishAudio(audioTrack);
}

void SampleLocalUserObserver::PublishVideoTrack(
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) {
  local_user_->publishVideo(videoTrack);
}

void SampleLocalUserObserver::UnpublishAudioTrack(
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) {
  local_user_->unpublishAudio(audioTrack);
}

void SampleLocalUserObserver::UnpublishVideoTrack(
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) {
  local_user_->unpublishVideo(videoTrack);
}

void SampleLocalUserObserver::onUserAudioTrackSubscribed(
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

void SampleLocalUserObserver::onUserVideoTrackSubscribed(
    agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack) {
  AG_LOG(INFO, "onUserVideoTrackSubscribed: userId %s, codecType %d, encodedFrameOnly %d", userId,
         trackInfo.codecType, trackInfo.encodedFrameOnly);
  std::lock_guard<std::mutex> _(observer_lock_);
  remote_video_track_ = videoTrack;
  if (remote_video_track_ && video_encoded_receiver_) {
    remote_video_track_->registerVideoEncodedImageReceiver(video_encoded_receiver_);
  }
  if (remote_video_track_ && media_packet_receiver_) {
    remote_video_track_->registerMediaPacketReceiver(media_packet_receiver_);
  }
  if (remote_video_track_ && video_frame_observer_) {
    remote_video_track_->addRenderer(video_frame_observer_);
  }
}

void SampleLocalUserObserver::onUserInfoUpdated(agora::user_id_t userId,
                                                ILocalUserObserver::USER_MEDIA_INFO msg, bool val) {
  AG_LOG(INFO, "onUserInfoUpdated: userId %s, msg %d, val %d", userId, msg, val);
}

void SampleLocalUserObserver::onUserAudioTrackStateChanged(
    agora::user_id_t userId, agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack,
    agora::rtc::REMOTE_AUDIO_STATE state, agora::rtc::REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) {
  AG_LOG(INFO, "onUserAudioTrackStateChanged: userId %s, state %d, reason %d", userId, state,
         reason);
}

void SampleLocalUserObserver::onIntraRequestReceived() {
  AG_LOG(INFO, "onIntraRequestReceived");
}

void SampleLocalUserObserver::onStreamMessage(agora::user_id_t userId, int streamId, const char* data,size_t length){
  AG_LOG(INFO,"onStreamMessage,userId %s, streamId %d\n",userId,streamId);
}

void SampleLocalUserObserver::onRemoteVideoTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
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