#include "uidtofile.h"
#include <fstream>
#include<stdio.h>

#include "utilities.h"

  UidToFile::UidToFile():
  _fileName("/tmp/uid.tmp"),
  _lastCheckTime(Now()){

  }

  void UidToFile::writeUid(const std::string& uid){

      std::ofstream file(_fileName);
      if(file.is_open()==false) return;

       file<<uid;
       file.close();
  }
  std::string UidToFile::readUid(){

    std::string uid;
    std::ifstream file(_fileName);
    if(file.is_open()==false) return "";

    file>>uid;

    file.close();

    return uid;
  }

  void UidToFile::removeUid(){

      remove(_fileName.c_str());
  }

std::string  UidToFile::checkAndReadUid(){

    auto timeDiff=GetTimeDiff(_lastCheckTime, Now());

    if(timeDiff<1000) return "";

    auto uid=readUid();

    removeUid();

     _lastCheckTime=Now();
    return uid;
}

std::string ReadCurrentUid(){

   UidToFile uidfile;

   auto uid=uidfile.readUid();

   uidfile.removeUid();

   return uid;
}