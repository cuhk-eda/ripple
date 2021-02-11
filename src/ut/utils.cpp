#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <sys/time.h>
#include <string>

#include "utils.h"

using namespace std;

//////////MISC FUNTIONS//////////

double rect_overlap_area(double alx, double aly, double ahx, double ahy, double blx, double bly, double bhx, double bhy){
    if(alx>=bhx || ahx<=blx || aly>=bhy || ahy<=bly){
        return 0.0;
    }
    return (std::min(ahx,bhx) - std::max(alx,blx)) * (std::min(ahy,bhy) - std::max(aly,bly));
}

////////MEMORY CONSUMPTION////////////
std::string show_mem_size(long memsize){
    stringstream ss;
    double size=memsize;
    if(memsize<1024){
        ss<<memsize;
        ss<<" B";
    }else if(memsize/1024<1024){
        ss<<memsize/1024;
        ss<<" KB";
    }else if(memsize/1024/1024<1024){
        ss<<size/1024/1024;
        ss<<" MB";
    }else if(memsize/1024/1024/1024<1024){
        ss<<size/1024/1024/1024;
        ss<<" GB";
    }else if(memsize/1024/1024/1024/1024<1024){
        ss<<size/1024/1024/1024/1024;
        ss<<" TB";
    }else{
        ss<<size/1024/1024/1024/1024/1024;
        ss<<" PB";
    }
    return ss.str();
}
void print_field(int field, std::string buffer){
    switch(field){
        case 0:
            printlog(LOG_INFO, "pid:     %s", buffer.c_str());
            break;
        case 1:
            printlog(LOG_INFO, "comm:    %s", buffer.c_str());
            break;
        case 2:
            printlog(LOG_INFO, "state:   %s", buffer.c_str());
            break;
        default: return;
    }
}
void print_process_stat(int stat){
    ifstream fs("/proc/self/stat");
    string buffer;
    if(stat==STAT_ALL){
        fs>>buffer; cout<<"pid:         "<<buffer<<endl;    //0
        fs>>buffer; cout<<"comm:        "<<buffer<<endl;    //1
        fs>>buffer; cout<<"state:       "<<buffer<<endl;    //2
        fs>>buffer; cout<<"ppid:        "<<buffer<<endl;    //3
        fs>>buffer; cout<<"pgrp:        "<<buffer<<endl;    //4
        fs>>buffer; cout<<"session:     "<<buffer<<endl;    //5
        fs>>buffer; cout<<"tty_nr:      "<<buffer<<endl;    //6
        fs>>buffer; cout<<"tpgid:       "<<buffer<<endl;    //7
        fs>>buffer; cout<<"flags:       "<<buffer<<endl;    //8
        fs>>buffer; cout<<"minflt:      "<<buffer<<endl;    //9
        fs>>buffer; cout<<"cminflt:     "<<buffer<<endl;    //10
        fs>>buffer; cout<<"majflt:      "<<buffer<<endl;    //11
        fs>>buffer; cout<<"cmajflt:     "<<buffer<<endl;    //12
    }else{
        for(int i=0; i<=12; i++){
            fs>>buffer;
        }
    }
    if(stat==STAT_ALL || stat==STAT_TIME){
        fs>>buffer; cout<<"utime:       "<<buffer<<endl;    //13
        fs>>buffer; cout<<"stime:       "<<buffer<<endl;    //14
        fs>>buffer; cout<<"cutime:      "<<buffer<<endl;    //15
        fs>>buffer; cout<<"cstime:      "<<buffer<<endl;    //16
    }else{
        for(int i=13; i<=16; i++){
            fs>>buffer;
        }
    }
    if(stat==STAT_ALL){
        fs>>buffer; cout<<"priority:    "<<buffer<<endl;    //17
        fs>>buffer; cout<<"nice:        "<<buffer<<endl;    //18
        fs>>buffer; cout<<"num_threads: "<<buffer<<endl;    //19
        fs>>buffer; cout<<"itrealvalue: "<<buffer<<endl;    //20
        fs>>buffer; cout<<"starttime:   "<<buffer<<endl;    //21
    }else{
        for(int i=17; i<=21; i++){
            fs>>buffer;
        }
    }
    if(stat==STAT_ALL || stat==STAT_MEM){
        fs>>buffer; cout<<"vsize:       "<<show_mem_size(atol(buffer.c_str()))<<endl;    //22
        fs>>buffer; cout<<"rss:         "<<show_mem_size(atol(buffer.c_str()))<<endl;    //23
        fs>>buffer; cout<<"rsslim:      "<<show_mem_size(atol(buffer.c_str()))<<endl;    //24
        cout<<setbase(16);
        fs>>buffer; cout<<"startcode:   "<<atol(buffer.c_str())<<endl;    //25
        fs>>buffer; cout<<"endcode:     "<<atol(buffer.c_str())<<endl;    //26
        fs>>buffer; cout<<"startstack:  "<<atol(buffer.c_str())<<endl;    //27
        fs>>buffer; cout<<"kstkesp:     "<<atol(buffer.c_str())<<endl;    //28
        fs>>buffer; cout<<"kstkeip:     "<<atol(buffer.c_str())<<endl;    //29
        cout<<setbase(10);
    }else{
        for(int i=22; i<=29; i++){
            fs>>buffer;
        }
    }
    if(stat==STAT_ALL){
        fs>>buffer; cout<<"signal:      "<<buffer<<endl;    //30
        fs>>buffer; cout<<"blocked:     "<<buffer<<endl;    //31
        fs>>buffer; cout<<"sigignore:   "<<buffer<<endl;    //32
        fs>>buffer; cout<<"sigcatch:    "<<buffer<<endl;    //33
        fs>>buffer; cout<<"wchan:       "<<buffer<<endl;    //34
        fs>>buffer; cout<<"nswap:       "<<buffer<<endl;    //35
        fs>>buffer; cout<<"cnswap:      "<<buffer<<endl;    //36
        fs>>buffer; cout<<"exit_signal: "<<buffer<<endl;    //37
        fs>>buffer; cout<<"processor:   "<<buffer<<endl;    //38
        fs>>buffer; cout<<"rt_priority: "<<buffer<<endl;    //39
        fs>>buffer; cout<<"policy:      "<<buffer<<endl;    //40
        fs>>buffer; cout<<"delayacct_blkio_ticks:  "<<buffer<<endl; //41
        fs>>buffer; cout<<"guest_time:  "<<buffer<<endl;    //42
        fs>>buffer; cout<<"cguest_time: "<<buffer<<endl;    //43
        fs>>buffer; cout<<"start_data:  "<<buffer<<endl;    //44
        fs>>buffer; cout<<"end_data:    "<<buffer<<endl;    //45
        fs>>buffer; cout<<"start_brk:   "<<buffer<<endl;    //46
        fs>>buffer; cout<<"arg_start:   "<<buffer<<endl;    //47
        fs>>buffer; cout<<"arg_end:     "<<buffer<<endl;    //48
        fs>>buffer; cout<<"env_start:   "<<buffer<<endl;    //49
        fs>>buffer; cout<<"env_end:     "<<buffer<<endl;    //50
        fs>>buffer; cout<<"exit_code:   "<<buffer<<endl;    //51
    }else{
        for(int i=30; i<=51; i++){
            fs>>buffer;
        }
    }
    fs.close();
}

