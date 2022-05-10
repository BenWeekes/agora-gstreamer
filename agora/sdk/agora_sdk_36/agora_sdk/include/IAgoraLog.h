//
//  Agora Media SDK
//
//  Copyright (c) 2015 Agora IO. All rights reserved.
//
#pragma once

#include <cstdlib>
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
#include <cstdint>
#endif

#ifndef OPTIONAL_ENUM_CLASS
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
#define OPTIONAL_ENUM_CLASS enum class
#else
#define OPTIONAL_ENUM_CLASS enum
#endif
#endif

#ifndef OPTIONAL_LOG_LEVEL_SPECIFIER
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
#define OPTIONAL_LOG_LEVEL_SPECIFIER LOG_LEVEL::
#else
#define OPTIONAL_LOG_LEVEL_SPECIFIER
#endif
#endif

namespace agora {
namespace commons {
/**
 * The output log level of the SDK.
 */
OPTIONAL_ENUM_CLASS LOG_LEVEL {
  LOG_LEVEL_NONE = 0x0000,
  /** 0x0001: (Default) Output logs of the FATAL, ERROR, WARN and INFO level. We recommend setting your log filter as this level.
   */
  LOG_LEVEL_INFO = 0x0001,
  /** 0x0002: Output logs of the FATAL, ERROR and WARN level.
   */
  LOG_LEVEL_WARN = 0x0002,
  /** 0x0004: Output logs of the FATAL and ERROR level.  */
  LOG_LEVEL_ERROR = 0x0004,
  /** 0x0008: Output logs of the FATAL level.  */
  LOG_LEVEL_FATAL = 0x0008,
};
/// @cond
/*
The SDK uses ILogWriter class Write interface to write logs as application
The application inherits the methods Write() to implentation their own  log writ

Write has default implementation, it writes logs to files.
Application can use setLogFile() to change file location, see description of set
*/
class ILogWriter {
 public:
  /** user defined log Write function
  @param log level
  @param log message content
  @param log message length
  @return
   - 0: success
   - <0: failure
  */
  virtual int32_t writeLog(LOG_LEVEL level, const char* message, uint16_t length) = 0;

  virtual ~ILogWriter() {}
};

enum LOG_FILTER_TYPE {
  LOG_FILTER_OFF = 0,
  LOG_FILTER_DEBUG = 0x080f,
  LOG_FILTER_INFO = 0x000f,
  LOG_FILTER_WARN = 0x000e,
  LOG_FILTER_ERROR = 0x000c,
  LOG_FILTER_CRITICAL = 0x0008,
  LOG_FILTER_MASK = 0x80f,
};
/// @endcond

const uint32_t MAX_LOG_SIZE = 20 * 1024 * 1024;  // 20MB
const uint32_t MIN_LOG_SIZE = 128 * 1024;        // 128KB
/** The default log size in KB.
 */
const uint32_t DEFAULT_LOG_SIZE_IN_KB = 1024;

/** The configuration of the log files that the SDK outputs.
 */
struct LogConfig
{
  /** The absolute path of log files.
   *
   * The default file path is `C:\Users\<user_name>\AppData\Local\Agora\<process_name>\agorasdk.log`
   *
   * Ensure that the directory for the log files exists and is writable. You can use this parameter to rename the log files.
   */
  const char* filePath;
  /**
   * The size (KB) of a `agorasdk.log` file. The value range is [128,1024]. The default value is 1,024 KB.
   * If you set `fileSizeInKByte` smaller than 128 KB, the SDK automatically adjusts it to 128 KB; if you set `fileSizeInKByte` greater than 1,024 KB, the SDK automatically adjusts it to 1,024 KB.
   *
   * @note This setting applies to the `agorasdk.log` file only and does not take effect for the `agoraapi.log` file.
   *
   */
  uint32_t fileSizeInKB;
  /** The output log level of the SDK. See \ref agora::commons::LOG_LEVEL "LOG_LEVEL".
   *
   * For example, if you set the log level to WARN, the SDK outputs the logs within levels FATAL, ERROR, and WARN.
   */
  LOG_LEVEL level;

  LogConfig() : filePath(NULL), fileSizeInKB(DEFAULT_LOG_SIZE_IN_KB), level(OPTIONAL_LOG_LEVEL_SPECIFIER LOG_LEVEL_INFO) {}
};
} //namespace commons
} //namespace agora

#undef OPTIONAL_LOG_LEVEL_SPECIFIER