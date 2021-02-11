#include "gp_data.h"

#include "../db/db.h"
#include "gp_global.h"
using namespace gp;

int numNodes = 0;
int numCells = 0;
int numMacros = 0;
int numPads = 0;
int numFixed = 0;

int numNets = 0;
int numPins = 0;
int numFences = 0;

double coreLX, coreLY;
double coreHX, coreHY;
double siteW, siteH;
int numSitesX, numSitesY;

map<string, int> nameMap;
vector<string> cellName;

vector<vector<int> > netCell;
vector<vector<double> > netPinX;
vector<vector<double> > netPinY;
vector<double> netWeight;
vector<int> netNDR;
vector<vector<int> > cellNet;
vector<vector<int> > cellPin;

vector<double> cellX;
vector<double> cellY;
vector<double> cellW;
vector<double> cellH;
vector<double> cellAW;
vector<double> cellAH;

// coordinate of the nearest fence region
vector<double> cellFenceX;
vector<double> cellFenceY;
vector<double> cellFenceDist;
vector<int> cellFenceRect;
vector<int> cellFence;

vector<vector<double> > fenceRectLX;
vector<vector<double> > fenceRectLY;
vector<vector<double> > fenceRectHX;
vector<vector<double> > fenceRectHY;

// for routing
int numRGridX;
int numRGridY;
double RGridW;
double RGridH;
double RGridLX;
double RGridLY;
double RGridHX;
double RGridHY;

void gp_copy_layout_in();
void gp_copy_place_in();

void gp_copy_in(bool layout) {
    if (layout) {
        gp_copy_layout_in();
    } else {
        gp_copy_place_in();
    }
}

vector<db::Cell *> cellDBMap;

