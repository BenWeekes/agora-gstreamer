#include "uidtofile.h"
#include <fstream>
#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "utilities.h"

#include <iostream>

  UidToFile::UidToFile():
  _fileName("/tmp/uid.tmp"),
  _lastCheckTime(Now()),
  _lastUserId(""){

     struct passwd *pw = getpwuid(getuid());
     const char *homedir = pw->pw_dir;
      _fileName=std::string(homedir)+"/uid.tmp";
  }

  void UidToFile::writeUid(const std::string& uid){

      std::ofstream file(_fileName);
      if(file.is_open()==false) return;

       file<<uid;
       file.close();

       std::cout<<"user id:  "<<uid<<" is written to "<<_fileName<<std::endl;
  }
  std::string UidToFile::readUid(){

    /*std::string uid;
    std::ifstream file(_fileName);
    if(file.is_open()==false) return "";

    std::getline(file, uid);

    file.close();

    return uid;*/

    return "";
  }

std::string  UidToFile::checkAndReadUid(){

    auto timeDiff=GetTimeDiff(_lastCheckTime, Now());

    if(timeDiff<1000) return "";

    auto uid=readUid();

     _lastCheckTime=Now();

     if(uid!=_lastUserId){
       _lastUserId=uid;
       return _lastUserId;
     }
     
    return "";
}

std::string ReadCurrentUid(){

   UidToFile uidfile;

   auto uid=uidfile.readUid();

   return uid;
}