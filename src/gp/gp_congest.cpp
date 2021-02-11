#include "gp_data.h"
#include "gp_global.h"
#include "gp_congest.h"

void updateCongMap() {
    gp_copy_out();
    GRResult gr_result;
    // grSetting.gcellTrackX = 30;
    // grSetting.gcellTrackY = 30;
    // grSetting.fixBlockedDemand = true;
    // grSetting.localCongestion = false;

    if (grSetting.ndrMultiplier > 1.0) {
        groute(gr_result, true, true);
        groute(gr_result, false, false);
    }else{
        groute(gr_result, false, true);
    }
}
