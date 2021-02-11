#ifndef _DP_H_
#define _DP_H_

#include "../tcl/tcl.h"
#include "dp_data.h"

namespace dp {

class DPModule : public ripple::ShellModule {
private:
    static string _name;

public:
    static unsigned CPU;
    static string DefEval;
    static unsigned MaxDisp;
    static double MaxDensity;
    static int MaxLGIter;
    static int MaxGMIter;
    static int MaxLMIter;
    static int MaxMPIter;
    static int LGStepSize;
    static int LGThresholdBase;
    static int LGThresholdStep;
    static unsigned LGTrial;
    static unsigned LGOperRegSize;
    static bool GMEnable;
    static unsigned GMThresholdBase;
    static unsigned GMThresholdStep;
    static int GMTrial;
    static int LMThresholdBase;
    static int LMThresholdStep;
    static int LMTrial;
    static bool EnableLocalVert;
    static bool EnableLocalReorder;
    static bool EnableLocalCompact;
    static bool EnablePinAcc;
    static bool EnablePinAccRpt;

    // default 1, larger number, higher accuracy
    static int MLLAccuracy;
    // default false
    static bool MLLDispFromInput;
    static bool MLLPGAlign;
    static unsigned MLLPinCost;
    static unsigned MLLMaxDensity;
    // default false
    static bool MLLUseILP;
    static bool MLLTotalDisp;

    void registerCommands();
    void registerOptions();
    void showOptions() const;

public:
    static DPlacer* dplacer;

    static bool dplace(const string& flowName = "iccad2017");
    static bool dplace(ripple::ShellOptions& args, ripple::ShellCmdReturn& ret);
    const string& name() const { return _name; }
};
}  // namespace dp

#endif
