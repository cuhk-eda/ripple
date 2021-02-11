#ifndef _GP_DATA_H_
#define _GP_DATA_H_

#include "../db/db.h"

#include <memory>

extern int numNodes;
extern int numCells;
extern int numMacros;
extern int numPads;
extern int numFixed; // numMacros+numPads

extern int numNets;
extern int numPins;
extern int numFences;

extern double coreLX, coreLY;
extern double coreHX, coreHY;
extern double siteW, siteH;
extern int numSitesX, numSitesY;

extern map<string,int> nameMap;
extern vector<string> cellName;

extern vector<vector<int> > netCell;
extern vector<vector<double> > netPinX;
extern vector<vector<double> > netPinY;
extern vector<double> netWeight;
extern vector<int> netNDR;
extern vector<vector<int> > cellNet;
extern vector<vector<int> > cellPin;

extern vector<double> cellX;
extern vector<double> cellY;
extern vector<double> cellW;
extern vector<double> cellH;
extern vector<double> cellAW;
extern vector<double> cellAH;

//coordinate of the nearest fence region
extern vector<double> cellFenceX;
extern vector<double> cellFenceY;
extern vector<double> cellFenceDist;
extern vector<int> cellFenceRect;
extern vector<int> cellFence;

extern vector<vector<double> > fenceRectLX;
extern vector<vector<double> > fenceRectLY;
extern vector<vector<double> > fenceRectHX;
extern vector<vector<double> > fenceRectHY;

//for routing
extern int numRGridX;
extern int numRGridY;
extern double RGridW;
extern double RGridH;
extern double RGridLX;
extern double RGridLY;
extern double RGridHX;
extern double RGridHY;

void gp_copy_in(bool layout);
void gp_copy_out();
double hpwl();

#endif
