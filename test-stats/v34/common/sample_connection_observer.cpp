
#include "sample_connection_observer.h"

#include "log.h"

void SampleConnectionObserver::onConnected(const agora::rtc::TConnectionInfo& connectionInfo,
                                           agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  AG_LOG(INFO, "onConnected: id %u, channelId %s, localUserId %s, reason %d\n", connectionInfo.id,
         connectionInfo.channelId.get()->c_str(), connectionInfo.localUserId.get()->c_str(),
         reason);

  // notify the thread which is waiting for the SDK to be connected
  connect_ready_.Set();
}

void SampleConnectionObserver::onDisconnected(const agora::rtc::TConnectionInfo& connectionInfo,
                                              agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason) {
  AG_LOG(INFO, "onDisconnected: id %u, channelId %s, localUserId %s, reason %d\n",
         connectionInfo.id, connectionInfo.channelId.get()->c_str(),
         connectionInfo.localUserId.get()->c_str(), reason);

  // notify the thread which is waiting for the SDK to be disconnected
  disconnect_ready_.Set();
}

void SampleConnectionObserver::onUplinkNetworkInfoUpdated(const agora::rtc::UplinkNetworkInfo& info) {
  AG_LOG(INFO, "onUplinkNetworkInfoUpdated: video_encoder_target_bitrate_bps %d\n",
         info.video_encoder_target_bitrate_bps);
}

void SampleConnectionObserver::onUserJoined(agora::user_id_t userId) {
  AG_LOG(INFO, "onUserJoined: userId %s\n", userId);
}

void SampleConnectionObserver::onUserLeft(agora::user_id_t userId,
                                          agora::rtc::USER_OFFLINE_REASON_TYPE reason) {
  AG_LOG(INFO, "onUserLeft: userId %s, reason %d\n", userId, reason);
}

void SampleConnectionObserver::onConnectionLost(const agora::rtc::TConnectionInfo& connectionInfo){
  AG_LOG(INFO, "onConnectionLost: id %u, channelId %s, localUserId %s\n",
         connectionInfo.id, connectionInfo.channelId.get()->c_str(),
         connectionInfo.localUserId.get()->c_str());
}

void SampleConnectionObserver::onApiCallExecuted(int err, const char* api, const char* result){
  AG_LOG(INFO,"onApiCallExecuted: err %d, api %s,  result %s\n",err,api,result);
}

void SampleConnectionObserver::onTokenPrivilegeWillExpire(const char* token){
  AG_LOG(WARNING,"onTokenPrivilegeWillExpire: taken %s\n",token);
}

void SampleConnectionObserver::onLastmileQuality(const agora::rtc::QUALITY_TYPE quality){
  AG_LOG(INFO,"onLastmileQuality: quality %d\n",quality);
}


void SampleConnectionObserver::onStreamMessageError(agora::user_id_t userId, int streamId, int code, int missed,int cached){
  AG_LOG(INFO,"onStreamMessage,userId %s, streamId %d, code %d, missed %d,cached %d\n",
  userId,streamId,code,missed,cached);
}

void SampleConnectionObserver::onError(agora::ERROR_CODE_TYPE error, const char* msg){
AG_LOG(ERROR,"onError:,error %d,msg  %s\n",error,msg);
}