#include "gp_main.h"

#include "../ut/utils.h"
#include "gp_congest.h"
#include "gp_data.h"
#include "gp_global.h"
#include "gp_inflate.h"
#include "gp_qsolve.h"
#include "gp_region.h"
#include "gp_spread.h"

#ifdef LIBGD
#include "gp_draw.h"
#endif

using namespace gp;

void placement();
void upperBound(const int iter, const int binSize, const int legalTimes, const int inflateTimes, const double td);

bool enable_keepinflate = false;
// bool enable_fencemode=false;

void gp_main() {
    unsigned timer_id = utils::Timer::start();

    printlog(LOG_INFO, "#nodes : %d", numNodes);
    printlog(LOG_INFO, "#nets  : %d", numNets);
    printlog(LOG_INFO, "#pins  : %d", numPins);
    printlog(LOG_INFO, "#cells : %d", numCells);
    printlog(LOG_INFO, "#macros: %d", numMacros);
    printlog(LOG_INFO, "#pads  : %d", numPads);
    printlog(LOG_VERBOSE, "#InitIter: %d", GPModule::InitIterations);
    printlog(LOG_VERBOSE, "#WLIter: %d", GPModule::MainWLIterations);
    printlog(LOG_VERBOSE, "#CGIter: %d", GPModule::MainCongIterations);
    printlog(LOG_VERBOSE, "#GRIter: %d", GPModule::MainGRIterations);
    printlog(LOG_INFO, "input wirelength = %ld", (long)hpwl());

    placement();

    printlog(LOG_INFO, "output wirelength = %ld", (long)hpwl());
    printlog(LOG_INFO, "global place runtime = %.2lf", utils::Timer::stop(timer_id));
}

void placement() {
    int initIter = GPModule::InitIterations;  // initial CG iteration
    // int initIter     = gpSetting.initIter;    //initial CG iteration
    int mainWLIter = GPModule::MainWLIterations;  // old version: 3
    // int mainWLIter   = gpSetting.mainWLIter;    // old version: 3
    int mainCongIter = GPModule::MainCongIterations;  // old version: 34
    int mainGRIter = GPModule::MainGRIterations;  // heavy global router iteration
    int LBIter = GPModule::LowerBoundIterations;
    int UBIter = GPModule::UpperBoundIterations;
    int mainIter = mainWLIter + mainCongIter + mainGRIter;  // total GP iteration
    int finalIter = GPModule::FinalIterations;
    // int finalIter    = gpSetting.finalIter;
    double pnWeightBegin = GPModule::PseudoNetWeightBegin;
    // double pnWeightBegin = gpSetting.pseudoNetWeightBegin;
    double pnWeightEnd = GPModule::PseudoNetWeightEnd;
    // double pnWeightEnd   = gpSetting.pseudoNetWeightEnd;
    NetModel initNetModel = NetModelNone;
    if (GPModule::InitNetModel == "Clique") {
        initNetModel = NetModelClique;
    } else if (GPModule::InitNetModel == "Star") {
        initNetModel = NetModelStar;
    } else if (GPModule::InitNetModel == "B2B") {
        initNetModel = NetModelB2B;
    } else if (GPModule::InitNetModel == "Hybrid") {
        initNetModel = NetModelHybrid;
    } else if (GPModule::InitNetModel == "MST") {
        initNetModel = NetModelMST;
    }
    NetModel mainNetModel = NetModelNone;
    if (GPModule::MainNetModel == "Clique") {
        mainNetModel = NetModelClique;
    } else if (GPModule::MainNetModel == "Star") {
        mainNetModel = NetModelStar;
    } else if (GPModule::MainNetModel == "B2B") {
        mainNetModel = NetModelB2B;
    } else if (GPModule::MainNetModel == "Hybrid") {
        mainNetModel = NetModelHybrid;
    } else if (GPModule::MainNetModel == "MST") {
        mainNetModel = NetModelMST;
    }

    createInflation();

    if (rippleSetting.force_stop) {
        rippleSetting.force_stop = false;
        return;
    }

    if (initIter > 0) {
        //--Random placement
        for (int i = 0; i < numCells; ++i) {
            cellX[i] = getrand((double)(coreLX + 0.5 * cellW[i]), (double)(coreHX - 0.5 * cellW[i]));
            cellY[i] = getrand((double)(coreLY + 0.5 * cellH[i]), (double)(coreHY - 0.5 * cellH[i]));
        }
        printlog(LOG_INFO, "random wirelength = %ld", (long)hpwl());
        //--Initial placement (CG)
        lowerBound(0.001, 0.0, initIter, LBModeFenceBBox, initNetModel);
        printlog(LOG_INFO, "initial wirelength = %ld", (long)hpwl());
    }
#ifdef LIBGD
    drawcell("gp_initial");
#endif

    //--Global Placement Iteration
    for (int iter = 1; iter <= mainIter; iter++) {
        if (rippleSetting.force_stop) {
            rippleSetting.force_stop = false;
            break;
        }
        findCellRegionDist();
        legalizeRegion();

        //--Upper Bound
        const double td{GPModule::TargetDensityBegin + (GPModule::TargetDensityEnd - GPModule::TargetDensityBegin) * (iter - 1) / (mainIter - 1)};
        if (iter <= mainWLIter) {
            upperBound(iter, 8, UBIter, 0, td);
        } else if (iter <= mainWLIter + mainCongIter) {
            grSetting.router = GRSetting::GRRouter::RippleGR;
            upperBound(iter, 4, UBIter, 1, td);
        } else {
            grSetting.router = GRSetting::GRRouter::CUGR;
            upperBound(iter, 4, UBIter, 1, td);
        }
        long upHPWL = hpwl();

#ifdef LIBGD
        drawcell("gp_up_", iter);
#endif

        //--Lower Bound
        lowerBound(0.001, pnWeightBegin + (pnWeightEnd - pnWeightBegin) * (iter - 1) / (mainIter - 1), LBIter, LBModeFenceRelax, mainNetModel, 0.0);
        long loHPWL = hpwl();

#ifdef LIBGD
        drawcell("gp_lo_", iter);
#endif

        printlog(LOG_INFO, "[%2d] up=%ld lo=%ld", iter, upHPWL, loHPWL);
    }

    upperBound(mainIter + 1, 4, finalIter, 0, GPModule::TargetDensityEnd);
    printlog(LOG_INFO, "final=%ld", (long)hpwl());
#ifdef LIBGD
    drawcell("gp_final");
#endif

    // copy(cellAW.begin(), cellAW.end(), cellW.begin());
    // copy(cellAH.begin(), cellAH.end(), cellH.begin());
    // updateCongMap();
}

void upperBound(const int iter, const int binSize, const int legalTimes, const int inflateTimes, const double td) {
    spreadCells(binSize, legalTimes, td);

    double restoreRatio = 1.0;
    if (iter < 10) {
        restoreRatio = 0.6;
    } else {
        restoreRatio = 0.3;
    }

    bool inflated = true;
    for (int i = 0; i < inflateTimes && inflated; i++) {
        updateCongMap();
        if (enable_keepinflate) {
            inflated = cellInflation(1, restoreRatio);
        } else {
            inflated = cellInflation(1, -1.0);
        }
        if (inflated) {
            spreadCells(binSize, 1, td);
        }
    }
    // updateCongMap();

    if (!enable_keepinflate) {
        copy(cellAW.begin(), cellAW.end(), cellW.begin());
        copy(cellAH.begin(), cellAH.end(), cellH.begin());
    }
}
