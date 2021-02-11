#ifndef _IO_UTILS_
#define _IO_UTILS_

#include <string>
#include <vector>
#include <fstream>

bool getInputStream(std::string &file, std::ifstream &ifs);
bool getOutputStream(std::string &file, std::ofstream &ofs);
bool getTokens(std::string &str, std::vector<std::string> &token);

#endif

