#include "utils.h"

bool getInputStream(std::string &file, std::ifstream *ifs){
    ifs = new std::ifstream(file.c_str());
    return true;
}

bool getOutputStream(std::string &file, std::ofstream &ofs){
    return true;
}
