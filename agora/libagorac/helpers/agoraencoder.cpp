#include "agoraencoder.h"
#include "agoralog.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "utilities.h"

AgoraEncoder::AgoraEncoder( const uint16_t& targetWidth,
                      const uint16_t& targetHeight,const uint32_t& bitRate,
                      const uint16_t& fps):
m_srcWidth(targetWidth),
m_srcHeight(targetHeight),
m_targetWidth(targetWidth),
m_targetHeight(targetHeight),
m_bitrate(bitRate),
m_scaleContext(nullptr),
_fps(fps){

}

AgoraEncoder::~AgoraEncoder(){
  /*if(m_avCodec){
     
     const int strideCount=3;
     for (int i = 0; i < strideCount; i++) {
        m_avInputFrame.img.plane[i]    = 0;
     } 
     x264_picture_clean(&m_avInputFrame);
     
     x264_encoder_close(m_avCodec);
     sws_freeContext(m_scaleContext);
   } */    
}
bool AgoraEncoder::init(){

  /*x264_param_t param;
  initParams(param);

  //create the encoder
  m_avCodec = x264_encoder_open( &param );
  if( m_avCodec==nullptr){
     logMessage("Cannot create X264 encoder.");
     return false;
  }
  
  //create a frame: no need to allocate data for the image, since we will just point it to the scaled frame
  x264_picture_alloc(&m_avInputFrame, X264_CSP_I420, 0, 0); 

  //create scaling context
  createVideoScaleContext();*/

  return true;
}

bool AgoraEncoder::encode(AVFrame* frame,uint8_t* out, uint32_t& outSize, bool requestKeyFrame){

  /*int nalCount;
  x264_nal_t* nals;
  int encodedBytes=0;

  if(m_avCodec==nullptr) return false;

  //dyamically read input from frame res
  if(frame->width!=m_srcWidth || frame->height!=m_srcHeight){
     onResolutionChange(frame->width, frame->height);
  }

  //scale video
  auto scaledFrame=scaleFrame(frame);
  if(scaledFrame==nullptr){
     return false;
  }

  //logMessage("scaled frame: "+std::string(m_avOutputFrame.width)+"X"+std::string(m_avOutputFrame.height));

  //directly copy data from decoded frame
  const int strideCount=3;
  for (int i = 0; i < strideCount; i++) {
      m_avInputFrame.img.plane[i]    = scaledFrame->data[i];
      m_avInputFrame.img.i_stride[i] = scaledFrame->linesize[i];
   } 

  m_avInputFrame.i_type =X264_TYPE_AUTO;
  if(requestKeyFrame){
     m_avInputFrame.i_type =X264_TYPE_IDR;
   }
    
  auto bytes = x264_encoder_encode(m_avCodec, &nals, &nalCount, &m_avInputFrame, &m_avOutputFrame);
  if (bytes >= 0){
    outSize=handlePostEncoding(out, nalCount, nals,0);
  }


  //flush delayed frames
  while( x264_encoder_delayed_frames(m_avCodec) ){
     encodedBytes = x264_encoder_encode( m_avCodec, &nals, &nalCount, nullptr, &m_avOutputFrame );
     outSize=handlePostEncoding(out, nalCount, nals,outSize);
  }

  //logMessage("ecoded frame: "+std::string(m_avOutputFrame.width)+"X"+std::string(m_avOutputFrame.height));

 //free up scaled frame
 av_freep(&scaledFrame->data[0]);
 av_frame_free(&scaledFrame);
  */

  return true;
}

static void  X264LogFunc( void *, int i_level, const char *psz, va_list ){
   //TODO: handle x264 log here if needed
}

