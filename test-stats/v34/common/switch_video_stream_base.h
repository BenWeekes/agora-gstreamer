#ifndef _SWITCH_VIDEO_STREAM_BASE_H_
#define _SWITCH_VIDEO_STREAM_BASE_H_

class SwitchVideoStreamSourceBase {
 public:
  SwitchVideoStreamSourceBase() = default;

  static void exchange_cur_next_flag() {
    flag_cur = 1 - flag_cur;
    isSwitch = true;
  }
  static int get_cur_flag() { return flag_cur; }
  static void reset_Switch() { isSwitch = false; }
  static bool get_switch_status() { return isSwitch; }

 private:
  static int flag_cur;
  static bool isSwitch;
};

int SwitchVideoStreamSourceBase::flag_cur = 0;
bool SwitchVideoStreamSourceBase::isSwitch = false;

#endif