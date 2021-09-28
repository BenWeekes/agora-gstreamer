#ifndef _AGORA_ENCODER_H_
#define _AGORA_ENCODER_H_

#include <stdint.h>
#include <x264.h>

class AVFrame;
struct SwsContext;

class AgoraEncoder{

public:

  AgoraEncoder( const uint16_t& targetWidth, 
       const uint16_t& targetHeight,
       const uint32_t& bitRate=300000,
       const uint16_t& fps=15);
  ~AgoraEncoder();

  bool init();

  bool encode(AVFrame* frame,uint8_t* out, uint32_t& outSize, bool requestKeyFrame);

  void setQMin(int qMin){_qMin=qMin;}
  void setQMax(int qMax){_qMax=qMax;}

protected:

  void initParams(x264_param_t& param);
  int handlePostEncoding(uint8_t* out, const int& nalCount,
                         x264_nal_t* nals,const uint32_t& offset);

  //create a scaler to change frame width and height
  bool  createVideoScaleContext();

  //called whenever source resolution is changed
  void onResolutionChange(const uint16_t& newWidth, const uint16_t& newHeight);

  AVFrame* scaleFrame(AVFrame* inFrame);
private:

  x264_t*          m_avCodec;
  x264_picture_t   m_avInputFrame;
  x264_picture_t   m_avOutputFrame;

  uint16_t         m_srcWidth;
  uint16_t         m_srcHeight;

  uint16_t         m_targetWidth;
  uint16_t         m_targetHeight;
  uint32_t         m_bitrate;

  SwsContext*      m_scaleContext;
  uint16_t         _fps;

  int              _qMin;
  int              _qMax;
};

#endif
