#include "dp_data.h"
#include "dp_global.h"
using namespace dp;

#define DEBUG_CELL_ID -1

bool DPlacer::isLegalMoveSR(Move& move, Region& region, double targetWeight)
{
    Cell* targetCell = move.target.cell;
    int targetW = targetCell->w;
    const long targetXLong = max<long>(region.lx, min<long>(region.hx - targetW, move.target.xLong()));

    int bestX = -1;
    int bestY = -1;
    long bestDisp = LONG_MAX;
    Segment* bestSeg = nullptr;

    for (int y = region.ly; y < region.hy; y++) {
        Segment* seg = getNearestSegmentInRow(targetCell, targetXLong, y);
        if (!seg) {
            if (targetCell->id == DEBUG_CELL_ID) {
                cout << "null segment" << endl;
            }
            continue;
        }
        if (seg->fid != targetCell->fid) {
            if (targetCell->id == DEBUG_CELL_ID) {
                cout << "fence mismatch" << endl;
            }
            continue;
        }

        Cell* cellL = nullptr;
        Cell* cellR = nullptr;
        CellListIter iterL;
        CellListIter iterR;

        shared_lock<shared_mutex> lock(seg->mtx);
        seg->getCellAt(targetCell->cx(), y, cellL, cellR, iterL, iterR);

        Cell* cL = cellL;
        Cell* cR = cellR;
        CellListIter iL = iterL;
        CellListIter iR = iterR;

        int boundL = cL ? (cL->hx() + Cell::getSpace(cL, targetCell)) : seg->boundL();
        int boundR = cR ? (cR->lx() - Cell::getSpace(targetCell, cR)) : seg->boundR();
        int cellWidthL = 0;
        int cellWidthR = 0;
        int requireSpace = targetW;
        int provideSpace = boundR - boundL;

        if (targetCell->id == DEBUG_CELL_ID) {
            cout << " L=" << (cellL ? cellL->id : -1);
            cout << " R=" << (cellR ? cellR->id : -1);
            cout << " bound=" << boundL << "," << boundR;
            cout << " need=" << requireSpace;
            cout << " have=" << provideSpace;
        }
        /*
        if(targetCell->id == DEBUG_CELL_ID){
            printlog(LOG_INFO, "* L=%d , R=%d",
                    cL==NULL?-1:cellL->id,
                    cR==NULL?-1:cellR->id);
            printlog(LOG_INFO, "* bound = (%d,%d)",
                    boundL, boundR);
            printlog(LOG_INFO, "require=%d provide=%d",
                    requireSpace, provideSpace);
        }
        */
        Cell* oCL = targetCell;
        Cell* oCR = targetCell;
        while (provideSpace < requireSpace && (cL || cR)) {
            /*
            if(targetCell->id == DEBUG_CELL_ID){
                printlog(LOG_INFO, "L=%d , R=%d",
                        cL==NULL?-1:cellL->id,
                        cR==NULL?-1:cellR->id);
                printlog(LOG_INFO, "bound = (%d,%d)",
                        boundL, boundR);
                printlog(LOG_INFO, "require=%d provide=%d",
                        requireSpace, provideSpace);
            }
            */
            if (cL) {
                cellWidthL += (cL->w + Cell::getSpace(cL, oCL));
                oCL = cL;
                if (iL == seg->cells.begin()) {
                    cL = nullptr;
                    boundL = seg->boundL();
                } else {
                    --iL;
                    cL = seg->cells.getCell(iL);
                    boundL = cL->hx() + Cell::getSpace(cL, oCL);
                }
            }

            if (cR) {
                cellWidthR += (cR->w + Cell::getSpace(oCR, cR));
                oCR = cR;
                ++iR;
                if (iR == seg->cells.end()) {
                    cR = nullptr;
                    boundR = seg->boundR();
                } else {
                    cR = seg->cells.getCell(iR);
                    boundR = cR->lx() - Cell::getSpace(oCR, cR);
                }
            }

            requireSpace = cellWidthL + targetW + cellWidthR;
            provideSpace = boundR - boundL;
        }

        if (targetCell->id == DEBUG_CELL_ID) {
            cout << " segment=" << seg->boundL() << ":" << seg->boundR();
            cout << " need=" << requireSpace;
            cout << " have=" << provideSpace;
            if (requireSpace > provideSpace) {
                cout << " fail" << endl;
            }
        }
        if (requireSpace > provideSpace) {
            //fail
            continue;
        }

        long cellX = targetXLong;
        int cellY = y;
        if (targetXLong < boundL + cellWidthL) {
            cellX = boundL + cellWidthL;
        } else if (targetXLong + targetW > boundR - cellWidthR) {
            cellX = boundR - cellWidthR - targetW;
        } else {
            cellX = targetXLong;
        }

        double dispX = targetWeight * abs(move.target.x() - cellX);
        double dispY = targetWeight * abs(move.target.y() - cellY);

        vector<CellMove> legals;
        cL = cellL;
        cR = cellR;
        iL = iterL;
        iR = iterR;
        oCL = targetCell;
        oCR = targetCell;
        boundL = cellX;
        boundR = cellX + targetW;
        while (cL) {
            boundL -= (cL->w + Cell::getSpace(cL, oCL));
            if (cL->lx() <= boundL) {
                break;
            }
            legals.push_back(CellMove(cL, boundL, cL->ly()));
            dispX += (cL->lx() - boundL);
            oCL = cL;
            if (iL == seg->cells.begin()) {
                cL = nullptr;
            } else {
                --iL;
                cL = seg->cells.getCell(iL);
            }
        }
        while (cR) {
            boundR += Cell::getSpace(oCR, cR);
            if (cR->lx() >= boundR) {
                break;
            }
            legals.push_back(CellMove(cR, boundR, cR->ly()));
            dispX += (boundR - cR->lx());
            oCR = cR;
            boundR += cR->w;
            ++iR;
            if (iR == seg->cells.end()) {
                cR = nullptr;
            } else {
                cR = seg->cells.getCell(iR);
            }
        }

        const long totalDisp = lround(dispX * siteW + dispY * siteH);
        if (totalDisp < bestDisp) {
            bestDisp = totalDisp;
            bestX = cellX;
            bestY = cellY;
            bestSeg = seg;
            move.legals = legals;
        }
    }

    if (bestDisp == LONG_MAX) {
        return false;
    }

    move.target.x(bestX);
    move.target.y(bestY);
    if (move.target.xLong() < bestSeg->boundL() || move.target.xLong() + static_cast<long>(targetCell->w) > bestSeg->boundR() || move.target.yLong() != bestSeg->y) {
        printlog(LOG_ERROR, "out of range (%d)", targetCell->id);
        getchar();
    }
    return true;
}

