#include "dp_data.h"
#include "dp_global.h"
using namespace dp;

/***** LocalCell *****/
LocalCell::LocalCell(Cell* cell)
    : parent(cell)
    , x(cell->lx())
    , y(cell->ly())
    , w(cell->w)
    , h(cell->h)
    , row(cell->rowPlace())
    , edgespaceL()
    , edgespaceR()
{
    if (DPModule::MLLDispFromInput) {
        _ox = cell->ox();
        _oy = cell->oy();
    } else {
        _ox = cell->lx();
        _oy = cell->ly();
    }
}

LocalCell::LocalCell(const int ix, const int iy, const int iw, const unsigned ih, const db::CellType* type, const unsigned l, const unsigned r)
    : _ox(ix)
    , _oy(iy)
    , x(ix)
    , y(iy)
    , w(iw)
    , h(ih)
    , fixed(true)
    , edgespaceL(database.edgetypes.distTable.size(), 0)
    , edgespaceR(database.edgetypes.distTable.size(), 0) {
    const vector<vector<int>>& distTable = database.edgetypes.distTable;
    if (type) {
        const int siteW = database.siteW;
        for (unsigned i = 0; i != distTable.size(); ++i) {
            edgespaceL[i] = max<int>(l, distTable[type->edgetypeL][i] / siteW) - l;
            edgespaceR[i] = max<int>(r, distTable[type->edgetypeR][i] / siteW) - r;
        }
    }
}

/***** LocalSegment *****/
LocalSegment::LocalSegment(Segment* segment, int lx, int hx)
    : y(segment->y)
    , parent(segment)
{
    x = max(lx, segment->x);
    w = min(hx, segment->x + (int)segment->w) - x;
}

/***** LocalRegion *****/
void LocalRegion::clear() {
    nCells = 0;
    nSegments = 0;
}

void LocalRegion::reset()
{
    if (DPModule::MLLDispFromInput) {
        for (unsigned i = 0; i < nCells; i++) {
            LocalCell& cell = localCells[i];
            if (cell.fixed) {
                continue;
            }
            cell.x = cell.parent->lx();
            cell.y = cell.parent->ly();
        }
    } else {
        for (unsigned i = 0; i < nCells; i++) {
            LocalCell& cell = localCells[i];
            if (cell.fixed) {
                continue;
            }

            cell.x = cell.oxLong();
            cell.y = cell.oyLong();
        }
    }
}

