#define DEBUG_MODE 1

#include <fstream>
#include <chrono>
#include <sys/time.h>
#include <mutex>

std::mutex g_mutex;

#define MAX_LOG_FILE_SIZE 1024*1024*250 //20 MB
#define LOG_FILE_NAME    "/tmp/agora.log"

void logMessage(const std::string& message){

#ifdef DEBUG_MODE

  char buffer[30];
  struct timeval tv;

  time_t curtime;

  gettimeofday(&tv, NULL); 
  curtime=tv.tv_sec;
  strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));
  int ms=(int)((tv.tv_usec)/1000);

  char fullTime[40];
  snprintf(fullTime, 40, "%s%3d",buffer, ms);

  std::lock_guard<std::mutex> guard(g_mutex);
  std::ofstream file(LOG_FILE_NAME,std::ios::app);
  if(!file.is_open()){
     return;
  }
  file<<fullTime<<": "<<message<<std::endl;
  file.close();

#endif	
  
}


void CheckAndRollLogFile(){

  #ifdef DEBUG_MODE

  std::lock_guard<std::mutex> guard(g_mutex);

  std::ofstream logFile(LOG_FILE_NAME,std::ios::app);
  if(!logFile.is_open()){
     return;
  }

  //TODO
  auto currentFileSize = logFile.tellp();
  logFile.close();

  if(currentFileSize>MAX_LOG_FILE_SIZE){

    char oldFileName[256];

    char buffer[30];
    struct timeval tv;
    time_t curtime;
    gettimeofday(&tv, NULL); 
    curtime=tv.tv_sec;
    strftime(buffer,30,"%m-%d-%Y-%I_%M_%S",localtime(&curtime));
    snprintf(oldFileName, 256, "/tmp/agora-%s.log", buffer);

    std::string cmd=std::string("cp ")+std::string(LOG_FILE_NAME)+" "+oldFileName;
    system(cmd.c_str());

    //truncate previous file
    std::ofstream logFile2(LOG_FILE_NAME);
    if(logFile2.is_open()){
       logFile2.close();
     }
  }


  #endif

}

