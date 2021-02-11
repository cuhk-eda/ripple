#ifndef _GR_RIPPLEGR_H_
#define _GR_RIPPLEGR_H_
#include "../db/db.h"
#include "../global.h"
#include "../io/io.h"
#include "../ut/geo.h"
#include "gr.h"

bool ripplegr();

class RippleGR {
public:
    void initDemandSupplyMap();
    void calcDemandMap();
    void calcSupplyMap(double BlockPorosity = 1.0);
    void writeCongestionMap();
    std::tuple<int, int, int, int, int, int> getPinInfo(db::Pin* pin);
    std::tuple<double, double> getTotalTracks();
    std::tuple<double, double> getWireSpace();
};
#endif
