#include <string>

#include "AgoraBase.h"

class OggOpusFileParser;

struct HelperAudioFrame {
  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  std::unique_ptr<char[]> buffer;
  int bufferLen;
};

class HelperOpusFileParser {
 public:
  HelperOpusFileParser(const char* filepath);
  ~HelperOpusFileParser();
  bool initialize();
  std::unique_ptr<HelperAudioFrame> getAudioFrame(int frameSizeDuration);

 private:
  std::string file_path;
  std::unique_ptr<OggOpusFileParser> file_parser_;
  int64_t sent_audio_frames_{0};
};
