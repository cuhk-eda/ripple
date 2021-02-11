#include "gp_inflate.h"

#include "gp_congest.h"
#include "gp_data.h"
#include "gp_global.h"

double maxCellInflateRatio = 2.5;  // xbar: 5.0
double maxGCellRatio = 2.5;        // xbar: 5.0
double maxFreeSpaceForInflate = 0.5;
double NDRWeight = 1.0;

vector<vector<double> > congRatio[2];
vector<vector<double> > cellRatio[2];

int nowDir = 0;

list<struct densityTile> congTiles[2];

vector<int> cellTileX[2];
vector<int> cellTileY[2];
vector<vector<double> > tileRatio[2];  // inflatioin ratio for each tile
vector<vector<int> > tileCellArea[2];  // cell area which is using the tile ratio

void getAllRouteCongTiles(int isHorizontal) {
    bool isH = (isHorizontal == -1 || isHorizontal == 1);
    bool isV = (isHorizontal == -1 || isHorizontal == 0);

    congTiles[nowDir].clear();

    for (int x = 0; x < numRGridX; x++) {
        for (int y = 0; y < numRGridY; y++) {
            double LSupply = 0.0, LDemand = 0.0;
            double RSupply = 0.0, RDemand = 0.0;
            double BSupply = 0.0, BDemand = 0.0;
            double TSupply = 0.0, TDemand = 0.0;
            if (x > 0) {
                LSupply = database.grGrid.getGCellSupplyH(x - 1, y);
                LDemand = database.grGrid.getGCellDemandH(x - 1, y);
                if (LSupply == 0.0) {
                    LSupply += 1.0;
                    LDemand += 1.0;
                }
            }
            if (x < numRGridX - 1) {
                RSupply = database.grGrid.getGCellSupplyH(x, y);
                RDemand = database.grGrid.getGCellDemandH(x, y);
                if (RSupply == 0.0) {
                    RSupply += 1.0;
                    RDemand += 1.0;
                }
            }
            if (y > 0) {
                BSupply = database.grGrid.getGCellSupplyV(x, y - 1);
                BDemand = database.grGrid.getGCellDemandV(x, y - 1);
                if (BSupply == 0.0) {
                    BSupply += 1.0;
                    BDemand += 1.0;
                }
            }
            if (y < numRGridY - 1) {
                TSupply = database.grGrid.getGCellSupplyV(x, y);
                TDemand = database.grGrid.getGCellDemandV(x, y);
                if (TSupply == 0.0) {
                    TSupply += 1.0;
                    TDemand += 1.0;
                }
            }
            double OFRatio = 1.0;
            if (isH) {
                if (LSupply > 0) {
                    OFRatio = max(OFRatio, max(1.0, LDemand / LSupply));
                }
                if (RSupply > 0) {
                    OFRatio = max(OFRatio, max(1.0, RDemand / RSupply));
                }
            }
            if (isV) {
                if (BSupply > 0) {
                    OFRatio = max(OFRatio, max(1.0, BDemand / BSupply));
                }
                if (TSupply > 0) {
                    OFRatio = max(OFRatio, max(1.0, TDemand / TSupply));
                }
            }
            if (OFRatio > 1.0) {
                densityTile congTile;
                congTile.x = x;
                congTile.y = y;
                congTile.density = min(maxGCellRatio, OFRatio);
                congTiles[nowDir].push_back(congTile);
                tileRatio[nowDir][x][y] = congTile.density;
            } else {
                tileRatio[nowDir][x][y] = 1.0;
            }
        }
    }
}

void deleteCongestionTiles() { congTiles[nowDir].clear(); }

bool compareCongInc(densityTile a, densityTile b) { return a.density < b.density; }
bool compareCongDec(densityTile a, densityTile b) { return a.density > b.density; }

void createInflation() {
    tileCellArea[0].resize(numRGridX, vector<int>(numRGridY, 0));
    tileCellArea[1].resize(numRGridX, vector<int>(numRGridY, 0));
    tileRatio[0].resize(numRGridX, vector<double>(numRGridY, 0.0));
    tileRatio[1].resize(numRGridX, vector<double>(numRGridY, 0.0));

    congRatio[0].resize(numRGridX, vector<double>(numRGridY, 0.0));
    congRatio[1].resize(numRGridX, vector<double>(numRGridY, 0.0));

    cellTileX[0].resize(numCells);
    cellTileX[1].resize(numCells);
    cellTileY[0].resize(numCells);
    cellTileY[1].resize(numCells);
}

void refreshInflateForRoute() {
    for (int i = 0; i < numCells; i++) {
        cellTileX[nowDir][i] = -1;
        cellTileY[nowDir][i] = -1;
    }
    for (int i = 0; i < numRGridX; i++) {
        for (int j = 0; j < numRGridY; j++) {
            tileCellArea[nowDir][i][j] = 0;
        }
    }
}

void setDirection(int isH) {
    if (isH) {
        nowDir = 0;
    } else {
        nowDir = 1;
    }
}

