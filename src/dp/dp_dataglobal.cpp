#include "dp.h"
#include "dp_data.h"
using namespace dp;

#include <cassert>
#include <cfloat>
using namespace std;

/***** OptimalRegion *****/

void Region::expand(const int l, const int r, const int b, const int t, const db::Region* reg) {
    int rlx = DPModule::dplacer->coreLX;
    int rhx = DPModule::dplacer->coreHX;
    int rly = DPModule::dplacer->coreLY;
    int rhy = DPModule::dplacer->coreHY;
    if (reg) {
        static const int clx = database.coreLX;
        static const int cly = database.coreLY;
        static const int csw = database.siteW;
        static const int csh = database.siteH;
        rlx = ceil((reg->lx - clx) / static_cast<double>(csw));
        rhx = floor((reg->hx - clx) / static_cast<double>(csw));
        rly = ceil((reg->ly - cly) / static_cast<double>(csh));
        rhy = floor((reg->hy - cly) / static_cast<double>(csh));
    }
    if (lx == rlx && ly == rly && hx == rhx && hy == rhy) {
        _isMax = true;
        return;
    }

    lx = max(rlx, min(rhx - l - r, lx - l));
    ly = max(rly, min(rhy - b - t, ly - b));
    hx = min(rhx, max(rlx + l + r, hx + r));
    hy = min(rhy, max(rly + b + t, hy + t));
}

void Region::shift(int dx, int dy) {
    if (lx + dx < DPModule::dplacer->coreLX) {
        dx = DPModule::dplacer->coreLX - lx;
    } else if (hx + dx > DPModule::dplacer->coreHX) {
        dx = DPModule::dplacer->coreHX - hx;
    }
    if (ly + dy < DPModule::dplacer->coreLY) {
        dy = DPModule::dplacer->coreLY - ly;
    } else if (hy + dy > DPModule::dplacer->coreHY) {
        dy = DPModule::dplacer->coreHY - hy;
    }
    lx += dx;
    hx += dx;
    ly += dy;
    hy += dy;
}

void Region::cover(const int x, const int y) {
    lx = min(lx, x);
    ly = min(ly, y);
    hx = max(hx, x);
    hy = max(hy, y);
}

int Region::distance(const int x, const int y) {
    int dist = 0;
    if (x < lx) {
        dist = lx - x;
    } else if (x > hx) {
        dist = x - hx;
    }
    if (y < ly) {
        dist += ly - y;
    } else if (y > hy) {
        dist += y - hy;
    }
    return dist;
}

void Region::constraint(const int lx, const int ly, const int hx, const int hy) {
    if (this->lx < lx) {
        int shift = lx - this->lx;
        this->lx += shift;
        this->hx += shift;
    } else if (this->hx > hx) {
        int shift = this->hx - hx;
        this->lx -= shift;
        this->hx -= shift;
    }
    if (this->ly < ly) {
        int shift = ly - this->ly;
        this->ly += shift;
        this->hy += shift;
    } else if (this->hy > hy) {
        int shift = this->hy - hy;
        this->ly -= shift;
        this->hy -= shift;
    }
}

/***** Cell *****/

vector<vector<int> > Cell::_spaceLUT;
vector<unsigned> Cell::_spaceMax;

void Cell::getLocalRegion(Region& region, const int rangeX, const int rangeY) const {
    region.lx = _x.load(memory_order_acquire);
    region.hx = _x.load(memory_order_acquire) + w;
    region.ly = _y.load(memory_order_acquire);
    region.hy = _y.load(memory_order_acquire) + h;
    int l = rangeX;
    int r = rangeX;
    int b = rangeY;
    int t = rangeY;
    region.expand(l, r, b, t, database.regions[fid]);
}

void Cell::getOriginalRegion(Region& region, const int rangeX, const int rangeY) const {
    region.lx = oxLong();
    region.ly = oyLong();
    region.hx = oxLong() + w;
    region.hy = oyLong() + h;
    region.expand(rangeX, rangeX, rangeY, rangeY, database.regions[fid]);
}

