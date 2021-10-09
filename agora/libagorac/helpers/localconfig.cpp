
#include "localconfig.h"
#include "agoralog.h"
#include <algorithm>

#include <mutex>

std::mutex g_config_file_mutex;

LocalConfig::LocalConfig():
_useDetailedVideoLog(false),
_useDetailedAudioLog(false),
_useSpeedupLog(false),
_useFpsLog(false),
_initialJbSize(120),       //in ms
_maxJbSize(8000),          //in ms
_timeToIncreaseJbSize(15),  //in seconds
_dumpAudioToFile(false),
_qMin(0),
_qMax(0)
{

}

 bool LocalConfig::loadConfig(const std::string& filePath){

  std::lock_guard<std::mutex> guard(g_config_file_mutex);

  std::ifstream configFile(filePath, std::ios::in);
  if(!configFile.is_open()){
	 logMessage("Not able to open rtmp config file: "+filePath);
     logMessage("Default configs will be used!");
	 return false;
  }

   return readConfig(configFile);
 }

 bool LocalConfig::prepareLine(const std::string& in, std::string& out){
	
  out=in;
  
  //remove comments
  auto ret=out.find("//");
  if(ret!=std::string::npos){
	 out=out.substr(0, ret);
  }
  
  return true;
}


 bool LocalConfig::readConfig(std::ifstream& file){

  char buffer[1024];

  std::string key;
  std::string value;

  while(!file.eof()){
	  
	  file.getline(buffer, 1024);

      std::string line(buffer);
      line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

      prepareLine(line,line);

      //ignore empty lines
      if(line.empty()) continue;

	  if(!getKeyValue(line, key, value)){
		  return false;
	  }

      if(key=="detailed-video-log" && value=="yes"){
         _useDetailedVideoLog=true;
      }
      else if(key=="detailed-audio-log" && value=="yes"){
         _useDetailedAudioLog=true;
      }
      else if(key=="fps-log" && value=="yes"){
         _useFpsLog=true;
      }
      else if(key=="speedup-log" && value=="yes"){
         _useSpeedupLog=true;
      }
      else if(key=="jb-initial-size-ms"){
         _initialJbSize=std::atoi(value.c_str());
      }
      else if(key=="jb-max-size-ms"){
         _maxJbSize=std::atoi(value.c_str());
      }
      else if(key=="Jb-max-doubles-if-emptied-within-seconds"){
         _timeToIncreaseJbSize=std::atoi(value.c_str());
      }
      else if(key=="dump-raw-audio-to-file" && value=="yes"){
         _dumpAudioToFile=true;
      }
      else if(key=="qmin"){
         _qMin=std::atoi(value.c_str());
      }
      else if(key=="qmax"){
         _qMax=std::atoi(value.c_str());
      }  
  }

  //validate user input
  if(_initialJbSize<0 || _initialJbSize>8000){
      _initialJbSize=120;  //default
  }

  return true;
 }

 bool LocalConfig::getKeyValue(const std::string& line,std::string& key, std::string& value){
	
  auto ret=line.find("=");
  if(ret!=std::string::npos){
     key=line.substr(0, ret);
     value=line.substr(ret+1, line.length());
     return true;
  }
	
  return false;
}

 void LocalConfig::print(){
    
   logMessage("Will use the following config for this call: ");

   logMessage("Detailed video log: "+getStringfromBool(_useDetailedVideoLog));
   logMessage("Detailed audio log: "+getStringfromBool(_useDetailedAudioLog));
   logMessage("FPS log: "+getStringfromBool(_useFpsLog));

   logMessage("jb-initial-size-ms: "+std::to_string(_initialJbSize));
   logMessage("jb-max-size-ms: "+std::to_string(_maxJbSize));
   logMessage("Jb-max-doubles-if-emptied-within-seconds: "+std::to_string(_timeToIncreaseJbSize));
 }

 std::string LocalConfig::getStringfromBool(const bool& flag){
    
    std::string ret=flag==true? "yes": "no";

    return ret;
 }