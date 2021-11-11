//  Agora RTC/MEDIA SDK
//
//  Created by Pengfei Han in 2020-03.
//  Copyright (c) 2020 Agora.io. All rights reserved.
//

#include "userobserver.h"
#include <iostream>

UserObserver::UserObserver(agora::rtc::ILocalUser* local_user, const bool& verbose)
    : local_user_(local_user),
    _onUserInfoChanged(nullptr),
    _onUserVolumeChanged(nullptr),
    _verbose(verbose){
  local_user_->registerLocalUserObserver(this);
}

UserObserver::~UserObserver() {
  local_user_->unregisterLocalUserObserver(this);
}

agora::rtc::ILocalUser* UserObserver::GetLocalUser() { return local_user_; }

void UserObserver::PublishAudioTrack(
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) {
  local_user_->publishAudio(audioTrack);
}

void UserObserver::PublishVideoTrack(
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) {
  local_user_->publishVideo(videoTrack);
}

void UserObserver::UnpublishAudioTrack(
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) {
  local_user_->unpublishAudio(audioTrack);
}

void UserObserver::UnpublishVideoTrack(
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) {
  local_user_->unpublishVideo(videoTrack);
}

void UserObserver::onUserAudioTrackSubscribed(
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

void UserObserver::onUserVideoTrackSubscribed(
    agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack) {
 /* AG_LOG(INFO, "onUserVideoTrackSubscribed: userId %s, codecType %d, encodedFrameOnly %d", userId,
         trackInfo.codecType, trackInfo.encodedFrameOnly);*/
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

void UserObserver::onUserInfoUpdated(agora::user_id_t userId,
                                                ILocalUserObserver::USER_MEDIA_INFO msg, bool val) {
 // AG_LOG(INFO, "onUserInfoUpdated: userId %s, msg %d, val %d", userId, msg, val);

   if(_onUserInfoChanged!=nullptr){
       _onUserInfoChanged(userId, msg, val);
   }

   if(_verbose){
       std::cout<<"UserObserver::onUserInfoUpdated: "
           <<"userId "
           <<userId
           <<", msg "<<msg
           <<", val "<<val 
           <<std::endl;
   }
 
}

void UserObserver::onUserAudioTrackStateChanged(
    agora::user_id_t userId, agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack,
    agora::rtc::REMOTE_AUDIO_STATE state, agora::rtc::REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) {
  

}

void UserObserver::onIntraRequestReceived() {
  if(_verbose==false)  return;
   std::cout<<"Agora sdk requested an iframe\n";
}

void UserObserver::onAudioVolumeIndication(const agora::rtc::AudioVolumeInfo* speakers,
                                       unsigned int speakerNumber, int totalVolume) {

   
    /* if(speakers!=nullptr && speakers->volume>0){

       if(_onUserVolumeChanged!=nullptr){
           _onUserVolumeChanged(speakers->userId, speakers->volume);
       }
       std::cout<< "spearker: "<<speakers->uid
                <<" speakerNumber: "<<speakerNumber
                <<" volume: "<<speakers->volume
                <<std::endl; 
     }*/
}

void UserObserver::onRemoteVideoTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
                                    const agora::rtc::RemoteVideoTrackStats& stats)
{

 #if SDK_BUILD_NUM!=110077
  if(_verbose==false)  return;

   std::cout<< "video stats (remote): "
            <<" receivedBitrate "<<stats.receivedBitrate
            <<", decoderOutputFrameRate "<<stats.decoderOutputFrameRate
            <<", rendererOutputFrameRate "<<stats.rendererOutputFrameRate
            <<", frameLossRate "<<stats.frameLossRate
            <<", packetLossRate "<<stats.packetLossRate
            <<", rxStreamType "<<stats.rxStreamType
            <<", totalFrozenTime "<<stats.totalFrozenTime
            <<", frozenRate "<<stats.frozenRate
            <<", totalDecodedFrames "<<stats.totalDecodedFrames
            <<", avSyncTimeMs "<<stats.avSyncTimeMs
            <<", downlink_process_time_ms "<<stats.downlink_process_time_ms
            <<", frame_render_delay_ms "<<stats.frame_render_delay_ms
            <<std::endl;
 #endif 

}

void UserObserver::onLocalVideoTrackStatistics(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
                                   const agora::rtc::LocalVideoTrackStats& stats) 
{
     
     if(_verbose==false)  return;

     std::cout<< "video stats (local): "
            <<" number_of_streams "<<stats.number_of_streams
            <<", bytes_major_stream "<<stats.bytes_major_stream
            <<", bytes_minor_stream "<<stats.bytes_minor_stream
            <<", frames_encoded "<<stats.frames_encoded
            <<", ssrc_major_stream "<<stats.ssrc_major_stream
            <<", ssrc_minor_stream "<<stats.ssrc_minor_stream
            <<", input_frame_rate "<<stats.input_frame_rate
            <<", encode_frame_rate "<<stats.encode_frame_rate
            <<", render_frame_rate "<<stats.render_frame_rate
            <<", target_media_bitrate_bps "<<stats.target_media_bitrate_bps
            <<", media_bitrate_bps "<<stats.media_bitrate_bps
            <<", total_bitrate_bps "<<stats.total_bitrate_bps
            <<", width "<<stats.width
            <<", height "<<stats.height
            <<", encoder_type "<<stats.encoder_type
            <<std::endl;
}
