#ifndef _LOCAL_CONFIG_H_
#define _LOCAL_CONFIG_H_

#include <string>
#include <fstream>

class LocalConfig{

 public:

  LocalConfig();

  bool loadConfig(const std::string& filePath);

  bool     useDetailedVideoLog(){return _useDetailedVideoLog;}
  bool     useDetailedAudioLog(){return _useDetailedAudioLog;}
  bool     useFpsLog(){return _useFpsLog;}

  bool     useSpeedupLog(){return _useSpeedupLog;}

  uint16_t  getInitialJbSize(){return _initialJbSize;}
  uint16_t  getMaxJbSize(){return _maxJbSize;}
  uint16_t  getTimeToIncreaseJbSize(){return _timeToIncreaseJbSize;}

  bool      dumpAudioToFile(){return _dumpAudioToFile;}

  int       getQMin(){return _qMin;}
  int       getQMax(){return _qMax;}

  void print();

protected:

  bool getKeyValue(const std::string& line,std::string& key, std::string& value);
  bool readConfig(std::ifstream& file);

  std::string getStringfromBool(const bool& flag);
  bool prepareLine(const std::string& in, std::string& out);

 private:

  bool       _useDetailedVideoLog;
  bool       _useDetailedAudioLog;
  bool       _useSpeedupLog;
  bool       _useFpsLog;

  uint16_t    _initialJbSize;
  uint16_t    _maxJbSize;

  uint16_t    _timeToIncreaseJbSize;

  bool        _dumpAudioToFile;

  int         _qMin;
  int         _qMax;
};

#endif
