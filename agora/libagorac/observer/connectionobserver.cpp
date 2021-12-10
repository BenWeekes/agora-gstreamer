
#include "connectionobserver.h"
#include "AgoraBase.h"

#include "../agoraio.h"

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

    if(_onUserConnected!=nullptr){
        _onUserConnected(connectionInfo.localUserId.get()->c_str(), USER_CONNECTED);
    }

	// notify the thread which is waiting for the SDK to be connected
	connect_ready_.Set();

    std::string userId=connectionInfo.localUserId.get()->c_str();
    if(_parent!=nullptr){
        _parent->addEvent(AGORA_EVENT_ON_CONNECTED, userId,0,0);
    }
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

    std::string userId=connectionInfo.localUserId.get()->c_str();
    if(_parent!=nullptr){
         _parent->addEvent(AGORA_EVENT_ON_USER_DISCONNECTED,userId,0,0);
    }
}

void ConnectionObserver::onConnecting(const agora::rtc::TConnectionInfo &connectionInfo,
											agora::rtc::CONNECTION_CHANGED_REASON_TYPE reason)
{
	std::cout<<"onConnecting: id "<< connectionInfo.id
             <<", channelId "<<connectionInfo.channelId.get()->c_str()
             <<", localUserId "<<connectionInfo.localUserId.get()->c_str()
             <<", reason "<<reason<<std::endl;

    std::string userId=connectionInfo.localUserId.get()->c_str();
    if(_parent!=nullptr){
         _parent->addEvent(AGORA_EVENT_ON_CONNECTING,userId,0,0);
    }
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

    std::string userId=connectionInfo.localUserId.get()->c_str();
    if(_parent!=nullptr){
         _parent->addEvent(AGORA_EVENT_ON_CONNECTION_LOST,userId,0,0);
    }
}

void ConnectionObserver::onUplinkNetworkInfoUpdated(const agora::rtc::UplinkNetworkInfo &info)
{
    //TODO:
	/*AG_LOG(INFO, "onBandwidthEstimationUpdated: video_encoder_target_bitrate_bps %d\n",
		   info.video_encoder_target_bitrate_bps);*/

    std::cout<<"onUplinkNetworkInfoUpdated\n";

    if(_parent!=nullptr){
         _parent->addEvent(AGORA_EVENT_ON_UPLINK_NETWORK_INFO_UPDATED,"",0,0);
    }
}

void ConnectionObserver::onUserJoined(agora::user_id_t userId)
{

    std::cout <<"onUserJoined: userId "<<userId<<std::endl;

    if(_onUserStateChanged!=nullptr){
        _onUserStateChanged(userId, USER_JOIN);
    }

    if(_parent!=nullptr){
         _parent->addEvent(AGORA_EVENT_ON_USER_CONNECTED,userId,0,0);
    }
}

void ConnectionObserver::onUserLeft(agora::user_id_t userId,
										  agora::rtc::USER_OFFLINE_REASON_TYPE reason)
{

   std::cout<<"onUserLeft: "<<"userId: "<<userId<<std::endl;

   if(_onUserStateChanged!=nullptr){
        _onUserStateChanged(userId, USER_LEAVE);
   }

   if(_parent!=nullptr){
         _parent->addEvent(AGORA_EVENT_ON_USER_DISCONNECTED,userId,0,0);
    }
}