void AgoraEncoder::initParams(x264_param_t& param){

  /*const int PACKET_SIZE=60000;

  x264_param_default_preset(&param, "veryfast", "zerolatency");
  param.i_csp = X264_CSP_I420;
  param.i_width  = m_targetWidth;
  param.i_height = m_targetHeight;
    
  param.b_repeat_headers = 1;
  //param.i_slice_max_size=PACKET_SIZE;
  param.b_annexb = 1;

  param.i_slice_max_size = 0;
  param.i_slice_max_mbs = 0;
  param.i_slice_count = 0;

  param.i_fps_num = _fps*1000;
  param.i_fps_den = 1000;

  param.i_keyint_max = _fps*1000;
  param.b_intra_refresh = 1000;

  param.i_threads = 1;

  //variable rate control
  param.rc.i_rc_method =  X264_RC_ABR;

  //variable rate control
  param.rc.i_vbv_buffer_size = param.rc.i_vbv_max_bitrate =param.rc.i_bitrate=m_bitrate/1000.0;

  //qmin and qmax
  if(_qMax>0){

     param.rc.i_qp_max=_qMax;
  }

  if(_qMin>0){
     param.rc.i_qp_min=_qMin;
  }

  //TODO: we may change this to X264_LOG_DEBUG when debugging issues
  param.i_log_level = X264_LOG_ERROR;
  param.pf_log=X264LogFunc;

  logMessage("**** qMin="+std::to_string(_qMin)+", qMax="+std::to_string(_qMax)+"****");

  param.i_level_idc = 31;
  x264_param_apply_profile(&param, "baseline");*/
}

int AgoraEncoder::handlePostEncoding(uint8_t* out, 
                                     const int& nalCount,
                                     x264_nal_t* nals,
                                     const uint32_t& offset){
	

  /*if(m_avOutputFrame.i_type ==X264_TYPE_IDR){
     logMessage("key frame is produced");
  }*/
  
  /*int totalBytes=0;
  for(int i=0;i<nalCount;i++){

    int iNalSize = nals[i].i_payload;

    memcpy(&out[totalBytes], nals[i].p_payload, iNalSize);	  
    totalBytes +=iNalSize;
  }

  return totalBytes;*/
  return 0;
}

bool AgoraEncoder::createVideoScaleContext(){

 /* AVPixelFormat sourceFormat=AV_PIX_FMT_YUV420P;
  AVPixelFormat targetFormat=AV_PIX_FMT_YUV420P;
  
  if (m_scaleContext!=nullptr) sws_freeContext(m_scaleContext);	
   
   m_scaleContext=sws_getContext(m_srcWidth, m_srcHeight,sourceFormat, 
                                   m_targetWidth, m_targetHeight,targetFormat,
                                   SWS_POINT, 0, 0, 0);
 
   if(m_scaleContext==nullptr){

      logMessage("cannot create scale context for video encoder");
      return false;
   }*/

   return true;
}
void AgoraEncoder::onResolutionChange(const uint16_t& newWidth, const uint16_t& newHeight){

  m_srcWidth=newWidth;
  m_srcHeight=newHeight;

  createVideoScaleContext();

  logMessage("Agora encoder has been reconfigured!");
}

AVFrame* AgoraEncoder::scaleFrame(AVFrame* inFrame){

  /*AVFrame *outFrame;

  outFrame=av_frame_alloc();
  if(outFrame==nullptr){
     return nullptr;
  }

  outFrame->format = AV_PIX_FMT_YUV420P;
  outFrame->width = m_targetWidth;
  outFrame->height = m_targetHeight;

  int ret = av_image_alloc(outFrame->data, outFrame->linesize,outFrame->width,outFrame->height,AV_PIX_FMT_YUV420P, 1);
  if (ret < 0) {
     logMessage("Could not allocate raw picture buffer\n");
     return nullptr;
  }

  ret=sws_scale(m_scaleContext, inFrame->data, inFrame->linesize, 0, m_srcHeight, outFrame->data, outFrame->linesize);
  if(ret<=0){
     logMessage("Unexpected error while converting a video frame: "+std::to_string(ret)+"\n");
     return nullptr;
  }

  return outFrame;*/
  return nullptr;
}

