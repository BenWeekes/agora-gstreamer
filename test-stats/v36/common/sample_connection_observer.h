#include "NGIAgoraRtcConnection.h"
#include "sample_event.h"

class SampleConnectionObserver : public agora::rtc::IRtcConnectionObserver,
                                 public agora::rtc::INetworkObserver {
 public:
  SampleConnectionObserver() {}
  int waitUntilConnected(int waitMs) { return connect_ready_.Wait(waitMs); }

 public:  // IRtcConnectionObserver
  void onConnected(const agora::rtc::TConnectionInfo& connectionInfo,
                   agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onDisconnected(const agora::rtc::TConnectionInfo& connectionInfo,
                      agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onConnecting(const agora::rtc::TConnectionInfo& connectionInfo,
                    agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}
  void onReconnecting(const agora::rtc::TConnectionInfo& connectionInfo,
                      agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}
  void onReconnected(const agora::rtc::TConnectionInfo& connectionInfo,
                     agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}
  void onConnectionLost(const agora::rtc::TConnectionInfo& connectionInfo) override;
  void onLastmileQuality(const agora::rtc::QUALITY_TYPE quality) override;
  void onTokenPrivilegeWillExpire(const char* token) override;
  void onTokenPrivilegeDidExpire() override {}
  void onConnectionFailure(const agora::rtc::TConnectionInfo& connectionInfo,
                           agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}
  void onUserJoined(agora::user_id_t userId) override;
  void onUserLeft(agora::user_id_t userId, agora::rtc::USER_OFFLINE_REASON_TYPE reason) override;
  void onTransportStats(const agora::rtc::RtcStats& stats) override {}
  void onLastmileProbeResult(const agora::rtc::LastmileProbeResult& result) override {}
  void onChannelMediaRelayStateChanged(int state, int code) override {}
  void onApiCallExecuted(int err, const char* api, const char* result) override;
  void onStreamMessageError(agora::user_id_t userId, int streamId, int code, int missed,int cached) override;
  void onError(agora::ERROR_CODE_TYPE error, const char* msg)override;
 public:  // INetworkObserver
  void onUplinkNetworkInfoUpdated(const agora::rtc::UplinkNetworkInfo& info) override;

 private:
  SampleEvent connect_ready_;
  SampleEvent disconnect_ready_;
};
