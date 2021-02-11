#ifndef _GP_GP_H_
#define _GP_GP_H_

#include "../tcl/tcl.h"
#include "gp_data.h"

int gplace(int argc = 0, char** argv = NULL);

namespace gp {

class GPModule : public ripple::ShellModule {
private:
    static std::string _name;

public:
    static int NumThreads;
    static int InitIterations;
    static int MainWLIterations;
    static int MainCongIterations;
    static int MainGRIterations;
    static int LowerBoundIterations;
    static int UpperBoundIterations;
    static int FinalIterations;

    static double PseudoNetWeightBegin;
    static double PseudoNetWeightEnd;
    static double TargetDensityBegin;
    static double TargetDensityEnd;

    static std::string InitNetModel;
    static std::string MainNetModel;

    static bool EnableFence;
    static bool EnableKeepInflate;
    static bool Experimental;

    void registerCommands();
    void registerOptions();
    void showOptions() const;

public:
    const string& name() const { return _name; }
    static bool gplace(ripple::ShellOptions& args, ripple::ShellCmdReturn& ret);
    static bool gplace();
};
}

#endif
