#ifndef _GR_SETTING_H_
#define _GR_SETTING_H_

class GRSetting{
    public:
        enum GRRouter{
            NONE = 0,
            FastRoute = 2,
            RippleGR = 3,
            CUGR = 5
        };
        GRRouter router;
        int gcellTrackX;
        int gcellTrackY;

        double ndrMultiplier;
        bool fixBlockedDemand;
        bool localCongestion;

        GRSetting(){
            router = NONE;
            gcellTrackX = 30;
            gcellTrackY = 30;
            // ndrMultiplier = 2.0;
            ndrMultiplier = 0.0;
            fixBlockedDemand = true;
            localCongestion = true;
        }
};

extern GRSetting grSetting;

#endif

