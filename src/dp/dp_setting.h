#ifndef _DP_SETTING_H_
#define _DP_SETTING_H_

class DPSetting {
public:
    DPSetting() {
        /*
        maxDisplacement     = 15000;
        maxDensity          = 1.0;

        MaxLGIter           = 300;
        MaxGMIter           = 100;
        MaxLMIter           = 0;

        LGStepSize          = 10;
        LGThresholdBase     = 0;
        LGThresholdStep     = 20;
        LGTrial             = 1;

        GMThresholdBase     = 10;
        GMThresholdStep     = 10;
        GMTrial             = 1;

        enableLocalVert     = true;
        enableLocalReorder  = true;
        enableLocalCompact  = true;
        */
    }

    int check() { return 0; }
};

extern DPSetting dpSetting;

#endif
