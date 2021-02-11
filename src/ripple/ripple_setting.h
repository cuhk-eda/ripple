#ifndef _RIPPLE_SETTING_H_
#define _RIPPLE_SETTING_H_

namespace ripple{
    class RippleSetting{
        public:
        enum PlaceFlow{
            Default,
            Global,
            CLI,
            Test,
            DAC2016,
            ICCAD2017,
            Eval
        };

        int force_stop;

        PlaceFlow flow;

        RippleSetting(){
            force_stop = 0;
            flow = ICCAD2017;
        }
    };
}

extern ripple::RippleSetting rippleSetting;

#endif

