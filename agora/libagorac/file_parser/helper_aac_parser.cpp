#include "helper_aac_parser.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/log.h"

#define ADTS_HEADER_SIZE (7)

typedef struct AACAudioFrame_ {
  uint16_t syncword{0};
  uint8_t id{0};
  uint8_t layer{0};
  uint8_t protection_absent{0};
  uint8_t profile{0};
  uint8_t sampling_frequency_index{0};
  uint8_t private_bit{0};
  uint8_t channel_configuration{0};
  uint8_t original_copy{0};
  uint8_t home{0};

  uint8_t copyrighted_id_bit{0};
  uint8_t copyrighted_id_start{0};
  uint16_t aac_frame_length{0};
  uint16_t adts_buffer_fullness{0};
  uint8_t number_of_raw_data_blocks_in_frame{0};
} AACAudioFrame;

static const uint32_t AacFrameSampleRateMap[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                                 22000, 16000, 12000, 11025, 8000,  7350};

HelperAacFileParser::HelperAacFileParser(const char* filepath)
    : file_path_(filepath), data_buffer_(nullptr), data_offset_(0) {}

HelperAacFileParser::~HelperAacFileParser() = default;

bool HelperAacFileParser::initialize() {
  int fd;
  struct stat sb;
  void* mapped;

  if ((fd = open(file_path_.c_str(), O_RDONLY)) < 0) {
    perror(file_path_.c_str());
    return false;
  }
  AG_LOG(INFO, "Open aac file %s successfully", file_path_.c_str());

  // get the file property
  if ((fstat(fd, &sb)) == -1) {
    perror("fstat");
    close(fd);
    return false;
  }

  // map the file to process address space
  if ((mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*)-1) {
    perror("mmap");
    close(fd);
    return false;
  }
  close(fd);

  data_size_ = sb.st_size;
  data_buffer_ = (uint8_t*)mapped;
  return true;
}

std::unique_ptr<HelperAudioFrame> HelperAacFileParser::getAudioFrame(int frameSizeDuration) {
  std::unique_ptr<HelperAudioFrame> audioFrame = nullptr;
  AACAudioFrame aacframe;

  // Check data offset and rewind to the file start if necessary
  if (data_offset_ + ADTS_HEADER_SIZE > data_size_) {
    data_offset_ = 0;
  }

  // Begin by reading the 7-byte fixed_variable headers
  unsigned char* hdr = &data_buffer_[data_offset_];

  // parse adts_fixed_header()
  aacframe.syncword = (hdr[0] << 4) | (hdr[1] >> 4);
  if (aacframe.syncword != 0xfff) {
    AG_LOG(ERROR, "Invalid AAC syncword 0x%x", aacframe.syncword);
    return audioFrame;
  }
  aacframe.id = (hdr[1] >> 3) & 0x01;
  aacframe.layer = (hdr[1] >> 1) & 0x03;
  aacframe.protection_absent = hdr[1] & 0x01;
  aacframe.profile = (hdr[2] >> 6) & 0x03;
  aacframe.sampling_frequency_index = (hdr[2] >> 2) & 0x0f;
  aacframe.private_bit = (hdr[2] >> 1) & 0x01;
  aacframe.channel_configuration = ((hdr[2] & 0x1) << 2) | (hdr[3] >> 6);
  aacframe.original_copy = (hdr[3] >> 5) & 0x01;
  aacframe.home = (hdr[3] >> 4) & 0x01;
  // parse adts_variable_header()
  aacframe.copyrighted_id_bit = (hdr[3] >> 3) & 0x01;
  aacframe.copyrighted_id_start = (hdr[3] >> 2) & 0x01;
  aacframe.aac_frame_length = ((hdr[3] & 0x3) << 11) | (hdr[4] << 3) | (hdr[5] >> 5);
  aacframe.adts_buffer_fullness = ((hdr[5] & 0x1f) << 6) | (hdr[6] >> 2);
  aacframe.number_of_raw_data_blocks_in_frame = hdr[6] & 0x03;

  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels = 1;
  audioFrameInfo.sampleRateHz = AacFrameSampleRateMap[aacframe.sampling_frequency_index];
  audioFrameInfo.codec = agora::rtc::AUDIO_CODEC_AACLC;
  // calculate audio frame size per channel
  audioFrameInfo.samplesPerChannel = audioFrameInfo.sampleRateHz * frameSizeDuration / 1000;

  audioFrame.reset(
      new HelperAudioFrame{audioFrameInfo, &data_buffer_[data_offset_], aacframe.aac_frame_length});

  data_offset_ += aacframe.aac_frame_length;
  return audioFrame;
}