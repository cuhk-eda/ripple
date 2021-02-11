#ifndef _STA_STA_H_
#define _STA_STA_H_

#include "lib.h"
#include "data.h"

namespace sta{

    class STATimer{
        public:
            enum TimingMode{
                EarlyRise = 0,
                EarlyFall = 1,
                LateRise  = 2,
                LateFall  = 3
            };
            STALibrary eLib;
            STALibrary lLib;
            bool initialized{false};

            vector<STACell*> cells;
            vector<STAIO*> ios;
            vector<STANet*> nets;
    };
}

extern sta::STATimer rtimer;

#endif

