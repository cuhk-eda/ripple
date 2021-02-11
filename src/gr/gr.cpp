#include "gr.h"
#include "../db/db.h"
#include "../global.h"
#include "../tcl/register.h"
#include "cugr.h"
#include "fastroute.h"
#include "ripplegr.h"

using namespace gr;

GRSetting grSetting;


int groute(GRResult& result, bool ndr, bool reset)
{
    switch (grSetting.router) {
    case GRSetting::FastRoute:
        return fastroute(result, ndr, reset);
    case GRSetting::RippleGR:
        return ripplegr();
    case GRSetting::CUGR:
        return cugr();
    default:
        return 0;
    }
}

int GRModule::GCellTracksX = 30;
int GRModule::GCellTracksY = 30;

void GRModule::showOptions() const {
    printlog(LOG_INFO, "gcellTracksX    : %d", GCellTracksX);
    printlog(LOG_INFO, "gcellTracksY    : %d", GCellTracksY);
}