void Cell::getOptimalRegion(Region& region) const {
    int nPins = pins.size();
    if (nPins == 0) {
        region.lx = DPModule::dplacer->coreLX;
        region.ly = DPModule::dplacer->coreLY;
        region.hx = DPModule::dplacer->coreHX - w;
        region.hy = DPModule::dplacer->coreHY - h;
        return;
    }
    vector<int> boundx(pins.size() * 2);
    vector<int> boundy(pins.size() * 2);
    vector<Pin*>::const_iterator cpi = pins.cbegin();
    vector<Pin*>::const_iterator cpe = pins.cend();
    for (int i = 0; cpi != cpe; ++cpi, i++) {
        const Pin* cellpin = *cpi;
        Region& netbound = cellpin->net->box;
        int cpx = cellpin->pinX();
        int cpy = cellpin->pinY();
        int lx = DPModule::dplacer->coreHX * DPModule::dplacer->siteW;
        int ly = DPModule::dplacer->coreHY * DPModule::dplacer->siteH;
        int hx = DPModule::dplacer->coreLX * DPModule::dplacer->siteW;
        int hy = DPModule::dplacer->coreLY * DPModule::dplacer->siteH;
        if (nPins > 2 && netbound.lx != cpx && netbound.hx != cpx && netbound.ly != cpy && netbound.hy != cpy) {
            lx = netbound.lx;
            ly = netbound.ly;
            hx = netbound.hx;
            hy = netbound.hy;
        } else {
            vector<Pin*>::const_iterator npi = cellpin->net->pins.cbegin();
            vector<Pin*>::const_iterator npe = cellpin->net->pins.cend();
            for (; npi != npe; ++npi) {
                Pin* netpin = *npi;
                if (netpin->cell == this) {
                    continue;
                }
                int px = netpin->pinX();
                int py = netpin->pinY();
                lx = min(px, lx);
                ly = min(py, ly);
                hx = max(px, hx);
                hy = max(py, hy);
            }
        }
        boundx[i * 2] = lx - cellpin->x;
        boundy[i * 2] = ly - cellpin->y;
        boundx[i * 2 + 1] = hx - cellpin->x;
        boundy[i * 2 + 1] = hy - cellpin->y;
    }

    nth_element(boundx.begin(), boundx.begin() + (nPins - 1), boundx.end());
    nth_element(boundy.begin(), boundy.begin() + (nPins - 1), boundy.end());
    region.lx = (int)floor((double)boundx[nPins - 1] / DPModule::dplacer->siteW);
    region.ly = (int)floor((double)boundy[nPins - 1] / DPModule::dplacer->siteH);
    /*
    int rx = *min_element(boundx.begin() + nPins, boundx.end());
    int ry = *min_element(boundy.begin() + nPins, boundy.end());
    region.hx = (int)ceil((double)rx / DPModule::dplacer.siteW);
    region.hy = (int)ceil((double)ry / DPModule::dplacer.siteH);
    */
    nth_element(boundx.begin(), boundx.begin() + nPins, boundx.end());
    nth_element(boundy.begin(), boundy.begin() + nPins, boundy.end());
    region.hx = (int)ceil((double)boundx[nPins] / DPModule::dplacer->siteW);
    region.hy = (int)ceil((double)boundy[nPins] / DPModule::dplacer->siteH);

    region.lx = max(DPModule::dplacer->coreLX, min(DPModule::dplacer->coreHX - (int)w, region.lx));
    region.ly = max(DPModule::dplacer->coreLY, min(DPModule::dplacer->coreHY - (int)h, region.ly));
    region.hx = max(DPModule::dplacer->coreLX, min(DPModule::dplacer->coreHX - (int)w, region.hx));
    region.hy = max(DPModule::dplacer->coreLY, min(DPModule::dplacer->coreHY - (int)h, region.hy));
}
void Cell::getImproveRegion(Region& region, Region& optimalRegion) const {
    if (optimalRegion.empty()) {
        getOptimalRegion(optimalRegion);
    }
    region = optimalRegion;
    region.cover(_x.load(memory_order_acquire), _y.load(memory_order_acquire));
}
Region Cell::getLocalRegion(const int rangeX, const int rangeY) const {
    Region region;
    getLocalRegion(region, rangeX, rangeY);
    return region;
}
Region Cell::getOriginalRegion(const int rangeX, const int rangeY) const {
    Region region;
    getOriginalRegion(region, rangeX, rangeY);
    return region;
}
Region Cell::getOptimalRegion() const {
    Region region;
    getOptimalRegion(region);
    return region;
}
Region Cell::getImproveRegion() const {
    Region region;
    Region optimalRegion;
    getImproveRegion(region, optimalRegion);
    return region;
}
Region Cell::getImproveRegion(Region& optimalRegion) const {
    Region region;
    getImproveRegion(region, optimalRegion);
    return region;
}