void loadCongestedList(int isH) {
    // Get all the congested tiles, and sort these tile according to their congestion
    getAllRouteCongTiles(isH);
    congTiles[nowDir].sort(compareCongInc);

    refreshInflateForRoute();

    for (int i = 0; i < numCells; i++) {
        int tx = (int)((cellX[i] - RGridLX) / RGridW);
        int ty = (int)((cellY[i] - RGridLY) / RGridH);
        tx = max(0, min(numRGridX - 1, tx));
        ty = max(0, min(numRGridY - 1, ty));
        if (tileRatio[nowDir][tx][ty] > 1.0) {
            cellTileX[nowDir][i] = tx;
            cellTileY[nowDir][i] = ty;
        }
    }

    /*
    for(int x=0; x<numRGridX; x++){
        for(int y=0; y<numRGridY; y++){
            if(tileRatio[nowDir][x][y] <= 1.0){
                continue;
            }
            vector<int>::iterator ni;
            vector<int>::iterator ne;
            if(isH==1){
                ni=circuit.groute_net_h[x][y].begin();
                ne=circuit.groute_net_h[x][y].end();
            }else{
                ni=circuit.groute_net_v[x][y].begin();
                ne=circuit.groute_net_v[x][y].end();
            }
            //for each net passing through the congested edge
            for(; ni!=ne; ++ni){
                int net=*ni;
                if(circuit.nets[net].pins.size() != netCell[net].size()){
                    printlog(LOG_ERROR, "pin count not match");
                }
                int nPins = netCell[net].size();
                for(int p=0; p<nPins; p++){
                    int cell = netCell[net][p];
                    if(cell >= numCells){
                        continue;
                    }
                    int tx = (int)((cellX[cell]+netPinX[net][p] - RGridLX) / RGridW);
                    int ty = (int)((cellY[cell]+netPinY[net][p] - RGridLY) / RGridH);
                    if(abs(tx-x)+abs(ty-y)>10){
                        continue;
                    }
                    int ox=cellTileX[nowDir][cell];
                    int oy=cellTileY[nowDir][cell];
                    if(ox==-1 || oy==-1 || tileRatio[nowDir][x][y]>tileRatio[nowDir][ox][oy]){
                        cellTileX[nowDir][cell]=x;
                        cellTileY[nowDir][cell]=y;
                        tileCellArea[nowDir][x][y]+=(int)(cellAW[cell]*cellAH[cell]);
                        if(ox>=0 && oy>=0){
                            tileCellArea[nowDir][ox][oy]-=(int)(cellAW[cell]*cellAH[cell]);
                        }
                    }
                }
            }
        }
    }
    */
}

bool cellInflation(int isHorizontal, double restoreRatio) {
    cout << "Restore ratio = " << restoreRatio << endl;
    setDirection(!isHorizontal);
    loadCongestedList(!isHorizontal);
    setDirection(isHorizontal);
    loadCongestedList(isHorizontal);

    if (congTiles[nowDir].size() == 0) {
        cout << "WARN: No congestion, abort cell inflation" << endl;
        return false;
    }

    // pre-inflate calculation
    vector<double> cellRatio(numCells, 1.0);
    int nNDR = 0;
    for (int i = 0; i < numCells; i++) {
        int tx = cellTileX[nowDir][i];
        int ty = cellTileY[nowDir][i];
        if (tx != -1 && tileRatio[nowDir][tx][ty] > 1) {
            cellRatio[i] = tileRatio[nowDir][tx][ty];
            int nNets = cellNet[i].size();
            for (int j = 0; j < nNets; j++) {
                if (netNDR[cellNet[i][j]] >= 0) {
                    cellRatio[i] *= NDRWeight;
                    nNDR++;
                    break;
                }
            }
        }
        cellRatio[i] = min(cellRatio[i], maxCellInflateRatio);
    }

    double siteArea = (double)database.siteH / (double)database.siteW;
    double beforeFreeArea = database.siteMap.nPlaceable * siteArea;
    double beforeCellArea = 0.0;
    double afterCellArea = 0.0;
    double limitCellArea = 0.0;
    double limitingScale = 1.0;

    for (int i = 0; i < numCells; i++) {
        double cellW = cellAW[i];
        double cellH = cellAH[i];
        beforeCellArea += (cellW * cellH);
        if (isHorizontal) {
            cellH *= cellRatio[i];
        } else {
            cellW *= cellRatio[i];
        }
        afterCellArea += (cellW * cellH);
    }
    limitCellArea = beforeCellArea + (beforeFreeArea - beforeCellArea) * maxFreeSpaceForInflate;
    if (afterCellArea > limitCellArea) {
        // inflate too much
        limitingScale = limitCellArea / afterCellArea;
        cout << "inflate too much, limit scale=" << limitingScale << endl;
    }

    if (restoreRatio < 0) {
        for (int i = 0; i < numCells; i++) {
            if (isHorizontal) {
                // cellW[i]*=cellRatio[i];
                cellH[i] *= cellRatio[i];
            } else {
                cellW[i] *= cellRatio[i];
                // cellH[i]*=cellRatio[i];
            }
        }
    } else {
        for (int i = 0; i < numCells; ++i) {
            double beforeRatio = cellW[i] / cellAW[i];
            double afterRatio = cellRatio[i];
            double finalRatio = max(1.0 + (1.0 - beforeRatio) * (1.0 - restoreRatio), afterRatio);
            if (isHorizontal) {
                cellH[i] = cellAH[i] * finalRatio;
            } else {
                cellW[i] = cellAW[i] * finalRatio;
            }
        }
    }
    return true;
}
