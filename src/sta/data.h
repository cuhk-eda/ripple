#ifndef _STA_DATA_H_
#define _STA_DATA_H_

#include "../global.h"
#include "../db/db.h"

namespace sta{


/*****************
  Timing Analysis
*****************/

class STANode{
    public:
        double x,y;
        int upEdge;
        int downEdges[4];
        double cap;  //wire capacitance
        double downCap; //downstream cap
        double load; //extra loading (pin)
        double delay; //delay from source
        double slew;
        STANode(){
            y = INT_MIN;
            x = INT_MIN;
            upEdge = -1;
            for(int i=0; i<4; i++){
                downEdges[i] = -1;
            }
            cap = 0.0;
            downCap = 0.0;
            load = 0.0;
            delay = DBL_MAX;
            slew = DBL_MAX;
        }
        STANode(double ix, double iy){
            x=ix;
            y=iy;
            upEdge = -1;
            for(int i=0; i<4; i++){
                downEdges[i] = -1;
            }
            cap = 0.0;
            downCap = 0.0;
            load = 0.0;
            delay = DBL_MAX;
            slew = DBL_MAX;
        }
};
class STAEdge{
    public:
        int fm;
        int to;
        double len;
        double res; //resistance
        double delay; //wire delay
        STAEdge(){
            fm = -1;
            to = -1;
            len = 0.0;
            res = 0.0;
            delay = DBL_MAX;
        }
        STAEdge(int f, int t){
            fm = f;
            to = t;
            len = 0.0;
            res = 0.0;
            delay = DBL_MAX;
        }
        STAEdge(int f, int t, double l){
            fm = f;
            to = t;
            len = l;
            res = 0.0;
            delay = DBL_MAX;
        }
};

class STANet{
    public:
        db::Net *net;

        vector<int> pins;

        vector<STANode> nodes;
        vector<STAEdge> edges;
        vector<int> topoNodes;
        vector<double> delays;

        bool error{false};
};

class STAIO{
    public:
        db::IOPin *iopin;
        char dir;

        STAIO(db::IOPin *iopin);
};

class STACell{
    public:
        db::Cell *cell;

        STACell(db::Cell *dbCell) : cell(dbCell) {}
};

}

#endif

