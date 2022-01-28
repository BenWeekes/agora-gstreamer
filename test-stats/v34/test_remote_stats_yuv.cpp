//  Agora RTC/MEDIA SDK
//
//  Created by Jay Zhang in 2020-04.
//  Copyright (c) 2020 Agora.io. All rights reserved.
//

#include <csignal>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>

#include "AgoraRefCountedObject.h"
#include "IAgoraService.h"
#include "NGIAgoraRtcConnection.h"
#include "common/log.h"
#include "common/opt_parser.h"
#include "common/sample_common.h"
#include "common/sample_connection_observer.h"
#include "common/sample_local_user_observer2.h"

#include "common/file_parser/helper_h264_parser.h"
#include "common/helper.h"


#define DEFAULT_VIDEO_FILE "received_video"

#define DEFAULT_CONNECT_TIMEOUT_MS (3000)
#define DEFAULT_FRAME_RATE (30)

struct SampleOptions {
  std::string appId;
  std::string channelId;
  std::string userId;
  std::string videoFile = DEFAULT_VIDEO_FILE;
};

static void sendOneH264Frame(
    int frameRate, std::unique_ptr<HelperH264Frame> h264Frame,
    agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> videoFrameSender) {
  agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo;
  videoEncodedFrameInfo.rotation = agora::rtc::VIDEO_ORIENTATION_0;
  videoEncodedFrameInfo.codecType = agora::rtc::VIDEO_CODEC_H264;
  videoEncodedFrameInfo.framesPerSecond = frameRate;
  videoEncodedFrameInfo.frameType =
      (h264Frame.get()->isKeyFrame ? agora::rtc::VIDEO_FRAME_TYPE::VIDEO_FRAME_TYPE_KEY_FRAME
                                   : agora::rtc::VIDEO_FRAME_TYPE::VIDEO_FRAME_TYPE_DELTA_FRAME);

  /*   AG_LOG(DEBUG, "sendEncodedVideoImage, buffer %p, len %d, frameType %d",
           reinterpret_cast<uint8_t*>(h264Frame.get()->buffer.get()), h264Frame.get()->bufferLen,
           videoEncodedFrameInfo.frameType); */

  videoFrameSender->sendEncodedVideoImage(reinterpret_cast<uint8_t*>(h264Frame.get()->buffer.get()),
                                          h264Frame.get()->bufferLen, videoEncodedFrameInfo);
}


static void SampleSendVideoH264Task(const std::string& fileName,
                                    agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> videoFrameSender,
                                    bool& exitFlag) {


  std::unique_ptr<HelperH264FileParser> h264FileParser(
      new HelperH264FileParser(fileName.c_str()));

  h264FileParser->initialize();

  // Calculate send interval based on frame rate. H264 frames are sent at this interval
  PacerInfo pacer = {0, 1000 / DEFAULT_FRAME_RATE, std::chrono::steady_clock::now()};


  while (!exitFlag) {
    if (auto h264Frame = h264FileParser->getH264Frame()) {
      sendOneH264Frame(DEFAULT_FRAME_RATE, std::move(h264Frame), videoFrameSender);
      waitBeforeNextSend(pacer);  // sleep for a while before sending next frame
    }
  };

}

static bool exitFlag = false;
static void SignalHandler(int sigNo) { exitFlag = true; }

