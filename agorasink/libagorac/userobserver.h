#ifndef __USEROBSERVER_H__
#define __USEROBSERVER_H__

#include <IAgoraService.h>
#include <NGIAgoraRtcConnection.h>
#include <mutex>

class MyUserObserver : public agora::rtc::ILocalUserObserver
{
public:
    MyUserObserver(agora::rtc::ILocalUser* _local_user);
    virtual ~MyUserObserver();

    agora::rtc::ILocalUser* GetLocalUser();

    void setVideoEncodedImageReceiver(agora::rtc::IVideoEncodedImageReceiver* receiver);

    void setVideoFrameObserver(agora::agora_refptr<agora::rtc::IVideoSinkBase> observer);
    void unsetVideoFrameObserver();


public:
    // inherit from agora::rtc::ILocalUserObserver
    void onAudioTrackPublishSuccess(
        agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack) override {}

    void onUserAudioTrackSubscribed(
        agora::user_id_t userId,
        agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack) override {}

    void onAudioTrackPublicationFailure(agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack,
                                      agora::ERROR_CODE_TYPE error) override {}

    void onUserAudioTrackStateChanged(agora::user_id_t userId,
                                    agora::agora_refptr<agora::rtc::IRemoteAudioTrack> audioTrack,
                                    agora::rtc::REMOTE_AUDIO_STATE state,
                                    agora::rtc::REMOTE_AUDIO_STATE_REASON reason,
                                    int elapsed) override {}

    void onLocalAudioTrackStateChanged(agora::agora_refptr<agora::rtc::ILocalAudioTrack> audioTrack,
                                     agora::rtc::LOCAL_AUDIO_STREAM_STATE state,
                                     agora::rtc::LOCAL_AUDIO_STREAM_ERROR errorCode) override {}

    void onVideoTrackPublishSuccess(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack) override {}

    void onVideoTrackPublicationFailure(agora::agora_refptr<agora::rtc::ILocalVideoTrack> videoTrack,
        agora::ERROR_CODE_TYPE error) override {}

    void onUserVideoTrackSubscribed(agora::user_id_t userId, agora::rtc::VideoTrackInfo trackInfo,
        agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack) override;

  void onUserVideoTrackStateChanged(agora::user_id_t userId,
                                    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
                                    agora::rtc::REMOTE_VIDEO_STATE state,
                                    agora::rtc::REMOTE_VIDEO_STATE_REASON reason,
                                    int elapsed) override {}

  void onRemoteVideoTrackStatistics(agora::agora_refptr<agora::rtc::IRemoteVideoTrack> videoTrack,
                                    const agora::rtc::RemoteVideoTrackStats& stats) override {}

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

  void onRemoteVideoStreamInfoUpdated(const agora::rtc::RemoteVideoStreamInfo& info) override;
  
  void onAudioSubscribeStateChanged(const char* channel, agora::rtc::uid_t uid,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE oldState,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE newState,
                                    int elapseSinceLastState) override {}

  void onVideoSubscribeStateChanged(const char* channel, agora::rtc::uid_t uid,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE oldState,
                                    agora::rtc::STREAM_SUBSCRIBE_STATE newState,
                                    int elapseSinceLastState) override;

  void onAudioPublishStateChanged(const char* channel, agora::rtc::STREAM_PUBLISH_STATE oldState,
                                  agora::rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override {}

  void onVideoPublishStateChanged(const char* channel, agora::rtc::STREAM_PUBLISH_STATE oldState,
                                  agora::rtc::STREAM_PUBLISH_STATE newState,
                                  int elapseSinceLastState) override {}

private:
    agora::rtc::ILocalUser* local_user = nullptr;
    agora::rtc::IVideoEncodedImageReceiver* video_encoded_receiver = nullptr;
    agora::agora_refptr<agora::rtc::IVideoSinkBase> video_frame_observer = nullptr;

    agora::agora_refptr<agora::rtc::IRemoteVideoTrack> remote_video_track;

    std::mutex observer_lock;
};

#endif
