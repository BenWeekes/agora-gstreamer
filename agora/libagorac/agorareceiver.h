#ifndef __AGORARECEIVERUSER_H__
#define __AGORARECEIVERUSER_H__

#include <memory>
#include <chrono>
#include <functional>
#include <fstream>
#include <list>

#include "observer/pcmframeobserver.h"
#include "observer/h264frameobserver.h"
#include "observer/connectionobserver.h"

//agora header files
#include "NGIAgoraRtcConnection.h"
#include "IAgoraService.h"
#include "AgoraBase.h"

#include "userobserver.h"
#include "helpers/context.h"
#include "helpers/utilities.h"

class AgoraReceiverUser 
{
public:
    AgoraReceiverUser(const std::string& _appId,
                      const std::string& _channel,
                      const std::string& _userId,
                      bool receiveAudio=false,
                      bool receiveVideo=false,
                      bool verbose=false,
                      const std::string& filePath="");

    virtual ~AgoraReceiverUser();

    virtual bool connect();
    virtual bool disconnect();

    void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);
    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

    size_t getNextVideoFrame(unsigned char* data, size_t max_buffer_size, int* is_key_frame);

    size_t getNextAudioFrame(unsigned char* data, size_t max_buffer_size);

protected:

    bool doConnect();

    void subscribeUser(const std::string& userId);

private:
    std::shared_ptr<H264FrameReceiver>  h264FrameReceiver;
    std::shared_ptr<UserObserver>       localUserObserver;

    PcmFrameObserver_ptr                 _pcmFrameObserver;
    ConnectionObserver_ptr               _connectionObserver;

    std::string                          _appId;
    std::string                          _channel;
    std::string                          _userId;

    agora::base::IAgoraService*                     _service;
    agora::agora_refptr<agora::rtc::IRtcConnection> _connection;
    agora::rtc::RtcConnectionConfiguration          _rtcConfig;

    agora::agora_refptr<agora::rtc::IMediaNodeFactory> _factory;
    agora::agora_refptr<agora::rtc::ILocalAudioTrack> _customAudioTrack;
    agora::agora_refptr<agora::rtc::ILocalVideoTrack> _customVideoTrack;

    bool                                           _connected = false;

     WorkQueue_ptr                                 _receivedVideoFrames;
     WorkQueue_ptr                                 _receivedAudioFrames;

     bool                                          _receiveAudio;
     bool                                          _receiveVideo;
     bool                                          _verbose;

     TimePoint                                     _lastReceivedFrameTime;

     agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> _videoFrameSender;

     std::shared_ptr<std::thread>      _senderThread;
     std::string                       _filePath;

     std::list<std::string>             _activeUsers;
     std::string                        _currentVideoUser;
};


#endif
