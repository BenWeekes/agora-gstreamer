#ifndef _AGORA_DECODER_H_
#define _AGORA_DECODER_H_

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavutil/mem.h"
}

//decode h264
class AgoraDecoder{
public:

  AgoraDecoder();

  ~AgoraDecoder();

  //initialize the encoder params, should be called before any decoding
  bool init();

  bool decode(const uint8_t* in, const uint32_t& inSize);

   AVFrame* getFrame(){return m_avOutFrame;}
protected:

 uint32_t copyDeocodedFrame(AVFrame *frame, uint8_t* buffer);

private:

  AVCodec        *m_avCodec;
  AVCodecContext *m_avContext;

  AVFrame        *m_avOutFrame;
  AVPacket       m_inputPacket;

};

#endif
