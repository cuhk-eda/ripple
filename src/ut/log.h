#ifndef _UT_LOG_H_
#define _UT_LOG_H_

///////////////LOGGING FUNCTIONS///////////////////////
//--log type
#define LOG_DEBUG       1       //0
#define LOG_VERBOSE     2       //1
#define LOG_INFO        4       //2
#define LOG_NOTICE      8       //3
#define LOG_WARN        16      //4
#define LOG_ERROR       32      //5
#define LOG_FATAL       64      //6
#define LOG_OK          128     //7
#define LOG_RESERVED7   256     //8
#define LOG_RESERVED6   1024    //9
#define LOG_RESERVED5   2048    //10
#define LOG_RESERVED4   4096    //12
#define LOG_RESERVED3   8192    //13
#define LOG_RESERVED2   16384   //14
#define LOG_RESERVED1   32768   //15

#define LOG_ALL         65535   //16-bit

//--special log type
#define LOG_PROMPT      65536   //16

#define LOG_FAIL    LOG_ERROR | LOG_FATAL
#define LOG_NORMAL  LOG_ALL ^ LOG_DEBUG ^ LOG_VERBOSE

#define LOG_LEVEL_DEFAULT LOG_ALL

#define LOG_PREFIX "[%8.3lf]%c "
#define LOG_SUFFIX "\n"

void init_log(int log_level = LOG_LEVEL_DEFAULT);
void printlog(int level, const char *format, ...);
void print_prefix(int level);
void print_suffix();

#endif

