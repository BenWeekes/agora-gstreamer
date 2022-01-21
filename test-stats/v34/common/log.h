#include <cstdio>

enum {ERROR=-1, INFO=0, WARNING, FATAL};

#define AG_LOG(level, format, ...) ((void)fprintf(stderr, "[ APP_LOG_" #level " ] " format "\n", ##__VA_ARGS__)) 
