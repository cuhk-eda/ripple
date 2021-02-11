#ifndef _GP_QSOLVE_H_
#define _GP_QSOLVE_H_

#include "gp_cg.h"

void lowerBound(
        double epsilon,
        double pseudoAlpha,
        int repeat,
        LBMode mode,
        NetModel model,
        double relax = 0.0,
        double maxDisp = -1);

#endif

