#ifndef _GP_SETTING_H_
#define _GP_SETTING_H_

enum LBMode{
    LBModeSimple        = 1,
    LBModeFenceBBox     = 2,
    LBModeFenceRect     = 3,
    LBModeFenceRelax    = 4
};

enum NetModel{
    NetModelNone        = 0,
    NetModelClique      = 1,
    NetModelStar        = 2,
    NetModelB2B         = 3,
    NetModelHybrid      = 4,
    NetModelMST         = 5
};

#endif
