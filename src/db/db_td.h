#ifndef _DB_TD_H_
#define _DB_TD_H_

namespace db{
    class TDBins{
        private:
            double td{1.0};
        public:
            int binL, binR;
            int binB, binT;
            int binStepX, binStepY;
            int binNX, binNY;

            void setTDBin(const double density) { td = density; }
            double getTDBin() const { return td; }
    };
}

#endif