int Cell::getSpace(const int lType, const char lFx, const int rType, const char rFx) {
    if (_spaceLUT.empty()) {
        return 0;
    }
    return _spaceLUT[lType * 2 + lFx][rType * 2 + rFx];
}

int Cell::getSpaceMax(const int type) {
    if (_spaceMax.empty()) {
        return 0;
    }
    return _spaceMax[type];
}

void Cell::setSpace(const vector<db::CellType*>& dbCellTypes) {
    _spaceLUT.clear();
    _spaceMax.clear();
    unsigned nDPTypes = dbCellTypes.size();
    if (nDPTypes < 500) {
        _spaceLUT.resize(nDPTypes * 2, vector<int>(nDPTypes * 2, 0));
        _spaceMax.resize(nDPTypes, 0);
        unsigned smm = 0;
        for (unsigned i = 0; i != nDPTypes; ++i) {
            unsigned s0 = 0;
            unsigned s1 = 0;
            unsigned s2 = 0;
            unsigned s3 = 0;
            unsigned sm = 0;
            for (unsigned j = 0; j != nDPTypes; ++j) {
                db::CellType* L = dbCellTypes[i];
                db::CellType* R = dbCellTypes[j];
                s0 = database.edgetypes.getEdgeSpace(L->edgetypeR, R->edgetypeL) / database.siteW;
                s1 = database.edgetypes.getEdgeSpace(L->edgetypeR, R->edgetypeR) / database.siteW;
                s2 = database.edgetypes.getEdgeSpace(L->edgetypeL, R->edgetypeL) / database.siteW;
                s3 = database.edgetypes.getEdgeSpace(L->edgetypeL, R->edgetypeR) / database.siteW;
                _spaceLUT[i * 2][j * 2] = s0;
                _spaceLUT[i * 2][j * 2 + 1] = s1;
                _spaceLUT[i * 2 + 1][j * 2] = s2;
                _spaceLUT[i * 2 + 1][j * 2 + 1] = s3;
                sm = max(sm, max(s0, max(s1, max(s2, s3))));
            }
            _spaceMax[i] = sm;
            smm = max(smm, sm);
        }
#ifndef NDEBUG
        printlog(LOG_INFO, "max edge space is %u", smm);
#endif
    } else {
        printlog(LOG_ERROR, "number of dp types is %u", nDPTypes);
    }
}

/***** Pin *****/
int Pin::pinX() const {
    if (!cell) {
        return x;
    }
    const int siteW = DPModule::dplacer->siteW;
    if (cell->fx()) {
        return (cell->lx() + cell->w) * siteW - x;  // x * sw + ( w * sw - x )
    }
    return cell->lx() * siteW + x;
}

int Pin::pinY() const {
    if (!cell) {
        return y;
    }
    const int siteH = DPModule::dplacer->siteH;
    if (cell->fy()) {
        return (cell->ly() + cell->h) * siteH - y;  // y * sh + ( h * sh - y )
    }
    return cell->ly() * siteH + y;
}

/***** Net *****/
Net::Net() { i = -1; }
void Net::update() {
    box.lx = box.ly = INT_MAX;
    box.hx = box.hy = INT_MIN;
    for (auto pin : pins) {
        int px = pin->pinX();
        int py = pin->pinY();
        box.cover(px, py);
    }
}

long long Net::hpwl() const { return box.width() + box.height(); }

/***** Segment *****/
Segment::Segment(int i, int x /* = 0*/, int y /* = 0*/, unsigned w /* = 0*/)
    : i(i), x(x), y(y), w(w), fid(0), flip(false), cells(), mtx() {}

Segment::Segment(const Segment& s) : i(s.i), x(s.x), y(s.y), w(s.w), fid(s.fid), flip(s.flip), cells(s.cells), mtx() {}

void Segment::addCell(Cell* cell) {
    assert(cell->ly() <= y);
    assert(cell->hy() > y);
    assert(cell->lx() >= x);
    assert(cell->hx() <= x + (int)w);
    cells.addCell(cell);
    cell->segments[y - cell->ly()] = this;
    // space -= cell->w;
}

void Segment::removeCell(Cell* cell) {
    assert(cell->segments[this->y - cell->ly()] == this);
    cells.removeCell(cell);
    cell->segments[this->y - cell->ly()] = nullptr;
    cell->placed = false;
    // space += cell->w;
}

