#include "helper_opus_parser.h"

#include <opusfile.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstring>
#include <vector>

#include "common/log.h"

struct decode_context {
  int current_packet{0};
  int total_length{0};

  int nsamples{0};
  int nchannels;
  int format;

  unsigned char packet[960 * 2 * 2]{0};
  long bytes{0};
  long b_o_s{0};
  long e_o_s{0};

  ogg_int64_t granulepos;
  ogg_int64_t packetno;
};

class OggOpusFileParser {
 public:
  explicit OggOpusFileParser(const char* filepath);
  virtual ~OggOpusFileParser();

 public:
  // AudioFileParser
  bool open();
  bool hasNext();

  void getNext(char* buffer, int* length);

  agora::rtc::AUDIO_CODEC_TYPE getCodecType();
  int getSampleRateHz();
  int getNumberOfChannels();
  int reset();

 private:
  void loadMetaInfo(OggOpusFile* oggOpusFile);

 private:
  char* oggOpusFilePath_;
  OggOpusFile* oggOpusFile_;
  int sampleRateHz_;
  int numberOfChannels_;
  decode_context decode_context_;
  bool eof;
};

int op_decode_cb(void* ctx, OpusMSDecoder* decoder, void* pcm, const ogg_packet* op, int nsamples,
                 int nchannels, int format, int li) {
  struct decode_context* context = (struct decode_context*)ctx;
  context->nsamples = nsamples;
  context->nchannels = nchannels;
  context->format = format;

  context->b_o_s = op->b_o_s;
  context->e_o_s = op->e_o_s;
  context->bytes = op->bytes;
  context->granulepos = op->granulepos;
  context->packetno = op->packetno;
  memcpy(context->packet, op->packet, op->bytes);

  context->total_length += op->bytes;
  ++context->current_packet;

  (void)pcm;
  (void)decoder;
  (void)li;

  return 0;
}

OggOpusFileParser::OggOpusFileParser(const char* filepath)
    : oggOpusFilePath_(strdup(filepath)),
      oggOpusFile_(nullptr),
      sampleRateHz_(48000),
      numberOfChannels_(0),
      eof(false) {}

OggOpusFileParser::~OggOpusFileParser() {
  if (oggOpusFile_) {
    op_free(oggOpusFile_);
  }
  free(static_cast<void*>(oggOpusFilePath_));
}

void OggOpusFileParser::loadMetaInfo(OggOpusFile* oggOpusFile) {
  const OpusHead* head = op_head(oggOpusFile, -1);
  numberOfChannels_ = head->channel_count;
  sampleRateHz_ = head->input_sample_rate;
  if (op_seekable(oggOpusFile)) {
  }
}

bool OggOpusFileParser::open() {
  int ret = 0;
  oggOpusFile_ = op_open_file(oggOpusFilePath_, &ret);
  if (oggOpusFile_) {
    op_set_decode_callback(oggOpusFile_, op_decode_cb, &decode_context_);
    loadMetaInfo(oggOpusFile_);
  }
  return oggOpusFile_ != nullptr;
}

int OggOpusFileParser::reset() {
  int ret = 0;
  if (oggOpusFile_) {
    ret = op_pcm_seek(oggOpusFile_, 0);
    eof = false;
  }
  return ret;
}

bool OggOpusFileParser::hasNext() {
  opus_int16 pcm[120 * 48 * 2];
  int ret = op_read_stereo(oggOpusFile_, pcm, sizeof(pcm) / sizeof(*pcm));
  return ret >= 0 && !eof;
}

void OggOpusFileParser::getNext(char* buffer, int* length) {
  if (*length > decode_context_.bytes) {
    memcpy(buffer, decode_context_.packet, decode_context_.bytes);
    *length = decode_context_.bytes;

    if (decode_context_.e_o_s) {
      eof = true;
    }
  }
}

agora::rtc::AUDIO_CODEC_TYPE OggOpusFileParser::getCodecType() {
  return agora::rtc::AUDIO_CODEC_OPUS;
}

int OggOpusFileParser::getSampleRateHz() {
  // return sampleRateHz_; // All Opus audio is coded at 48 kHz, and should also be decoded at 48
  // kHz
  return 48000;
}

int OggOpusFileParser::getNumberOfChannels() { return numberOfChannels_; }

HelperOpusFileParser::HelperOpusFileParser(const char* filepath) : file_path(filepath) {}

HelperOpusFileParser::~HelperOpusFileParser() = default;

bool HelperOpusFileParser::initialize() {
  file_parser_.reset(new OggOpusFileParser(file_path.c_str()));

  if (!file_parser_ || !file_parser_->open()) {
    printf("Open opus file %s failed\n", file_path.c_str());
    return false;
  }
  printf("Open opus file %s successfully\n", file_path.c_str());
  return true;
}

std::unique_ptr<HelperAudioFrame> HelperOpusFileParser::getAudioFrame(int frameSizeDuration) {
  std::unique_ptr<HelperAudioFrame> audioFrame = nullptr;
  static uint8_t databuf[8192] = {0};
  static int length = 8192;
  static int bytesnum = 0;
  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels = file_parser_->getNumberOfChannels();
  audioFrameInfo.sampleRateHz = file_parser_->getSampleRateHz();
  audioFrameInfo.codec = file_parser_->getCodecType();
  // calculate Opus frame size
  audioFrameInfo.samplesPerChannel = file_parser_->getSampleRateHz() * frameSizeDuration / 1000;

  if (!file_parser_->hasNext()) {
    file_parser_->reset();
    return nullptr;
  }
  length = 8192;
  file_parser_->getNext(reinterpret_cast<char*>(databuf), &length);
  if (length > 0) {
    std::unique_ptr<char[]> buffer2(new char[length]);
    memcpy(buffer2.get(), databuf, length);
    audioFrame.reset(new HelperAudioFrame{audioFrameInfo, std::move(buffer2), length});

    bytesnum += length;
  }
  return audioFrame;
}
