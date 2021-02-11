#ifndef _GR_H_
#define _GR_H_

#include "../global.h"
#include "../tcl/tcl.h"
#include "gr_setting.h"

typedef struct GRResult GRResult;

struct GRResult {
    int maxHOF;
    int maxVOF;
    int totalHOF;
    int totalVOF;

    int totalWL;
    int tof;
    int totalVia;
    int ofEdge;
};

int groute(GRResult& result, bool ndr, bool reset);
inline int groute(GRResult& result) { return groute(result, false, true); }

namespace gr {

class GRModule : public ripple::ShellModule {
public:
    static int GCellTracksX;
    static int GCellTracksY;

    void showOptions() const;
};
}

#endif