unsigned Segment::setLocalRegion(LocalRegion& localRegion, unsigned& nLocalCells, const int fid) {
    CellListIter iterL, iterR;
    Cell* cellL = nullptr;
    Cell* cellR = nullptr;
    CellListIter ci;

    const int lx = localRegion.lx;
    const int ly = localRegion.ly;
    const int hx = localRegion.hx;
    const int hy = localRegion.hy;

    unsigned r = y - ly;
    LocalSegment& localSeg = localRegion.localSegments[r];

    //  cout << "try lock seg\n";
    shared_lock<shared_mutex> lock(mtx);
    //  cout << "success lock seg\n";
    getCellAt(lx, y, cellL, cellR, iterL, iterR);
    if (!cellL) {
        ci = iterR;
    } else {
        ci = iterL;
    }

    unsigned totalCellArea = 0;

    if (ci != cells.begin()) {
        Cell* cell = cells.getCell(ci - 1);
        if (cell->hx() + cell->getSpaceMax() > lx) {
            const int fixX = lx;
            const int fixY = y;
            const int fixW = cell->hx() - lx;
            const int fixH = 1;
            localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, max(0, fixW), fixH, cell->getDBCellType(), 0, max(0, -fixW));
            localSeg.localCells[localSeg.nCells++] = nLocalCells++;
            totalCellArea += max(0, fixW);
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO, "add fix cell b %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
            }
        }
    }

    // CellListIter ci = seg.cells.begin();
    CellListIter ce = cells.end();

    for (; ci != ce; ++ci) {
        Cell* cell = cells.getCell(ci);
        // Cell *cell = *ci;
        if (cell->fid != fid) {
            continue;
        }
        if (cell->hx() <= lx) {
            if (cell->hx() + cell->getSpaceMax() > lx) {
                localRegion.localCells[nLocalCells] =
                    LocalCell(lx, y, 0, 1, cell->getDBCellType(), 0, lx - cell->hx());
                localSeg.localCells[localSeg.nCells++] = nLocalCells++;
                if (localRegion.useCount == DEBUG_ID) {
                    printlog(
                        LOG_INFO, "add fix cell b %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
                }
                //  }
            }
            continue;
        }
        if (cell->lx() >= hx) {
            /*
                    int fixX = cell->lx() - cell->getSpaceMax();
                    if (nLocalCells) {
                        const LocalCell& localCell = localRegion.localCells[nLocalCells - 1];
                        if (localCell.y == ry && localCell.fixed) {
                            fixX = max(fixX, localCell.x + localCell.w);
                        }
                    }
                    */
            if (cell->lx() - cell->getSpaceMax() < hx) {
                localRegion.localCells[nLocalCells] =
                    LocalCell(hx, y, 0, 1, cell->getDBCellType(), cell->lx() - hx, 0);
                localSeg.localCells[localSeg.nCells++] = nLocalCells++;
                if (localRegion.useCount == DEBUG_ID) {
                    printlog(
                        LOG_INFO, "add fix cell b %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
                }
            }
            break;
        }

        if (cell->ly() > y) {
            printlog(LOG_ERROR, "ly=%d ry=%d", cell->ly(), y);
        }
        if (cell->hy() - 1 < y) {
            printlog(LOG_ERROR, "hy=%d ry=%d", cell->hy(), y);
        }

        // overlap cells
        if (cell->lx() < lx && cell->hx() > lx) {
            int fixX = lx;
            /*
                    if (nLocalCells) {
                        const LocalCell& localCell = localRegion.localCells[nLocalCells - 1];
                        if (localCell.y == ry && localCell.fixed) {
                            fixX = max(fixX, localCell.x + localCell.w);
                        }
                    }
                    */
            int fixY = y;
            int fixW = min(cell->hx(), hx) - fixX;
            int fixH = 1;
            localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH, cell->getDBCellType(), 0, 0);
            localSeg.localCells[localSeg.nCells++] = nLocalCells++;
            totalCellArea += fixW;
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO, "add fix cell b %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
            }
            continue;
        }
        if (cell->hx() > hx && cell->lx() < hx) {
            int fixX = max(cell->lx(), lx);
            int fixY = y;
            int fixW = hx - fixX;
            int fixH = 1;
            localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH, cell->getDBCellType(), 0, 0);
            localSeg.localCells[localSeg.nCells++] = nLocalCells++;
            totalCellArea += fixW;
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO, "add fix cell c %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
            }
            continue;
        }
        if (cell->ly() < ly || cell->hy() > hy) {
            const int fixX = max(cell->lx(), lx);
            const int fixY = y;
            const int fixW = min(cell->hx(), hx) - fixX;
            const int fixH = 1;
            localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH, cell->getDBCellType(), 0, 0);
            localSeg.localCells[localSeg.nCells++] = nLocalCells++;
            totalCellArea += fixW;
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO, "add fix cell d %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
            }
            continue;
        }

        // real local cells
        if (cell->ly() != y) {
            localSeg.localCells[localSeg.nCells++] = localRegion.localCellMap[cell->i];
            totalCellArea += cell->w;
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO, "ly=%d ry=%d", cell->ly(), y);
                printlog(LOG_INFO,
                         "add local cell a %d to row %d (%d), parent id = %d",
                         localRegion.localCellMap[cell->i],
                         localSeg.y,
                         localSeg.nCells,
                         cell->i);
            }
        } else {
            localRegion.localCellMap[cell->i] = nLocalCells;
            localRegion.localCells[nLocalCells] = LocalCell(cell);
            localSeg.localCells[localSeg.nCells++] = nLocalCells++;
            totalCellArea += cell->w;
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO,
                         "add local cell b %d to row %d (%d), parent id = %d",
                         nLocalCells - 1,
                         localSeg.y,
                         localSeg.nCells,
                         cell->i);
            }
        }
    }
    return totalCellArea;
}

