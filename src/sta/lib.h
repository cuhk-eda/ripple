#ifndef _STA_LIB_H_
#define _STA_LIB_H_

#include "../global.h"
//#include "../db/db.h"

namespace sta{

    /*****************
      Timing Library
    *****************/
    class STALibraryLUT {
        public:
            bool isScalar = false;
            double value;
            vector<double> indexX;
            vector<double> indexY;
            vector<double> indexZ;
            double minX;
            double maxX;
            double minY;
            double maxY;
            double minZ;
            double maxZ;
            vector<vector<vector<double>>> values;

            STALibraryLUT() {}
            STALibraryLUT(double scalar);

            void clear();
            //  double get(double x = 0, double y = 0, double z = 0);
            void print();
    };
    
    class STALibraryTiming {
        public:
            STALibraryLUT delayRise;
            STALibraryLUT delayFall;
            STALibraryLUT slewRise;
            STALibraryLUT slewFall;
            int relatedPin;
            string relatedPinName;
            char timingSense; //'+' / '-' / 'x'
    };

    class STALibraryOPin {
        private:
            string _name = "";
        public:
            int pin = -1;
            double min_capacitance = 0.0;
            double max_capacitance = 0.0;
            vector<STALibraryTiming> timings;
            
            STALibraryOPin(const string& name = "") : _name(name) {}
            STALibraryOPin(const STALibraryOPin& opin);

            inline const string& name() const { return _name; }
            inline void name(const string& s) { _name = s; }
    };

    class STALibraryIPin {
        private:
            string _name = "";
        public:
            int pin = -1;
            double capacitance = 0.0;

            STALibraryIPin(const string& name = "");
            STALibraryIPin(const STALibraryIPin& ipin);

            inline const string& name() const { return _name; }
            inline void name(const string& s) { _name = s; }
    };

    class STALibraryCell {
        private:
            string _name;
        public:
            vector<int> pins;
            //map from FCell pin index to STALibraryCell index
            vector<STALibraryOPin> opins;
            vector<STALibraryIPin> ipins;
            vector<vector<int> > timingArcs;
            
            STALibraryCell(const string& name = "");

            inline const string& name() const { return _name; }
            inline void name(const string& s) { _name = s; }

            inline void addOPin(const string& s) { opins.emplace_back(s); }
            inline void addIPin(const string& s) { ipins.emplace_back(s); }
    };
    
    class STALibrary {
        public:
            string name;
            vector<STALibraryCell> cells;

            void preLoad();
            void postLoad();
            unsigned addCell(const string& name);
    };
}

#endif

