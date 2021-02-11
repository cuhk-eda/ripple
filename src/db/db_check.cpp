#include "db.h"
using namespace db;

#include "../ut/utils.h"

void Database::checkPlaceError() {
    printlog(LOG_INFO, "starting checking...");
    int nError = 0;
    vector<Cell *> cells = this->cells;
    sort(cells.begin(), cells.end(), [](const Cell *a, const Cell *b) { return a->lx() < b->lx(); });
    int nCells = cells.size();
    for (int i = 0; i < nCells; i++) {
        Cell *cell_i = cells[i];
        // int lx = cell_i->x;
        int hx = cell_i->hx();
        int ly = cell_i->ly();
        int hy = cell_i->hy();
        for (int j = i + 1; j < nCells; j++) {
            Cell *cell_j = cells[j];
            if (cell_j->lx() >= hx) {
                break;
            }
            if (cell_j->ly() >= hy || cell_j->hy() <= ly) {
                continue;
            }
            nError++;
        }
    }

    printlog(LOG_INFO, "#overlap=%d", nError);
}

void Database::checkDRCError() {
    printlog(LOG_INFO, "starting checking...");
    vector<int> nOverlapErrors(3);
    vector<int> nSpacingErrors(3);

    class Metal {
    public:
        Rectangle rect;
        const Cell *cell;
        const IOPin *iopin;
        const SNet *snet;
    };

    vector<int> minSpace(3);
    vector<vector<Metal>> metals(3);

    for (unsigned i = 0; i != 3; ++i) {
        minSpace[i] = getRLayer(i)->spacing;
    }

    for (const Cell *cell : cells) {
        unsigned nPins = cell->numPins();
        for (unsigned j = 0; j != nPins; ++j) {
            Pin *pin = cell->pin(j);
            for (const Geometry &geo : pin->type->shapes) {
                Metal metal;
                metal.rect.lx = cell->lx() + geo.lx;
                metal.rect.hx = cell->lx() + geo.hx;
                metal.rect.ly = cell->ly() + geo.ly;
                metal.rect.hy = cell->ly() + geo.hy;
                metal.cell = cell;
                metal.iopin = nullptr;
                metal.snet = nullptr;
                const int rIndex = geo.layer.rIndex;
                if (rIndex >= 0 && rIndex <= 2) {
                    metals[rIndex].push_back(metal);
                }
            }
        }
    }

    for (const IOPin *iopin : iopins) {
        for (const Geometry &geo : iopin->type->shapes) {
            Metal metal;
            metal.rect.lx = iopin->x + geo.lx;
            metal.rect.hx = iopin->x + geo.hx;
            metal.rect.ly = iopin->y + geo.ly;
            metal.rect.hy = iopin->y + geo.hy;
            metal.cell = nullptr;
            metal.iopin = iopin;
            metal.snet = nullptr;
            const int rIndex = geo.layer.rIndex;
            if (rIndex >= 0 && rIndex <= 2) {
                metals[rIndex].push_back(metal);
            }
        }
    }

    for (const SNet *snet : snets) {
        for (const Geometry &geo : snet->shapes) {
            Metal metal;
            metal.rect.lx = geo.lx;
            metal.rect.hx = geo.hx;
            metal.rect.ly = geo.ly;
            metal.rect.hy = geo.hy;
            metal.cell = nullptr;
            metal.iopin = nullptr;
            metal.snet = snet;
            const int rIndex = geo.layer.rIndex;
            if (rIndex >= 0 && rIndex <= 2) {
                metals[rIndex].push_back(metal);
            }
        }
    }

    for (unsigned i = 0; i != 3; ++i) {
        printlog(LOG_INFO, "m%d = %u", i + 1, metals[i].size());
        sort(metals[i].begin(), metals[i].end(), [](const Metal &a, const Metal &b) {
            return (a.rect.lx == b.rect.lx) ? (a.rect.ly < b.rect.ly) : (a.rect.lx < b.rect.lx);
        });
    }

    for (unsigned L = 0; L != 3; ++L) {
        unsigned nMetals = metals[L].size();
        int minS = minSpace[L];
        for (unsigned i = 0; i != nMetals; ++i) {
            Metal &mi = metals[L][i];
            // int lx = mi.rect.lx;
            int hx = mi.rect.hx;
            int ly = mi.rect.ly;
            int hy = mi.rect.hy;
            for (unsigned j = i + 1; j != nMetals; ++j) {
                const Metal &mj = metals[L][j];
                if (mj.rect.lx - minS >= hx) {
                    break;
                }
                if (mj.rect.ly - minS >= hy || mj.rect.hy + minS <= ly) {
                    continue;
                }
                if ((mi.cell != NULL && mi.cell == mj.cell) || (mi.iopin != NULL && mi.iopin == mj.iopin) ||
                    (mi.snet != NULL && mi.snet == mj.snet)) {
                    continue;
                }
                ++(nSpacingErrors[L]);
                if (mj.rect.lx >= hx) {
                    continue;
                }
                if (mj.rect.ly >= hy || mj.rect.hy <= ly) {
                    continue;
                }
                ++(nOverlapErrors[L]);
            }
        }
    }

    /*
    vector<Cell*> cells = this->cells;
    sort(cells.begin(), cells.end(), Cell::CompareXInc);
    int nCells = cells.size();
    for(int i=0; i<nCells; i++){
        Cell *cell_i = cells[i];
        //int lx = cell_i->x;
        int hx = cell_i->x + cell_i->width();
        int ly = cell_i->y;
        int hy = cell_i->y + cell_i->height();
        for(int j=i+1; j<nCells; j++){
            Cell *cell_j = cells[j];
            if(cell_j->x >= hx){
                break;
            }
            if(cell_j->y >= hy || cell_j->y + cell_j->height() <= ly){
                continue;
            }
            nError++;
        }
    }
    */

    for (unsigned i = 0; i != 3; ++i) {
        printlog(LOG_INFO, "#M%u overlaps = %d", i + 1, nOverlapErrors[i]);
        printlog(LOG_INFO, "#M%u spacings = %d", i + 1, nSpacingErrors[i]);
    }
}
