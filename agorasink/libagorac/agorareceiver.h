#ifndef __AGORARECEIVERUSER_H__
#define __AGORARECEIVERUSER_H__

#include <memory>

class AgoraReceiverUser;
std::shared_ptr<AgoraReceiverUser> create_receive_user(const std::string& _appId,
                                                       const std::string& _channel,
                                                       const std::string& _userId);

  size_t get_next_video_frame(std::shared_ptr<AgoraReceiverUser> receiver, 
               unsigned char* data, size_t max_buffer_size, int* is_key_frame);                                                     


#endif
