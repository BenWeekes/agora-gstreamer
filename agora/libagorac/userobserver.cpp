#include "userobserver.h"
#include <mutex>

#include "helpers/agoralog.h"

UserObserver::UserObserver(agora::rtc::ILocalUser* _local_user) :
    local_user(_local_user)
{
    local_user->registerLocalUserObserver(this);
}

UserObserver::~UserObserver()
{
     //local_user->unregisterLocalUserObserver(this);
    _iFrame_request_fn=nullptr;
}

agora::rtc::ILocalUser* UserObserver::GetLocalUser()
{
    return local_user;
}

void UserObserver::setVideoEncodedImageReceiver(agora::rtc::IVideoEncodedImageReceiver* receiver)
{
    video_encoded_receiver = receiver;
}

void UserObserver::setVideoFrameObserver(agora::agora_refptr<agora::rtc::IVideoFrameObserver2> observer)
{
    video_frame_observer = observer;
}

void UserObserver::unsetVideoFrameObserver()
{
}

void UserObserver::setOnIframeRequestReceivedFn(const IFrameRequest_fn& fn){
    _iFrame_request_fn=fn;
  }

void UserObserver::onUserVideoTrackSubscribed(agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack)
{
    /*g_logger->Info("video track subscribed: userId {}, codecType {}, encodedFrameOnly {}", userId,
        trackInfo.codecType, trackInfo.encodedFrameOnly);*/

    
    
    std::lock_guard<std::mutex> lock(observer_lock);
    remote_video_track = videoTrack;
    if (remote_video_track && video_encoded_receiver)
        remote_video_track->registerVideoEncodedImageReceiver(video_encoded_receiver);
    if (remote_video_track && video_frame_observer)
        remote_video_track->registerVideoFrameObserver(video_frame_observer);
}

void UserObserver::onUserVideoTrackStateChanged(agora::user_id_t userId, 
    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
    agora::rtc::REMOTE_VIDEO_STATE state, agora::rtc::REMOTE_VIDEO_STATE_REASON reason, int elapsed)
{
   /* g_logger->Info("user video track state changed: userId {}, state {}, reason {}, elapsed {}", userId,
        state, reason, elapsed);*/

    if (reason == agora::rtc::REMOTE_VIDEO_STATE_REASON_REMOTE_OFFLINE){
        
    }
        
}

void UserObserver::onRemoteVideoTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
    const agora::rtc::RemoteVideoTrackStats& stats)
{
    //g_logger->Info("remote video track statistics: userId {}, delay {}, width {}, height {}", stats.uid,
    //    stats.delay, stats.width, stats.height);
}

void UserObserver::onLocalVideoTrackStateChanged(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
    agora::rtc::LOCAL_VIDEO_STREAM_STATE state, agora::rtc::LOCAL_VIDEO_STREAM_ERROR errorCode)
{
   // g_logger->Info("local video track state changed: state {}, errorCode {}", state, errorCode);
}

void UserObserver::onLocalVideoTrackStatistics(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
    const agora::rtc::LocalVideoTrackStats& stats)
{
    //g_logger->Info("local video track statistics: width {}, height {}", stats.width, stats.height);
}

void UserObserver::onRemoteVideoStreamInfoUpdated(const agora::rtc::RemoteVideoStreamInfo& info)
{
    //g_logger->Info("remote video stream info updated: uid {}, streamType {}", info.uid, info.stream_type);
}

void UserObserver::onVideoSubscribeStateChanged(const char* channel, agora::rtc::uid_t uid,
    agora::rtc::STREAM_SUBSCRIBE_STATE oldState, agora::rtc::STREAM_SUBSCRIBE_STATE newState,
    int elapseSinceLastState)
{
    //g_logger->Info("video state changed: channel {}, uid {}, oldState {}, newState", channel, uid, oldState, newState);
}

void UserObserver::onUserInfoUpdated(agora::user_id_t userId, USER_MEDIA_INFO msg, bool val)
{
   // g_logger->Info("user info updated: userId {}, msg {}, val {}", userId, msg, val);
}

void UserObserver::onIntraRequestReceived()
{
  if(_iFrame_request_fn!=nullptr){
     _iFrame_request_fn();
  }
}