int main(int argc, char* argv[]) {
  SampleOptions options;
  opt_parser optParser;

  optParser.add_long_opt("token", &options.appId, "The token for authentication");
  optParser.add_long_opt("channelId", &options.channelId, "Channel Id");
  optParser.add_long_opt("userId", &options.userId, "User Id / default is 0");
   optParser.add_long_opt("videoFile", &options.videoFile, "Output video file");

  if ((argc <= 1) || !optParser.parse_opts(argc, argv)) {
    std::ostringstream strStream;
    optParser.print_usage(argv[0], strStream);
    std::cout << strStream.str() << std::endl;
    return -1;
  }

  if (options.appId.empty()) {
    AG_LOG(ERROR, "Must provide appId!");
    return -1;
  }

  if (options.channelId.empty()) {
    AG_LOG(ERROR, "Must provide channelId!");
    return -1;
  }

  std::signal(SIGQUIT, SignalHandler);
  std::signal(SIGABRT, SignalHandler);
  std::signal(SIGINT, SignalHandler);

  // Create Agora service
  auto service = createAndInitAgoraService(false, true, true);
  if (!service) {
    AG_LOG(ERROR, "Failed to creating Agora service!");
  }

  // Create Agora connection

  agora::rtc::RtcConnectionConfiguration ccfg;
  //ccfg.clientRoleType = agora::rtc::CLIENT_ROLE_AUDIENCE;
  ccfg.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;


  ccfg.channelProfile = agora::CHANNEL_PROFILE_COMMUNICATION;
  // _rtcConfig.channelProfile = agora::CHANNEL_PROFILE_COMMUNICATION_1v1;
    //_rtcConfig.channelProfile=agora::CHANNEL_PROFILE_LIVE_BROADCASTING;
    //_rtcConfig.channelProfile=agora::CHANNEL_PROFILE_CLOUD_GAMING;
    //_rtcConfig.channelProfile=agora::CHANNEL_PROFILE_LIVE_BROADCASTING_2;

  ccfg.autoSubscribeVideo = false;

  agora::agora_refptr<agora::rtc::IRtcConnection> connection = service->createRtcConnection(ccfg);
  if (!connection) {
    AG_LOG(ERROR, "Failed to creating Agora connection!");
    return -1;
  }

  // Subcribe streams from all remote users
  agora::rtc::ILocalUser::VideoSubscriptionOptions subscriptionOptions;
  connection->getLocalUser()->subscribeAllVideo(subscriptionOptions);

  // Register connection observer to monitor connection event
  auto connObserver = std::make_shared<SampleConnectionObserver>();
  connection->registerObserver(connObserver.get());
  connection->registerNetworkObserver(connObserver.get());

  // Create local user observer
  auto localUserObserver = std::make_shared<SampleLocalUserObserver2>(connection->getLocalUser(),options.videoFile);

  // Connect to Agora channel
  if (connection->connect(options.appId.c_str(), options.channelId.c_str(),
                          options.userId.c_str())) {
    AG_LOG(ERROR, "Failed to connect to Agora channel!");
    return -1;
  }

  //========sender code

  // Create media node factory
  agora::agora_refptr<agora::rtc::IMediaNodeFactory> factory = service->createMediaNodeFactory();
  if (!factory) {
    AG_LOG(ERROR, "Failed to create media node factory!");
  }

  // Create video frame sender
  agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> videoFrameSender =
      factory->createVideoEncodedImageSender();
  if (!videoFrameSender) {
    AG_LOG(ERROR, "Failed to create video frame sender!");
    return 0;
  }

  // Create video track
  agora::base::SenderOptions option;
  option.ccMode = agora::base::CC_ENABLED;
  agora::agora_refptr<agora::rtc::ILocalVideoTrack> customVideoTrack =
      service->createCustomVideoTrack(videoFrameSender, option);
  if (!customVideoTrack) {
    AG_LOG(ERROR, "Failed to create video track!");
    return 0;
  }

  // Publish video track
  connection->getLocalUser()->publishVideo(customVideoTrack);

  // Wait until connected before sending media stream
  connObserver->waitUntilConnected(DEFAULT_CONNECT_TIMEOUT_MS);

  std::thread* systhread =
        new std::thread(SampleSendVideoH264Task, "test_multi_slice.h264",videoFrameSender, std::ref(exitFlag));


  // Periodically check exit flag
  while (!exitFlag) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Disconnect from Agora channel
  if (connection->disconnect()) {
    AG_LOG(ERROR, "Failed to disconnect from Agora channel!");
    return -1;
  }
  AG_LOG(INFO, "Disconnected from Agora channel successfully");

  // Destroy Agora connection and related resources
  localUserObserver.reset();
  connection = nullptr;

  // Destroy Agora Service
  service->release();
  service = nullptr;

  return 0;
}
