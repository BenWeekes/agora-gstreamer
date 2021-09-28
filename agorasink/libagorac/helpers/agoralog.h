#ifndef _AGORA_LOG_H_
#define _AGORA_LOG_H_

#include <string>

//log a debug message to a file, only in debug mode
void logMessage(const std::string& message);

void CheckAndRollLogFile();

#endif
