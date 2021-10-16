
#include <functional>
#include "NGIAgoraRtcConnection.h"
#include "../agoraevent.h"


enum UserState{USER_JOIN, USER_LEAVE};

using OnUserStateChange_fn=std::function<void(const std::string& userId, const UserState& newState)>;

class ConnectionObserver : public agora::rtc::IRtcConnectionObserver,
                                 public agora::rtc::INetworkObserver {
 public:
  ConnectionObserver():_onUserStateChanged(nullptr) {}
  int waitUntilConnected(int waitMs) { return connect_ready_.Wait(waitMs); }

  void setOnUserStateChanged(const OnUserStateChange_fn& f){
      _onUserStateChanged=f;
  }

 public:  // IRtcConnectionObserver
  void onConnected(const agora::rtc::TConnectionInfo& connectionInfo,
                   agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onDisconnected(const agora::rtc::TConnectionInfo& connectionInfo,
                      agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onConnecting(const agora::rtc::TConnectionInfo& connectionInfo,
                    agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onReconnecting(const agora::rtc::TConnectionInfo& connectionInfo,
                      agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onReconnected(const agora::rtc::TConnectionInfo& connectionInfo,
                     agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override;
  void onConnectionLost(const agora::rtc::TConnectionInfo& connectionInfo) override;
  void onLastmileQuality(const agora::rtc::QUALITY_TYPE quality) override {}
  void onTokenPrivilegeWillExpire(const char* token) override {}
  void onTokenPrivilegeDidExpire() override {}
  void onConnectionFailure(const agora::rtc::TConnectionInfo& connectionInfo,
                           agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) override {}
  void onUserJoined(agora::user_id_t userId) override;
  void onUserLeft(agora::user_id_t userId, agora::rtc::USER_OFFLINE_REASON_TYPE reason) override;
  void onTransportStats(const agora::rtc::RtcStats& stats) override {}
  void onLastmileProbeResult(const agora::rtc::LastmileProbeResult& result) override {}
  void onChannelMediaRelayStateChanged(int state, int code) override {}

 public:  // INetworkObserver
  void onUplinkNetworkInfoUpdated(const agora::rtc::UplinkNetworkInfo& info) override;

 private:
  SampleEvent connect_ready_;
  SampleEvent disconnect_ready_;

  OnUserStateChange_fn     _onUserStateChanged;
};

using ConnectionObserver_ptr=std::shared_ptr<ConnectionObserver>;
