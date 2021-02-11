#ifndef _DB_ROUTE_H_
#define _DB_ROUTE_H_

namespace db {
class GCell {
public:
    int lx = 0;
    int ly = 0;
    int hx = 0;
    int hy = 0;
    float supplyX = 0;
    float supplyY = 0;
    short supplyZ = 0;
    float demandX = 0;
    float demandY = 0;
    short demandZ = 0;
    list<Net*> netsX;
    list<Net*> netsY;
    list<Net*> netsZ;
    list<Pin*> pins;
};

class GRGrid {
private:
    int nx = 0;
    int ny = 0;
    int nz = 0;
    int tx = 0;
    int ty = 0;
    int tz = 0;
    vector<vector<vector<GCell>>> gcells;
    char* rgrids = nullptr;

public:
    int trackL, trackR;
    int trackB, trackT;
    int trackStepX, trackStepY;
    int trackNX, trackNY;

    int gcellL, gcellR;
    int gcellB, gcellT;
    unsigned gcellStepX = 0;
    unsigned gcellStepY = 0;
    unsigned gcellNX = 0;
    unsigned gcellNY = 0;

    void initGCells(int nx, int ny, int nz);

    float getGCellSupplyH(int x, int y);
    float getGCellSupplyV(int x, int y);
    float getGCellDemandH(int x, int y);
    float getGCellDemandV(int x, int y);

    GCell* getGCell(int x, int y, int z);
    void initRGrids(int tx, int ty, int nlayer);
    void setRGrid(int x, int y, int z, char v);
    char getRGrid(int x, int y, int z);

    void addBlockage(int lx, int ly, int hx, int hy, int z);
};

}

#endif
