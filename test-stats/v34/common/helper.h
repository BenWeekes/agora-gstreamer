#include <chrono>
#include <string>
struct PacerInfo {
  int sendTimes;
  int sendIntervalInMs;
  std::chrono::steady_clock::time_point startTime;
};

struct DataStreamResult {
  bool check_result = true;
  int received_msg_count = 0;
  int received_total_bytes = 0;
};

uint64_t now_ms_t();

void waitBeforeNextSend(PacerInfo& pacer);

std::string getCurrentSystemTimeChrono();

void spendTimeInfoStatistics(uint64_t T1, uint64_t T2, int statistics_count);
