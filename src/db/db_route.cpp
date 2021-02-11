#include "db.h"
using namespace db;

#include "../global.h"

/***** GRGrid *****/

void GRGrid::initGCells(int nx, int ny, int nz) {
    this->nx = nx;
    this->ny = ny;
    this->nz = nz;

    gcells.resize(nx, vector<vector<GCell>>(ny, vector<GCell>(nz)));

    for (int x = 0; x < nx; x++) {
        for (int y = 0; y < ny; y++) {
            for (GCell& gcell : gcells[x][y]) {
                gcell.lx = gcellL + x * gcellStepX;
                gcell.ly = gcellB + y * gcellStepY;
                gcell.hx = gcell.lx + gcellStepX;
                gcell.hy = gcell.ly + gcellStepY;
            }
        }
    }
}

float GRGrid::getGCellSupplyH(int x, int y) {
    float supplyH = 0;
    for (int z = 0; z < nz; z++) {
        supplyH += gcells[x][y][z].supplyX;
    }
    return supplyH;
}
float GRGrid::getGCellSupplyV(int x, int y) {
    float supplyV = 0;
    for (int z = 0; z < nz; z++) {
        supplyV += gcells[x][y][z].supplyY;
    }
    return supplyV;
}
float GRGrid::getGCellDemandH(int x, int y) {
    float demandH = 0;
    for (int z = 0; z < nz; z++) {
        demandH += gcells[x][y][z].demandX;
    }
    return demandH;
}
float GRGrid::getGCellDemandV(int x, int y) {
    float demandV = 0;
    for (int z = 0; z < nz; z++) {
        demandV += gcells[x][y][z].demandY;
    }
    return demandV;
}
GCell* GRGrid::getGCell(int x, int y, int z) { return &gcells[x][y][z]; }

void GRGrid::initRGrids(int tx, int ty, int nlayer) {
    this->tx = tx;
    this->ty = ty;
    this->tz = (nlayer - 1) / 8 + 1;

    rgrids = (char*)calloc(tx * ty * tz, sizeof(char));

    long mem = sizeof(char) * tx * ty * tz;
    printlog(LOG_DEBUG, "grids = %d x %d x %d", tx, ty, tz);
    printlog(LOG_DEBUG, "mem = %s", show_mem_size(mem).c_str());
}

void GRGrid::setRGrid(int x, int y, int z, char v) {
    int i = x * (ty * tz) + y * tz + z / 8;
    int s = z % 8;
    if (v == 0) {
        rgrids[i] &= ~(1 << s);
    } else {
        rgrids[i] |= (1 << s);
    }
}

char GRGrid::getRGrid(int x, int y, int z) {
    int i = x * (ty * tz) + y * tz + z / 8;
    int s = z % 8;
    return (rgrids[i] >> s) & 1;
    // return (rgrids[x][y][z/8] >> z) & 1;
}

void GRGrid::addBlockage(int lx, int ly, int hx, int hy, int z) {
    int X3Q = trackStepX * 3 / 4;
    int Y3Q = trackStepY * 3 / 4;
    int XQ = trackStepX / 4;
    int YQ = trackStepY / 4;
    if (hx < trackL - X3Q || hy < trackB - Y3Q || lx > trackR + X3Q || ly > trackT + Y3Q) {
        return;
    }
    lx = max(trackL, lx);
    ly = max(trackB, ly);
    hx = min(trackR, hx);
    hy = min(trackT, hy);
    int startX = (lx - trackL + XQ) / trackStepX;
    int startY = (ly - trackB + YQ) / trackStepY;
    int endX = (hx - trackL + X3Q - 1) / trackStepX;
    int endY = (hy - trackB + X3Q - 1) / trackStepY;
    /*
    printlog(LOG_INFO, "block=%d:%d %d:%d tracks=%d:%d %d:%d",
            lx, hx, ly, hy, startX, endX, startY, endY);
            */
    for (int x = startX; x <= endX; x++) {
        for (int y = startY; y <= endY; y++) {
            setRGrid(x, y, z, (char)0);
        }
    }
}
