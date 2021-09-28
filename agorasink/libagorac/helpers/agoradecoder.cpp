#include "agoradecoder.h"
#include "agoralog.h"

AgoraDecoder::AgoraDecoder(){

}
AgoraDecoder::~AgoraDecoder(){

  avcodec_close(m_avContext);

  if (m_avContext != nullptr){
    avcodec_free_context(&m_avContext);
  }

  if (m_avOutFrame != nullptr) {
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54, 0, 0)
    av_free(m_avOutFrame);
#elif LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 0, 0)
    avcodec_free_frame(&m_avOutFrame);
#else
    av_frame_free(&m_avOutFrame);
#endif
  }
}

bool AgoraDecoder::init(){

  avcodec_register_all();
  if ((m_avCodec = avcodec_find_decoder(AV_CODEC_ID_H264)) == nullptr) {

    logMessage("Agora video decoder: H264 Codec not found!");
    return false;
  }

  av_init_packet(&m_inputPacket);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 0, 0)
  m_avContext = avcodec_alloc_context2(m_avCodec->type);
#else
  m_avContext = avcodec_alloc_context3(m_avCodec);
#endif

  if (m_avContext == nullptr) {
     logMessage("Agora video decoder: cannot create context!");
     return false;
  }

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 0, 0)
  m_avContext->error_recognition = FF_ER_AGGRESSIVE;
#endif

  m_avContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  m_avContext->workaround_bugs = FF_BUG_AUTODETECT;

#ifdef FF_IDCT_H264
      m_avContext->idct_algo = FF_IDCT_H264;
#else
      m_avContext->idct_algo = FF_IDCT_AUTO;
#endif
      m_avContext->flags2 |= AV_CODEC_FLAG2_DROP_FRAME_TIMECODE |
                           AV_CODEC_FLAG2_CHUNKS;


  if (avcodec_open2(m_avContext, m_avCodec, NULL) < 0) {
     logMessage("Agora video decoder: cannot open decoder!");
     return false;
  }

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 0, 0)
  m_avOutFrame = avcodec_alloc_frame();
#else
  m_avOutFrame = av_frame_alloc();
#endif
  
  if(m_avOutFrame==nullptr){
    logMessage("Agora video decoder: cannot allocate a frame!");
    return false;
  }

  m_avOutFrame->format = m_avContext->pix_fmt = AV_PIX_FMT_YUV420P;
  m_avOutFrame->quality = -1;

  logMessage("Agora video decoder: initialized correctly");

  return true;
}

bool AgoraDecoder::decode(const uint8_t* in, const uint32_t& inSize){

  //logMessage("start decoding a frame");

  m_inputPacket.data = ( uint8_t*)in;
  m_inputPacket.size = inSize;

  auto ret=avcodec_send_packet(m_avContext, &m_inputPacket);
  if(ret!=0){
     logMessage("Agora video decoder: failed");
     return false;
  }

  ret=avcodec_receive_frame(m_avContext, m_avOutFrame);
  if(ret!=0){
    logMessage("Agora video decoder: failed");
    return false;
  }

  //logMessage("end  decoding a frame: "+std::to_string(m_avOutFrame->width)+"x"+std::to_string(m_avOutFrame->height));

  return true;
}

//TODO: we do not copy in this version, we pass output frame directly to the encoder
uint32_t  AgoraDecoder::copyDeocodedFrame(AVFrame *frame, uint8_t* buffer){

  uint32_t pitchY = frame->linesize[0];
  uint32_t pitchU = frame->linesize[1];
  uint32_t pitchV = frame->linesize[2];

  uint8_t *avY = frame->data[0];
  uint8_t *avU = frame->data[1];
  uint8_t *avV = frame->data[2];


  uint32_t totalBytes=0;
  for (int i = 0; i < frame->height; i++) {

     memcpy(&buffer[totalBytes],  avY, frame->width);
     avY += pitchY;
     totalBytes +=frame->width;
  }


  for (int i = 0; i < frame->height/2; i++) {
     memcpy(&buffer[totalBytes],  avU, frame->width/2);
     avU += pitchU;
     totalBytes +=frame->width/2;
  }

  for (int i = 0; i < frame->height/2; i++) {

    memcpy(&buffer[totalBytes],  avV, frame->width/2);
    avV += pitchV;
    totalBytes +=frame->width/2;
  }


  return totalBytes;
}
