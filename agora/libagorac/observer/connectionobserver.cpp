
#include "connectionobserver.h"
#include "AgoraBase.h"

#include "iostream"

void ConnectionObserver::onConnected(const agora::rtc::TConnectionInfo &connectionInfo,
										   agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
	/*AG_LOG(INFO, "onConnected: id %u, channelId %s, localUserId %s, reason %d\n", connectionInfo.id,
		   connectionInfo.channelId.get()->c_str(), connectionInfo.localUserId.get()->c_str(),
		   reason);*/

    std::cout<<"onConnected: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<", reason "<<reason<<std::endl;

	// notify the thread which is waiting for the SDK to be connected
	connect_ready_.Set();
}

void ConnectionObserver::onDisconnected(const agora::rtc::TConnectionInfo &connectionInfo,
											  agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
	std::cout<<"onDisconnected: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<", reason "<<reason<<std::endl;

	// notify the thread which is waiting for the SDK to be disconnected
	disconnect_ready_.Set();
}

void ConnectionObserver::onConnecting(const agora::rtc::TConnectionInfo &connectionInfo,
											agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
	std::cout<<"onConnecting: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<", reason "<<reason<<std::endl;
}

void ConnectionObserver::onReconnecting(const agora::rtc::TConnectionInfo &connectionInfo,
											  agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
	std::cout<<"onReconnecting: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<", reason "<<reason<<std::endl;
}

void ConnectionObserver::onReconnected(const agora::rtc::TConnectionInfo &connectionInfo,
											 agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
	std::cout<<"onReconnected: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<", reason "<<reason<<std::endl;
}

void ConnectionObserver::onConnectionLost(const agora::rtc::TConnectionInfo &connectionInfo)
{
	std::cout<<"onConnectionLost: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<std::endl;
}

void ConnectionObserver::onUplinkNetworkInfoUpdated(const agora::rtc::UplinkNetworkInfo &info)
{
    //TODO:
	/*AG_LOG(INFO, "onBandwidthEstimationUpdated: video_encoder_target_bitrate_bps %d\n",
		   info.video_encoder_target_bitrate_bps);*/
}

void ConnectionObserver::onUserJoined(agora::user_id_t userId)
{

    std::cout <<"onUserJoined: userId "<<userId<<std::endl;

    if(_onUserStateChanged!=nullptr){
        _onUserStateChanged(userId, USER_JOIN);
    }
}

void ConnectionObserver::onUserLeft(agora::user_id_t userId,
										  agora::rtc::USER_OFFLINE_REASON_TYPE reason)
{

   std::cout<<"onUserLeft: "<<"userId: "<<userId<<std::endl;

   if(_onUserStateChanged!=nullptr){
        _onUserStateChanged(userId, USER_LEAVE);
   }
}