void LocalRegion::placeL(vector<int>& posL, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
#ifndef NEW_PLACEL_PLACER
    //Data structure:
    //- fronts : pointing to the first cell
    //- ends : pointing to the end (NULL)
    //- LBounds : x-coor of the last placed cell's right / the boundary
    vector<int> fronts(nSegments);
    vector<int> ends(nSegments);
    vector<int> LBounds(nSegments);
    for (unsigned r = 0; r < nSegments; r++) {
        fronts[r] = 0;
        ends[r] = localSegments[r].nCells;
        LBounds[r] = lx; //localSegments[r].x;
    }
    for (bool finished = false; !finished;) {
        finished = true;
        for (unsigned r = 0; r < nSegments; r++) {
            while (fronts[r] != ends[r]) {
                finished = false;
                const LocalCell& cell = localCells[localSegments[r].localCells[fronts[r]]];
                if (cell.h == 1) {
                    //single-row cell, just push to left
                    if (cell.fixed) {
                        posL[cell.i] = cell.x;
                    } else {
                        int lb = LBounds[r];
                        for (; !_isMax && lb < hx && cellTypeMaps[cell.parent->type].vScore(lb); ++lb);
                        posL[cell.i] = lb;
                    }
                } else if (cell.y != localSegments[r].y) {
                    //multi-row cell not on this row, ignore it for this moment
                    break;
                } else {
                    //multi-row cell, check if any cell before it is not placed
                    bool blocked = false;
                    for (unsigned s = 1; s < cell.h; ++s) {
                        if (localSegments[r + s].localCells[fronts[r + s]] != cell.i) {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked) {
                        break;
                    } else if (cell.fixed) {
                        posL[cell.i] = cell.x;
                    } else {
                        int lb = LBounds[r];
                        for (unsigned s = 1; s < cell.h; ++s) {
                            lb = max(lb, LBounds[r + s]);
                        }
                        for (; !_isMax && lb < hx && cellTypeMaps[cell.parent->type].vScore(lb); ++lb);
                        posL[cell.i] = lb;
                    }
                }
                for (unsigned s = 0; s != cell.h; ++s) {
                    if (fronts[r + s] + 1 != ends[r + s]) {
                        Cell* parent = localCells[localSegments[r + s].localCells[fronts[r + s] + 1]].parent;
                        if (parent) {
                            int edgetypeL = parent->getDBCellType()->edgetypeL;
                            if (cell.fixed) {
                                LBounds[r + s] = posL[cell.i] + cell.w + cell.edgespaceR[edgetypeL];
                            } else {
                                LBounds[r + s] = posL[cell.i] + cell.w + Cell::getSpace(cell.parent, parent);
                            }
                        }
                    }
                    ++fronts[r + s];
                }
            }
        }
    }
#else
    vector<int> LBounds(nSegments, lx);
    for (int i = 0; i < nCells; i++) {
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
        LocalCell& cell = localCells[localCellList[i]];
#endif
#ifdef NEW_PLACEL_PLACER_USE_SORTED_CELLS
        LocalCell& cell = localCells[i];
#endif
        if (cell.fixed) {
            posL[cell.i] = cell.x;
        } else {
            int L = LBounds[cell.y - ly];
            for (int s = 1; s < cell.h; s++) {
                L = max(L, LBounds[cell.y - ly + s]);
            }
            posL[cell.i] = L;
        }
        for (int s = 0; s < cell.h; s++) {
            LBounds[cell.y - ly + s] = posL[cell.i] + cell.w;
        }
    }
#endif
}

void LocalRegion::placeR(vector<int>& posR, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
#ifndef NEW_LEFT_RIGHT_PLACE
    //Data structure:
    //- fronts : pointing to the last cell
    //- ends : pointing to the begining (NULL)
    //- RBounds : x-coor of the last placed cell's left / the boundary
    vector<int> fronts(nSegments);
    vector<int> ends(nSegments);
    vector<int> RBounds(nSegments);
    for (unsigned r = 0; r < nSegments; r++) {
        fronts[r] = localSegments[r].nCells - 1;
        ends[r] = -1;
        RBounds[r] = hx; //localSegments[r].x + localSegments[r].w;
    }
    for (bool finished = false; !finished;) {
        finished = true;
        for (unsigned r = 0; r < nSegments; r++) {
            while (fronts[r] != ends[r]) {
                finished = false;
                LocalCell& cell = localCells[localSegments[r].localCells[fronts[r]]];
                if (cell.h == 1) {
                    //single-row cell, just push to right
                    if (cell.fixed) {
                        posR[cell.i] = cell.x;
                    } else {
                        int rb = RBounds[r] - cell.w;
                        for (; !_isMax && rb > lx && cellTypeMaps[cell.parent->type].vScore(rb); --rb);
                        posR[cell.i] = rb;
                    }
                } else if (cell.y != localSegments[r].y) {
                    //multi-row cell not on this row, ignore it
                    break;
                } else {
                    //multi-row cell, check if any cell beofre it is not placed
                    bool blocked = false;
                    for (unsigned s = 1; s < cell.h; ++s) {
                        if (localSegments[r + s].localCells[fronts[r + s]] != cell.i) {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked) {
                        break;
                    }
                    if (cell.fixed) {
                        posR[cell.i] = cell.x;
                    } else {
                        int rb = RBounds[r] - cell.w;
                        for (unsigned s = 1; s < cell.h; ++s) {
                            rb = min(rb, RBounds[r + s] - cell.w);
                        }
                        for (; !_isMax && rb > lx && cellTypeMaps[cell.parent->type].vScore(rb); --rb);
                        posR[cell.i] = rb;
                    }
                }
                for (unsigned s = 0; s != cell.h; ++s) {
                    RBounds[r + s] = posR[cell.i];
                    if (fronts[r + s]) {
                        Cell* parent = localCells[localSegments[r + s].localCells[fronts[r + s] - 1]].parent;
                        if (parent) {
                            int edgetypeR = parent->getDBCellType()->edgetypeR;
                            if (cell.fixed) {
                                RBounds[r + s] = posR[cell.i] - cell.edgespaceL[edgetypeR];
                            } else {
                                RBounds[r + s] = posR[cell.i] - Cell::getSpace(parent, cell.parent);
                            }
                        }
                    }
                    --fronts[r + s];
                }
            }
        }
    }
#else
    vector<int> RBounds(nSegments, hx);
    for (int i = nCells - 1; i >= 0; i--) {
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
        LocalCell& cell = localCells[localCellList[i]];
#endif
#ifdef NEW_PLACEL_PLACER_USE_SORTED_CELLS
        LocalCell& cell = localCells[i];
#endif
        if (cell.fixed) {
            posR[cell.i] = cell.x;
        } else {
            int R = RBounds[cell.y - ly];
            for (int s = 1; s < cell.h; s++) {
                R = min(R, RBounds[cell.y - ly + s]);
            }
            posR[cell.i] = R - cell.w;
        }
        for (int s = 0; s < cell.h; s++) {
            RBounds[cell.y - ly + s] = posR[cell.i];
        }
    }

#endif
}

void LocalRegion::placeSpreadL(int targetCell, vector<int>& posL, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
    bool finished = false;
    //spread to left
    while (!finished) {
        finished = true;
        for (unsigned r = 0; r < nSegments; r++) {
            LocalSegment& segment = localSegments[r];
            int nCells = segment.nCells;
            for (int i = nCells - 2; i >= 0; i--) {
                if (segment.localCells[i] == targetCell) {
                    //newly inserted cell is fixed
                    continue;
                }
                LocalCell* cellL = &localCells[segment.localCells[i]];
                LocalCell* cellR = &localCells[segment.localCells[i + 1]];
                if (cellL->fixed || cellR->fixed) {
                    continue;
                }
                int lW = cellL->w + Cell::getSpace(cellL->parent, cellR->parent);
                if (posL[cellL->i] + lW > posL[cellR->i]) {
                    finished = false;
                    int rb = posL[cellR->i] - lW;
                    for (; !_isMax && rb > lx && cellTypeMaps[cellL->parent->type].vScore(rb); --rb);
                    posL[cellL->i] = rb;
                    assert(posL[cellL->i] >= lx);
                }
            }
        }
    }
}
void LocalRegion::placeSpreadR(int targetCell, vector<int>& posR, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
    bool finished = false;
    //spread to right
    while (!finished) {
        finished = true;
        for (unsigned r = 0; r < nSegments; r++) {
            LocalSegment& segment = localSegments[r];
            int nCells = segment.nCells;
            for (int i = 1; i < nCells; i++) {
                if (segment.localCells[i] == targetCell) {
                    //newly inserted cell is fixed
                    continue;
                }
                LocalCell* cellR = &localCells[segment.localCells[i]];
                LocalCell* cellL = &localCells[segment.localCells[i - 1]];
                if (cellR->fixed || cellL->fixed) {
                    continue;
                }
                int lW = cellL->w + Cell::getSpace(cellL->parent, cellR->parent);
                if (posR[cellR->i] < posR[cellL->i] + lW) {
                    finished = false;
                    int lb = posR[cellL->i] + lW;
                    for (; !_isMax && lb < hx && cellTypeMaps[cellR->parent->type].vScore(lb); ++lb);
                    posR[cellR->i] = lb;
                    assert(posR[cellR->i] + cellR->w <= hx);
                }
            }
        }
    }
}

void LocalRegion::estimate(
    vector<int>& posL,
    vector<int>& posR,
    const vector<vector<LegalMoveInterval> >& intervals,
    const vector<int>& insertionPoint,
    int targetX, Cell* targetCell, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
    estimateL(posL, intervals, insertionPoint, targetX, targetCell, cellTypeMaps);
    estimateR(posR, intervals, insertionPoint, targetX, targetCell, cellTypeMaps);
}

void LocalRegion::estimateL(
    vector<int>& posL,
    const vector<vector<LegalMoveInterval> >& intervals,
    const vector<int>& insertionPoint,
    int targetX, Cell* targetCell, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
    // update cells direcly left of target cell
    const db::CellType* targetCellCType = targetCell->getDBCellType();
    for (unsigned r = 0; r != nSegments; ++r) {
        if (insertionPoint[r] < 0) {
            continue;
        }
        const LocalCell* lCell = intervals[r][insertionPoint[r]].L.cell;
        if (lCell) {
            int lW = lCell->w;
            if (lCell->fixed) {
                lW += lCell->edgespaceR[targetCellCType->edgetypeL];
            } else {
                lW += Cell::getSpace(lCell->parent, targetCell);
            }
            if (posL[lCell->i] < 0 && lCell->x > targetX - lW) {
                int rb = targetX - lW;
                for (; !_isMax && rb > lx && cellTypeMaps[lCell->parent->type].vScore(rb); --rb);
                posL[lCell->i] = rb;
            }
        }
    }

    // propagate to other cells
    bool finished = false;
    while (!finished) {
        finished = true;
        for (const auto& segment : localSegments) {
            for (int i = segment.nCells - 2; i >= 0; i--) {
                if (posL[segment.localCells[i + 1]] < 0) {
                    //right cell is not moved
                    continue;
                }
                LocalCell* cellL = &localCells[segment.localCells[i]];
                LocalCell* cellR = &localCells[segment.localCells[i + 1]];
                if (cellL->fixed || cellR->fixed) {
                    continue;
                }
                int lW = cellL->w + Cell::getSpace(cellL->parent, cellR->parent);
                if (posL[cellL->i] < 0) {
                    if (cellL->x > posL[cellR->i] - lW) {
                        finished = false;
                        int rb = posL[cellR->i] - lW;
                        for (; !_isMax && rb > lx && cellTypeMaps[cellL->parent->type].vScore(rb); --rb);
                        posL[cellL->i] = rb;
                    }
                } else {
                    if (posL[cellL->i] > posL[cellR->i] - lW) {
                        finished = false;
                        int rb = posL[cellR->i] - lW;
                        for (; !_isMax && rb > lx && cellTypeMaps[cellL->parent->type].vScore(rb); --rb);
                        posL[cellL->i] = rb;
                    }
                }
                assert(posL[cellL->i] == -1 || posL[cellL->i] >= lx);
            }
        }
    }
}

void LocalRegion::estimateR(
    vector<int>& posR,
    const vector<vector<LegalMoveInterval> >& intervals,
    const vector<int>& insertionPoint,
    int targetX, Cell* targetCell, const vector<CellTypeMap<MLLCost>>& cellTypeMaps) {
    // update cells direcly right of target cell
    const db::CellType* targetCellCType = targetCell->getDBCellType();
    for (unsigned r = 0; r != nSegments; ++r) {
        if (insertionPoint[r] < 0) {
            continue;
        }
        const LocalCell* rCell = intervals[r][insertionPoint[r]].R.cell;
        if (rCell != NULL) {
            int rW = targetCell->w;
            if (rCell->fixed) {
                rW += rCell->edgespaceL[targetCellCType->edgetypeR];
            } else {
                rW += Cell::getSpace(targetCell, rCell->parent);
            }
            if (posR[rCell->i] < 0 && rCell->x < targetX + rW) {
                int lb = targetX + rW;
                for (; !_isMax && lb < hx && cellTypeMaps[rCell->parent->type].vScore(lb); ++lb);
                posR[rCell->i] = lb;
            }
        }
    }

    // propagate to other cells
    bool finished = false;
    while (!finished) {
        finished = true;
        for (const auto& segment : localSegments) {
            for (unsigned i = 1; i < segment.nCells; i++) {
                if (posR[segment.localCells[i - 1]] < 0) {
                    //left cell is not moved
                    continue;
                }
                LocalCell* cellR = &localCells[segment.localCells[i]];
                LocalCell* cellL = &localCells[segment.localCells[i - 1]];
                if (cellR->fixed || cellL->fixed) {
                    continue;
                }
                int lW = cellL->w + Cell::getSpace(cellL->parent, cellR->parent);
                if (posR[cellR->i] < 0) {
                    if (cellR->x < posR[cellL->i] + lW) {
                        finished = false;
                        int lb = posR[cellL->i] + lW;
                        for (; !_isMax && lb < hx && cellTypeMaps[cellR->parent->type].vScore(lb); ++lb);
                        posR[cellR->i] = lb;
                    }
                } else {
                    if (posR[cellR->i] < posR[cellL->i] + lW) {
                        finished = false;
                        int lb = posR[cellL->i] + lW;
                        for (; !_isMax && lb < hx && cellTypeMaps[cellR->parent->type].vScore(lb); ++lb);
                        posR[cellR->i] = lb;
                    }
                }
                assert(posR[cellR->i] == -1 || posR[cellR->i] + cellR->w <= hx);
            }
        }
    }
}

bool LocalRegion::isValid(int x, int y, int w, int h) {
    if (x < lx || x + w > hx || y < ly || y + h > hy) {
        return false;
    }
    bool valid = true;
    for (unsigned s = 0; s < nSegments; s++) {
        if (y == localSegments[s].y) {
            for (int i = 0; i < h; i++) {
                if (x < localSegments[s + i].x || x + w > localSegments[s + i].x + localSegments[s + i].w) {
                    valid = false;
                    break;
                }
            }
            break;
        }
    }
    if (!valid) {
        printlog(LOG_ERROR, "region: (%d,%d):(%d,%d)",
            lx, ly, hx, hy);
        printlog(LOG_ERROR, "local cell: %d", nCells);
        for (unsigned s = 0; s < nSegments; s++) {
            printlog(LOG_ERROR, "%d: (%d,%d)",
                localSegments[s].y,
                localSegments[s].x,
                localSegments[s].x + localSegments[s].w);
        }
    }
    return valid;
}

bool LocalRegion::verify(std::string title)
{
    for (int i = 0; i < (int)nCells; i++) {
        if (localCells[i].parent && localCells[i].y != localCells[i].parent->ly()) {
            printlog(LOG_ERROR, "%s : violate", title.c_str());
            getchar();
            return false;
        }
    }
    return true;
}

bool LocalRegion::checkOverlap()
{
    for (unsigned i = 0; i < nSegments; i++) {
        LocalSegment& seg = localSegments[i];
        if (seg.nCells == 0) {
            continue;
        }
        int L = seg.x;
        int R = seg.x + seg.w;
        for (unsigned c = 0; c < seg.nCells; c++) {
            LocalCell& cell = localCells[seg.localCells[c]];
            if (cell.x < L) {
                return true;
            }
            L = cell.x + cell.w;
            if (L > R) {
                return true;
            }
        }
    }
    return false;
}

void LocalRegion::report()
{
    printlog(LOG_INFO, "-----------------------------");
    printlog(LOG_INFO, "LocalRegion");
    printlog(LOG_INFO, "-----------------------------");
    printlog(LOG_INFO, "(%d,%d),(%d,%d)", lx, ly, hx, hy);
    printlog(LOG_INFO, "-Fixed Cells-----------------");
    for (unsigned i = 0; i < nCells; i++) {
        LocalCell& cell = localCells[i];
        if (!cell.fixed) {
            continue;
        }
        printlog(LOG_INFO, "%d : (%d,%d) (%d,%d) w=%d h=%d", cell.i, cell.x, cell.y, cell.x + cell.w, cell.y + cell.h, cell.w, cell.h);
    }
    printlog(LOG_INFO, "-Movable Cells---------------");
    for (unsigned i = 0; i < nCells; i++) {
        LocalCell& cell = localCells[i];
        if (cell.fixed) {
            continue;
        }
        printlog(LOG_INFO, "%d : (%d,%d) (%d,%d) w=%d h=%d", cell.i, cell.x, cell.y, cell.x + cell.w, cell.y + cell.h, cell.w, cell.h);
    }
    printlog(LOG_INFO, "-----------------------------");
}
