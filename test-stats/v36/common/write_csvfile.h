#ifndef _WRITE_CSV_FILE_H_
#define _WRITE_CSV_FILE_H_

#include <string>
#include <fstream>
#include <memory>

struct delayTimeBaseInfo
{
    int round_count_;
    int spendTime_;
    int data_length;
};

class WriteCSVFileHandle
{
  public:
    WriteCSVFileHandle(const std::string filepath="./delay_csvfile.csv"):filepath_(filepath){}
    ~WriteCSVFileHandle();
    void open();
    void write_csvfile(const struct delayTimeBaseInfo &delay_info);
    void close();
  private:
    std::string filepath_;
    std::ofstream csvfile_;
};



#endif