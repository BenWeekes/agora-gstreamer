#ifndef _UID_TO_FILE_H_
#define _UID_TO_FILE_H_

#include <string>

#include "../agoratype.h"

class UidToFile{

public:

  UidToFile();
  void writeUid(const std::string& uid);
  std::string readUid();

  std::string  checkAndReadUid();

private:
   std::string  _fileName;
   TimePoint   _lastCheckTime;

   std::string  _lastUserId;
};

std::string ReadCurrentUid();

#endif

