#include "utils.h"

///////////////LOGGING FUNCTIONS///////////////////////

unsigned _log_timer = UINT_MAX;
unsigned _log_level = 0;

void init_log(int log_level){
    _log_level = log_level;
    if(_log_timer == UINT_MAX){
        _log_timer = utils::Timer::start();
    }
}
void printlog(int level, const char *format, ...){
    if((level & _log_level) == 0){
        return;
    }
    print_prefix(level);
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    print_suffix();
    fflush(stdout);
}

void print_prefix(int level){
    if(level == LOG_FATAL){
        printf(ESC_BOLD);
        printf(ESC_BLACK);
        printf(ESC_BG_RED);
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), '!');
    }else if(level == LOG_ERROR){
        printf(ESC_BOLD);
        printf(ESC_RED);
        printf(ESC_BG_DEFAULT);
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), 'E');
    }else if(level == LOG_WARN){
        printf(ESC_BOLD);
        printf(ESC_YELLOW);
        printf(ESC_BG_BLACK);
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), 'W');
    }else if(level == LOG_NOTICE){
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), 'N');
    }else if(level == LOG_PROMPT){
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), '>');
    }else if(level == LOG_OK){
        printf(ESC_GREEN);
        printf(ESC_BG_DEFAULT);
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), ' ');
    }else{
        printf(LOG_PREFIX, utils::Timer::time(_log_timer), ' ');
    }
    fflush(stdout);
}

void print_suffix(){
    printf(ESC_RESET);
    printf(LOG_SUFFIX);
}
