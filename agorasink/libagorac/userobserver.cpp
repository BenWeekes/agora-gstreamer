#include "userobserver.h"
#include <mutex>

#include "helpers/agoralog.h"

//UserObserver

MyUserObserver::MyUserObserver(agora::rtc::ILocalUser* _local_user) :
    local_user(_local_user)
{
    local_user->registerLocalUserObserver(this);
}

MyUserObserver::~MyUserObserver()
{
    local_user->unregisterLocalUserObserver(this);
}

agora::rtc::ILocalUser* MyUserObserver::GetLocalUser()
{
    return local_user;
}

void MyUserObserver::setVideoEncodedImageReceiver(agora::rtc::IVideoEncodedImageReceiver* receiver)
{
    video_encoded_receiver = receiver;
}

void MyUserObserver::setVideoFrameObserver(agora::agora_refptr<agora::rtc::IVideoSinkBase> observer)
{
    video_frame_observer = observer;
}

void MyUserObserver::unsetVideoFrameObserver()
{
}

void MyUserObserver::onUserVideoTrackSubscribed(agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack)
{
   // logger->Info("video track subscribed: userId {}, codecType {}, encodedFrameOnly {}", userId,
     //   trackInfo.codecType, trackInfo.encodedFrameOnly);
     //
   logMessage("AGORA: video track subscribed");
    
    std::lock_guard<std::mutex> lock(observer_lock);
    remote_video_track = videoTrack;
    if (remote_video_track && video_encoded_receiver)
        remote_video_track->registerVideoEncodedImageReceiver(video_encoded_receiver);
    if (remote_video_track && video_frame_observer)
        remote_video_track->addRenderer(video_frame_observer);
}

void MyUserObserver::onRemoteVideoStreamInfoUpdated(const agora::rtc::RemoteVideoStreamInfo& info)
{
   // logger->Info("remote video stream info updated: uid {}, streamType {}", info.uid, info.stream_type);
   logMessage("AGORA: remote video stream info updated");
}

void MyUserObserver::onVideoSubscribeStateChanged(const char* channel, agora::rtc::uid_t uid,
    agora::rtc::STREAM_SUBSCRIBE_STATE oldState, agora::rtc::STREAM_SUBSCRIBE_STATE newState,
    int elapseSinceLastState)
{
   // logger->Info("video state changed: channel {}, uid {}, oldState {}, newState", channel, uid, oldState, newState);
   logMessage("AGORA: video state changed");
}

void MyUserObserver::onUserInfoUpdated(agora::user_id_t userId, USER_MEDIA_INFO msg, bool val)
{
   // logger->Info("user info updated: userId {}, msg {}, val {}", userId, msg, val);
   logMessage("AGORA: user info updated");

}

void MyUserObserver::onIntraRequestReceived()
{
  logMessage("AGORA: ********iFrame request received*******");
}
