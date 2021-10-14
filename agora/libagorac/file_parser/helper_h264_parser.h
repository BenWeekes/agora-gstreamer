#include <memory>
#include <string>

struct HelperH264Frame {
  bool isKeyFrame;
  std::unique_ptr<uint8_t[]> buffer;
  int bufferLen;
};

class HelperH264FileParser {
 public:
  HelperH264FileParser(const char* filepath);
  ~HelperH264FileParser();

  std::unique_ptr<HelperH264Frame> getH264Frame();
  bool initialize();
  void setFileParseRestart();

 private:
  void _getH264Frame(std::unique_ptr<HelperH264Frame>& h264Frame, bool is_key_frame,
                     int frame_start, int frame_end);

  std::string file_path_;
  int data_offset_;
  int data_size_;
  uint8_t* data_buffer_;
};
