#ifndef _GR_CUGR_H_
#define _GR_CUGR_H_
#include "../db/db.h"
#include "../io/io.h"

class CUGRSetting {
public:
    int iterations;
    int threads;

public:
    CUGRSetting() {
        iterations = 1;
        threads = 8;
    }
};

void initializeVariable();
bool cugr();
bool readCUGROutputTxt(const string& inputTxt);
bool writeComponents(ofstream& ofs);
bool writeDEF(const string& inputDef, const string& outputDef);
string cugrGetOrient(bool flipX, bool flipY);

#endif