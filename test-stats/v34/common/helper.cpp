#include "helper.h"

#include <thread>

void waitBeforeNextSend(PacerInfo& pacer) {
  auto sendFrameEndTime = std::chrono::steady_clock::now();
  int nextDurationInMs = (++pacer.sendTimes * pacer.sendIntervalInMs);
  int waitIntervalInMs = nextDurationInMs - std::chrono::duration_cast<std::chrono::milliseconds>(
                                                sendFrameEndTime - pacer.startTime)
                                                .count();
  if (waitIntervalInMs > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitIntervalInMs));
  }
}


std::string getCurrentSystemTimeChrono() {
  auto now = std::chrono::system_clock::now();
  uint64_t dis_millseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count() -
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
              .count() *
          1000;
  time_t tt = std::chrono::system_clock::to_time_t(now);
  auto time_tm = localtime(&tt);
  char strTime[25] = {0};
  sprintf(strTime, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
          time_tm->tm_year + 1900, time_tm->tm_mon + 1, time_tm->tm_mday,
          time_tm->tm_hour, time_tm->tm_min, time_tm->tm_sec,
          (int)dis_millseconds);
  return std::string(strTime);
}

uint64_t now_ms_t() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

void spendTimeInfoStatistics(uint64_t T1, uint64_t T2,int statistics_count)
{
  static int sumSpendTime;
  static int round_count;
  static int min_spend_time_per_statistics=-1;
  static int max_spend_time_per_statistics;
  int spend_time = T2 - T1;
  sumSpendTime += spend_time;
  max_spend_time_per_statistics =
          (spend_time > max_spend_time_per_statistics)
              ? spend_time: max_spend_time_per_statistics;
  if (min_spend_time_per_statistics == -1) {
    min_spend_time_per_statistics = spend_time;
  } else {
    min_spend_time_per_statistics =(spend_time < min_spend_time_per_statistics)
                ? spend_time
                : min_spend_time_per_statistics;
  }

  if (round_count % statistics_count == 0) {
    double averageSpendTime =
        (static_cast<double>(sumSpendTime) / statistics_count);
    printf("the averge spend time per %d times is: %f\n ",
            statistics_count, averageSpendTime);
    printf("the max spend time is:%d. the min spend time is:%d\n",
           max_spend_time_per_statistics, min_spend_time_per_statistics);
    max_spend_time_per_statistics = 0;
    min_spend_time_per_statistics = 0;
    sumSpendTime = 0;
    round_count = 0;
  }
  round_count++;
}