/***** CellMove *****/

bool CellMove::isFlipX() const {
    switch (_orient) {
        case 0:
            return false;  // N
        case 2:
            return true;  // S
        case 4:
            return true;  // FN
        case 6:
            return false;  // FS
    }
    return false;
}
bool CellMove::isFlipY() const {
    switch (_orient) {
        case 0:
            return false;  // N
        case 2:
            return true;  // S
        case 4:
            return false;  // FN
        case 6:
            return true;  // FS
    }
    return false;
}

/***** Move *****/
Move::Move(Cell* cell, const float x, const float y, const int orient) : target(cell, x, y, orient) { legals.reserve(20); }

long long Move::displacementFromInput(int targetWeight) {
    double dispx = abs(target.cell->ox() - target.x()) * targetWeight;
    double dispy = abs(target.cell->oy() - target.y()) * targetWeight;
    for (const auto& move : legals) {
        dispx += abs(move.cell->ox() - move.x());
        dispy += abs(move.cell->oy() - move.y());
    }
    return llround(dispx * DPModule::dplacer->siteW + dispy * DPModule::dplacer->siteH);
}

long long Move::displacement(int targetWeight) {
    double dispx = abs(target.cell->tx() - target.x()) * targetWeight;
    double dispy = abs(target.cell->ty() - target.y()) * targetWeight;
    for (const auto& move : legals) {
        dispx += abs(move.cell->lx() - move.x());
        dispy += abs(move.cell->ly() - move.y());
    }
    return llround(dispx * DPModule::dplacer->siteW + dispy * DPModule::dplacer->siteH);
}

Move Move::inverse() {
    Move move = *this;
    move.target.x(target.cell->lx());
    move.target.y(target.cell->ly());
    for (CellMove& legal : move.legals) {
        legal.x(legal.cell->lx());
        legal.y(legal.cell->ly());
    }
    return move;
}

double Density::getOverFlow(const vector<unsigned>& cellArea, const double densityLimit, int fid) const {
    if (nRegions == 1) {
        fid = 0;
    }
    int totalBinCellArea = 0;
    int totalWhiteSpaceArea = 0;
    int totalCellArea = 0;
    double temp = 0.0;
    if (fid < 0) {
        for (int j = 0; j < nBinsX; j++) {
            for (int k = 0; k < nBinsY; k++) {
                for (int i = 0; i < nRegions; i++) {
                    totalBinCellArea += binCellArea[i][j][k];
                    totalWhiteSpaceArea += binFreeArea[i][j][k];
                }
            }
        }
        for (int i = 0; i < nRegions; i++) {
            totalCellArea += (int)cellArea[i];
        }

    } else {
        for (int j = 0; j < nBinsX; j++) {
            for (int k = 0; k < nBinsY; k++) {
                totalBinCellArea += binCellArea[fid][j][k];
                totalWhiteSpaceArea += binFreeArea[fid][j][k];
            }
        }
        totalCellArea = (int)cellArea[fid];
    }
    if (totalCellArea == 0) {
        return 0.0;
    }
    temp = (double)totalBinCellArea - ((double)totalWhiteSpaceArea) * densityLimit;
    if (temp < 0.0) {
        temp = 0.0;
    }
    return temp / (double)totalCellArea;
}