void gp_copy_layout_in() {
    double scale = 1.0 / (double)database.siteW;

    map<db::Cell *, int> dbCellMap;
    map<db::Region *, int> dbRegionMap;
    map<db::IOPin *, int> dbIOPinMap;

    coreLX = 0.0;
    coreLY = 0.0;
    coreHX = (double)(database.coreHX - database.coreLX) * scale;
    coreHY = (double)(database.coreHY - database.coreLY) * scale;
    siteW = database.siteW * scale;
    siteH = database.siteH * scale;
    numSitesX = database.nSitesX;
    numSitesY = database.nSitesY;

    numNodes = database.cells.size() + database.iopins.size();
    numCells = 0;
    numMacros = 0;
    numPads = database.iopins.size();
    numFixed = numPads;

    numNets = database.nets.size();
    numFences = database.regions.size();

    for (int i = 0; i < (int)database.cells.size(); i++) {
        if (database.cells[i]->fixed()) {
            numMacros++;
            numFixed++;
        } else {
            numCells++;
        }
    }
    for (int i = 0; i < (int)database.nets.size(); i++) {
        numPins += database.nets[i]->pins.size();
    }

    cellDBMap.resize(numCells, NULL);

    fenceRectLX.resize(numFences);
    fenceRectLY.resize(numFences);
    fenceRectHX.resize(numFences);
    fenceRectHY.resize(numFences);
    for (int i = 0; i < (int)database.regions.size(); i++) {
        db::Region *region = database.regions[i];
        dbRegionMap[region] = i;
        int nRects = region->rects.size();
        fenceRectLX[i].resize(nRects);
        fenceRectLY[i].resize(nRects);
        fenceRectHX[i].resize(nRects);
        fenceRectHY[i].resize(nRects);
        for (int r = 0; r < nRects; r++) {
            fenceRectLX[i][r] = (region->rects[r].lx - database.coreLX) * scale;
            fenceRectLY[i][r] = (region->rects[r].ly - database.coreLY) * scale;
            fenceRectHX[i][r] = (region->rects[r].hx - database.coreLX) * scale;
            fenceRectHY[i][r] = (region->rects[r].hy - database.coreLY) * scale;
        }
    }

    cellX.resize(numNodes, 0);
    cellY.resize(numNodes, 0);
    cellW.resize(numNodes, 0);
    cellH.resize(numNodes, 0);
    cellAW.resize(numNodes, 0);
    cellAH.resize(numNodes, 0);

    cellFenceX.resize(numCells, 0);
    cellFenceY.resize(numCells, 0);
    cellFenceDist.resize(numCells, 0);
    cellFenceRect.resize(numCells, 0);
    cellFence.resize(numCells, -1);

    netCell.resize(numNets);
    netPinX.resize(numNets);
    netPinY.resize(numNets);
    netWeight.resize(numNets, 1.0);
    netNDR.resize(numNets, -1);

    cellNet.resize(numNodes);
    cellPin.resize(numNodes);

    int c_i = 0;
    int m_i = numCells;
    int p_i = numCells + numMacros;
    for (int i = 0; i < (int)database.cells.size(); i++) {
        db::Cell *cell = database.cells[i];
        if (cell->fixed()) {
            dbCellMap[cell] = m_i;
            cellAW[m_i] = cellW[m_i] = (double)cell->width() * scale;
            cellAH[m_i] = cellH[m_i] = (double)cell->height() * scale;
            cellX[m_i] = (double)(cell->lx() - database.coreLX) * scale + cellW[m_i] * 0.5;
            cellY[m_i] = (double)(cell->ly() - database.coreLY) * scale + cellH[m_i] * 0.5;
            m_i++;
        } else {
            dbCellMap[cell] = c_i;
            cellDBMap[c_i] = cell;
            cellAW[c_i] = cellW[c_i] = (double)cell->width() * scale;
            cellAH[c_i] = cellH[c_i] = (double)cell->height() * scale;
            cellX[c_i] = (double)(cell->lx() - database.coreLX) * scale + cellW[c_i] * 0.5;
            cellY[c_i] = (double)(cell->ly() - database.coreLY) * scale + cellH[c_i] * 0.5;
            if (cell->region != NULL) {
                cellFence[c_i] = dbRegionMap[cell->region];
            } else {
                cellFence[c_i] = -1;
            }
            c_i++;
        }
    }
    for (int i = 0; i < (int)database.iopins.size(); i++) {
        db::IOPin *iopin = database.iopins[i];
        dbIOPinMap[iopin] = p_i;
        int lx, ly, hx, hy;
        iopin->type->getBounds(lx, ly, hx, hy);
        double cx = (hx + lx) / 2;
        double cy = (hy + ly) / 2;
        cellAW[p_i] = cellW[p_i] = 0.0;
        cellAH[p_i] = cellH[p_i] = 0.0;
        cellX[p_i] = (double)(iopin->x - database.coreLX + cx) * scale;
        cellY[p_i] = (double)(iopin->y - database.coreLY + cy) * scale;
        p_i++;
    }
    for (int i = 0; i < (int)database.nets.size(); i++) {
        db::Net *net = database.nets[i];
        if (net->ndr != NULL) {
            netNDR[i] = 0;
        }
        for (int j = 0; j < (int)net->pins.size(); j++) {
            db::Pin *pin = net->pins[j];
            if (pin == NULL) continue;
            if (pin->cell != NULL) {
                int lx, ly, hx, hy;
                pin->type->getBounds(lx, ly, hx, hy);
                double px = (hx + lx - pin->cell->width()) * 0.5 * scale;
                double py = (hy + ly - pin->cell->height()) * 0.5 * scale;
                netCell[i].push_back(dbCellMap[pin->cell]);
                netPinX[i].push_back(px);
                netPinY[i].push_back(py);
            } else {
                netCell[i].push_back(dbIOPinMap[pin->iopin]);
                netPinX[i].push_back(0.0);
                netPinY[i].push_back(0.0);
            }
        }
    }

    numRGridX = database.grGrid.gcellNX;
    numRGridY = database.grGrid.gcellNY;
    RGridW = database.grGrid.gcellStepX * scale;
    RGridH = database.grGrid.gcellStepY * scale;
    RGridLX = (database.grGrid.gcellL - database.coreLX) * scale;
    RGridLY = (database.grGrid.gcellB - database.coreLY) * scale;
    RGridHX = (database.grGrid.gcellR - database.coreLX) * scale;
    RGridHY = (database.grGrid.gcellT - database.coreLY) * scale;

    // shift io pins such that all io pins are routable
    for (int i = numCells + numMacros; i < numNodes; i++) {
        if (cellX[i] < RGridLX) cellX[i] = RGridLX;
        if (cellX[i] > RGridHX) cellX[i] = RGridHX;
        if (cellY[i] < RGridLY) cellY[i] = RGridLY;
        if (cellY[i] > RGridHY) cellY[i] = RGridHY;
    }

    //  GPModule::TargetDensityBegin = max(GPModule::TargetDensityBegin, database.tdBins.getTDBin());
    GPModule::TargetDensityEnd = database.tdBins.getTDBin();
    GPModule::TargetDensityBegin = GPModule::TargetDensityEnd;
}

void gp_copy_place_in() {
    double scale = 1.0 / (double)database.siteW;
    for (int i = 0; i < numCells; i++) {
        db::Cell *cell = cellDBMap[i];
        cellX[i] = (cell->lx() - database.coreLX) * scale + 0.5 * cellAW[i];
        cellY[i] = (cell->ly() - database.coreLY) * scale + 0.5 * cellAH[i];
    }
}

void gp_copy_out() {
    double scale = database.siteW;
    for (int i = 0; i < numCells; i++) {
        db::Cell *cell = cellDBMap[i];
        int x = lround((cellX[i] - 0.5 * cellAW[i]) * scale) + database.coreLX;
        int y = lround((cellY[i] - 0.5 * cellAH[i]) * scale) + database.coreLY;
        cell->place(x, y);
    }
}

double hpwl() {
    double lenx = 0;
    double leny = 0;
    for (int i = 0; i < numNets; i++) {
        int nPins = netCell[i].size();
        if (nPins < 2) {
            continue;
        }
        double lx, ly, hx, hy;
        int cell = netCell[i][0];
        lx = hx = cellX[cell] + netPinX[i][0];
        ly = hy = cellY[cell] + netPinY[i][0];
        for (int p = 1; p < nPins; p++) {
            cell = netCell[i][p];
            double px = cellX[cell] + netPinX[i][p];
            double py = cellY[cell] + netPinY[i][p];
            lx = min(lx, px);
            ly = min(ly, py);
            hx = max(hx, px);
            hy = max(hy, py);
        }
        lenx += hx - lx;
        leny += hy - ly;
    }
    return lenx + leny;
}
