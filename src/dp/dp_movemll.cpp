#include "dp_data.h"
#include "dp_global.h"

using namespace dp;

int DPlacer::getOptimalX(vector<CriticalPoint>& critiPtsInput,
                         const unsigned targetCellType,
                         const int rangeBoundL,
                         const int rangeBoundR) const {
    vector<CriticalPoint*> critiPts(critiPtsInput.size());
    for (unsigned i = 0; i != critiPts.size(); ++i) {
        critiPts[i] = &critiPtsInput[i];
    }
    sort(critiPts.begin(), critiPts.end(), [](const CriticalPoint* p1, const CriticalPoint* p2) {
        return p1->x < p2->x;
    });
    //  merge AND remove cancelled pts
    unsigned filled = 0;
    for (unsigned i = 1; i != critiPts.size(); ++i) {
        if (critiPts[i]->x == critiPts[filled]->x) {
            critiPts[filled]->lSlope += critiPts[i]->lSlope;
            critiPts[filled]->rSlope += critiPts[i]->rSlope;
        } else {
            //  remove cancelled pts
            if (critiPts[filled]->lSlope || critiPts[filled]->rSlope) {
                ++filled;
            }
            critiPts[filled] = critiPts[i];
        }
    }
    critiPts.resize(filled + 1);
    vector<int> slopes(critiPts.size() + 1, 0);
    //  forward propagation
    for (unsigned i = 0; i != critiPts.size(); ++i) {
        slopes[i + 1] = slopes[i] + critiPts[i]->rSlope;
    }
    //  backward propagation
    int acc = 0;
    for (unsigned i = critiPts.size(); i; --i) {
        acc += critiPts[i - 1]->lSlope;
        slopes[i - 1] += acc;
    }
    // get values
    vector<int> values(critiPts.size());
    values[0] = 0;
    unsigned iMin = 0;
    // int bx=critiPts[0]->x, ex=critiPts[0]->x; // begin/end x
    int minV = 0;
    for (unsigned i = 1; i != critiPts.size(); ++i) {
        values[i] = values[i - 1] + slopes[i] * (critiPts[i]->x - critiPts[i - 1]->x);
        if (values[i] < minV) {
            minV = values[i];
            iMin = i;
            // bx = ex = critiPts[i]->x;
        }
        // else if (values[i]==minV){
        //     ex = critiPts[i]->x;
        // }
    }
    // return (bx+ex) / 2;
    unsigned iL = iMin;
    unsigned iR = iMin;
    int placeXL = critiPts[iL]->x;
    int placeXR = critiPts[iR]->x;
    if (iMin < critiPts.size() - 1 && values[iMin + 1] == values[iMin]) {
        iL = iMin + 1;
        placeXL = (critiPts[iMin]->x + critiPts[iMin + 1]->x) / 2;
        placeXR = (critiPts[iMin]->x + critiPts[iMin + 1]->x) / 2 + 1;
    }
    int placeXC = placeXL;

    if (placeXR < rangeBoundL) {
        for (placeXR = rangeBoundL; placeXR <= rangeBoundR; ++placeXR) {
            if (!cellTypeMaps[targetCellType].vScore(placeXR)) {
                return placeXR;
            }
        }
    } else if (placeXL > rangeBoundR) {
        for (placeXL = rangeBoundR; placeXL >= rangeBoundL; --placeXL) {
            if (!cellTypeMaps[targetCellType].vScore(placeXL)) {
                return placeXL;
            }
        }
    } else {
        while (placeXL >= rangeBoundL || placeXR <= rangeBoundR) {
            int vL = values[iL] - slopes[iL] * (critiPts[iL]->x - placeXL);
            int vR = values[iR] + slopes[iR + 1] * (placeXR - critiPts[iR]->x);
            if (vL <= vR) {
                if (placeXL >= 0 && placeXL <= coreHX && !cellTypeMaps[targetCellType].vScore(placeXL)) {
                    return placeXL;
                }
                --placeXL;
                if (iL > 0 && placeXL <= critiPts[iL - 1]->x) {
                    --iL;
                }
            } else {
                if (placeXR >= 0 && placeXR <= coreHX && !cellTypeMaps[targetCellType].vScore(placeXR)) {
                    return placeXR;
                }
                ++placeXR;
                if (iR < critiPts.size() - 1 && placeXR >= critiPts[iR + 1]->x) {
                    ++iR;
                }
            }
        }
    }
    return placeXC;
}

bool DPlacer::isLegalMoveMLL(Move& m, Region& region, double weight, unsigned iThread) {
    LocalRegion& localRegion = localRegions[iThread];
    // bounding box surrounding the cell to be moved
    const int lx = region.lx;
    const int ly = region.ly;
    const int hx = region.hx;
    const int hy = region.hy;

    Cell* targetCell = m.target.cell;
    const float targetXFloat = m.target.x();
    const float targetYFloat = m.target.y();
    const long targetXLong = m.target.xLong();
    double density = defineLocalRegion(localRegion, targetCell, region);
    if (density >
        max(DPModule::MLLMaxDensity, (unsigned)ceil(database.regions[targetCell->fid]->density * 1000)) / 1000.0) {
        //  printlog(LOG_WARN, "skip density %f", density);
        return false;
    }
    //  if(localRegion.nCells) { localMinCostFlow(localRegion); }

    localRegion.useCount++;

    unsigned nRows = hy - ly;
    int targetW = targetCell->w;
    int targetCellH = targetCell->h;
    char targetRow = targetCell->rowPlace();
    unsigned targetCellType = targetCell->type;
    db::CellType* targetCellCType = targetCell->getDBCellType();

    vector<vector<LegalMoveInterval>> intervals(nRows);

    for (unsigned r = 0; r != nRows; ++r) {
        //#cell + 1 interval for each row
        unsigned nIntervals = localRegion.localSegments[r].nCells + 1;
        intervals[r].resize(nIntervals, LegalMoveInterval(r));
    }

    vector<int> pos(localRegion.nCells + 1, -1);
    for (unsigned i = 0; i != localRegion.nCells; ++i) {
        pos[i] = localRegion.localCells[i].x;
    }
    // do left/right-most placement and set up intervals

    // left-most placement
    localRegion.placeL(pos, cellTypeMaps);

    for (unsigned r = 0; r != nRows; ++r) {
        const LocalSegment& segment = localRegion.localSegments[r];
        intervals[r][0].L.pos = segment.x;
        intervals[r][0].L.cell = nullptr;
        for (unsigned i = 0; i < segment.nCells; ++i) {
            const LocalCell& cell = localRegion.localCells[segment.localCells[i]];
            int lb = pos[cell.i] + cell.w;
            if (cell.fixed) {
                lb += cell.edgespaceR[targetCellCType->edgetypeL];
            } else {
                lb += Cell::getSpace(cell.parent, targetCell);
            }
            for (; !region.isMax() && lb < hx && cellTypeMaps[targetCellType].vScore(lb); ++lb);
            intervals[r][i + 1].L.pos = lb;
            intervals[r][i + 1].L.cell = &cell;
        }
    }
    // right-most placement
    localRegion.placeR(pos, cellTypeMaps);

    for (unsigned r = 0; r != nRows; ++r) {
        const LocalSegment& segment = localRegion.localSegments[r];
        for (unsigned i = 0; i != segment.nCells; ++i) {
            const LocalCell& cell = localRegion.localCells[segment.localCells[i]];
            int rb = pos[cell.i] - targetW;
            if (cell.fixed) {
                rb -= cell.edgespaceL[targetCellCType->edgetypeR];
            } else {
                rb -= Cell::getSpace(targetCell, cell.parent);
            }
            for (; !region.isMax() && rb > lx && cellTypeMaps[targetCellType].vScore(rb); --rb);
            intervals[r][i].R.pos = rb;
            intervals[r][i].R.cell = &cell;
        }
        intervals[r][segment.nCells].R.pos = segment.x + segment.w - targetW;
        intervals[r][segment.nCells].R.cell = NULL;
    }

    localRegion.reset();

    for (unsigned r = 0; r != nRows; ++r) {
        // remove all invalid intervals
        vector<LegalMoveInterval>::iterator ib = intervals[r].begin();
        vector<LegalMoveInterval>::iterator ie = intervals[r].end();
        vector<LegalMoveInterval>::iterator end = remove_if(ib, ie, LegalMoveInterval::isInvalid);
        if (end != ie) {
            intervals[r].resize(distance(ib, end));
        }

        LocalSegment& segment = localRegion.localSegments[r];
        unsigned nIntervals = intervals[r].size();
        for (unsigned i = 0; i != nIntervals; ++i) {
            // critical positions
            intervals[r][i].i = i;
            const LocalCell* lCell = intervals[r][i].L.cell;
            const LocalCell* rCell = intervals[r][i].R.cell;
            if (lCell) {
                if (lCell->fixed) {
                    intervals[r][i].optPointL(lCell->ox() + lCell->w + lCell->edgespaceR[targetCellCType->edgetypeL]);
                } else {
                    intervals[r][i].optPointL(lCell->ox() + lCell->w + Cell::getSpace(lCell->parent, targetCell));
                }
            } else {
                intervals[r][i].optPointL(segment.x);
            }
            if (rCell) {
                if (rCell->fixed) {
                    intervals[r][i].optPointR(rCell->ox() - targetW - rCell->edgespaceL[targetCellCType->edgetypeR]);
                } else {
                    intervals[r][i].optPointR(rCell->ox() - targetW - Cell::getSpace(targetCell, rCell->parent));
                }
            } else {
                intervals[r][i].optPointR(segment.x + segment.w - targetW);
            }
            // associate endpoint and intervals
            intervals[r][i].L.interval = &intervals[r][i];
            intervals[r][i].R.interval = &intervals[r][i];
        }
    }

    // sort the interval endpoints
    vector<LegalMoveIntervalEndpoint*> intervalSort;

    for (unsigned r = 0; r != nRows; ++r) {
        for (unsigned i = 0; i != intervals[r].size(); ++i) {
            intervalSort.push_back(&intervals[r][i].L);
            intervalSort.push_back(&intervals[r][i].R);
        }
    }

    sort(intervalSort.begin(),
         intervalSort.end(),
         [](const LegalMoveIntervalEndpoint* a, const LegalMoveIntervalEndpoint* b) {
             if (a->pos != b->pos) {
                 return a->pos < b->pos;
             }
             if (a->side != b->side) {
                 return a->side == 'L';
             }
             if (a->cell != b->cell) {
                 return a->cell < b->cell;
             }
             return a->interval->r < b->interval->r;
         });

    // insertion point enumeration

    vector<int> inQueueBgn(nRows, 0);
    vector<int> inQueueEnd(nRows, 0);
    vector<vector<int>> validBegin(nRows, vector<int>(nRows, 0));

    vector<int> posL(localRegion.nCells + 1, -1);
    vector<int> posR(localRegion.nCells + 1, -1);
    int bestX = INT_MAX;
    int bestY = INT_MAX;
    MLLCost bestCost = MLLCostInf;
//  unsigned bestMaxDisp = INT_MAX;
//  unsigned bestCostMaxDisp = INT_MAX;
#ifdef MLL_VERIFY_ESTIMATION
    MLLCost bestTargetCostX = MLLCostInf;
    MLLCost bestTargetCostY = MLLCostInf;
    MLLCost bestLocalCostX = MLLCostInf;
    MLLCost bestLocalCostY = MLLCostInf;
    vector<int> bestPosL = posL;
    vector<int> bestPosR = posR;
#endif
    vector<int> bestIntervals(nRows);

    //  scanning the endpoints
    for (LegalMoveIntervalEndpoint* endpoint : intervalSort) {
        unsigned row = endpoint->interval->r;

        // right endpoint: remove an interval
        if (endpoint->side == 'R') {
            for (unsigned r = 0; r != nRows; ++r) {
                if (validBegin[r][row] == inQueueBgn[row]) {
                    ++validBegin[r][row];
                }
            }
            ++inQueueBgn[row];
            continue;
        }

        // if added interval cell is multi-row, update validBegin
        const LocalCell* cell = endpoint->interval->L.cell;
        if (cell && cell->h > 1) {
            // multi-row local cell, clear incompatible intervals
            for (unsigned r = cell->y - ly; r != unsigned(cell->y - ly + cell->h); ++r) {
                if (r == row) {
                    continue;
                }
                // clear all intervals
                if (inQueueEnd[r] > 0 && intervals[r][inQueueEnd[r] - 1].L.cell == cell) {
                    validBegin[row][r] = inQueueEnd[r] - 1;
                    continue;
                }
                validBegin[row][r] = inQueueEnd[r];
            }
        }

        // left endpoint: insert an interval
        ++inQueueEnd[row];

#ifdef MLL_FIND_REAL_CRITICAL_POINTS
        // vector<int>optBounds(localRegion.nCells * 2 + 2, -1);
        vector<int> optBounds(localRegion.nCells * 4 + 2, -1);
#else
        vector<int> optBounds(targetCellH * 4 + 2, -1);
#endif

        // find all combination using "row"
        unsigned btmRow = max(0, (int)row - targetCellH + 1);
        unsigned topRow = btmRow + targetCellH - 1;
        for (; btmRow <= row && topRow < nRows; ++btmRow, ++topRow) {
            vector<int> indexes(nRows, -1);

            // power-ground matching
            int placeY = localRegion.ly + btmRow;
            if (targetRow != 'a' && ((targetRow == 'e' && placeY % 2 == 1) || (targetRow == 'o' && placeY % 2 == 0))) {
                continue;
            }

            //  avoid horizontal rails overlapping
            if (cellTypeMaps[targetCellType].hScore(placeY)) {
                //  printlog(LOG_INFO, "skip row %d", placeY);
                continue;
            }

            // calculate total number of insertion points
            // initialize the indexes
            unsigned nTotal = 1;
            for (unsigned r = btmRow; r <= topRow && nTotal; ++r) {
                if (r == row) {
                    indexes[r] = inQueueEnd[r] - 1;
                } else {
                    indexes[r] = validBegin[row][r];
                    nTotal *= (inQueueEnd[r] - validBegin[row][r]);
                }
            }

            if (!nTotal) {
                continue;
            }

            //  int rangeBoundL = INT_MIN;
            //  int rangeBoundR = INT_MAX;
            //  int rowDispBound = (int)DPModule::MaxDisp - abs(placeY - targetY) * siteH / siteW;
            for (unsigned index = 0; index != nTotal; ++index, ++indexes[btmRow]) {
                // for each insertion point

                // generate permutation
                for (unsigned r = btmRow; r != topRow; ++r) {
                    if (indexes[r] >= inQueueEnd[r]) {
                        if (r == row) {
                            indexes[r] = inQueueEnd[r] - 1;
                        } else {
                            indexes[r] = validBegin[row][r];
                        }
                        ++indexes[r + 1];
                    } else {
                        break;
                    }
                }

                //  find range bound
                long rangeBoundL = lx;
                long rangeBoundR = hx;
                for (unsigned r = btmRow; r <= topRow; ++r) {
                    rangeBoundL = max<long>(intervals[r][indexes[r]].L.pos, rangeBoundL);
                    rangeBoundR = min<long>(intervals[r][indexes[r]].R.pos, rangeBoundR);
                }
                if (rangeBoundL > rangeBoundR) {
                    continue;
                }
                if (localRegion.useCount == DEBUG_ID) {
                    printlog(LOG_INFO, "-----------------");
                }
                // without disturbing left/right cells
                long woDistL = LONG_MIN;
                long woDistR = LONG_MAX;
                for (unsigned r = btmRow; r <= topRow; ++r) {
                    woDistL = max<int>(intervals[r][indexes[r]].optPointL() + 0.5, woDistL);
                    woDistR = min<int>(intervals[r][indexes[r]].optPointR() + 0.5, woDistR);
                }
                woDistL = max(woDistL, targetXLong);
                woDistR = min(woDistR, targetXLong);
                woDistL = min(woDistL, rangeBoundR);
                woDistR = max(woDistR, rangeBoundL);

                long placeX = targetXLong;

                if (woDistL != targetXLong || targetXLong != woDistR || cellTypeMaps[targetCellType].vScore(targetXLong)) {
// find all critical points
#ifdef MLL_FIND_REAL_CRITICAL_POINTS

                    vector<CriticalPoint> critiPts;

                    // push to left
                    posL.assign(localRegion.nCells, -1);
                    localRegion.estimateL(posL, intervals, indexes, woDistR, targetCell, cellTypeMaps);
                    for (unsigned i = 0; i != localRegion.nCells; ++i) {
                        if (posL[i] >= 0) {
                            const int shift = woDistR - posL[i];
                            const int curX = localRegion.localCells[i].x + shift;
                            const int gpX = localRegion.localCells[i].parent->oxLong() + shift;
                            if (curX <= gpX) {
                                if (curX >= rangeBoundL) {
                                    critiPts.emplace_back(curX, -1, 0);
                                }
                            } else {
                                // -1, 1, 0
                                if (gpX >= rangeBoundL) {
                                    critiPts.emplace_back(gpX, -2, 0);
                                }
                                if (curX >= rangeBoundL) {
                                    critiPts.emplace_back(curX, 1, 0);
                                }
                            }
                        }
                    }

#ifdef MLL_FIND_DISPLACEMENT_CURVE
                    DisplacementCurve curve;

                    for (unsigned i = 0; i < localRegion.nCells; i++) {
                        if (posL[i] >= 0) {
                            LocalCell& cell = localRegion.localCells[i];
                            assert(cell.i == (int)i);
                            assert(posL[i] < cell.x);
                            int c = rangeBoundL + cell.x - posL[i];  // x_t^L + disp_i^L
                            if (cell.ox >= cell.x) {
                                curve.addCurve(i, c, c, 'L');  // type 1
                            } else if (cell.ox > posL[i]) {
                                curve.addCurve(i, c, (c - cell.ox + cell.x), 'L');  // type 2
                            } else {
                                curve.addCurve(i, c, posL[i], 'L');  // type 3
                            }
                        }
                    }
#endif

                    // push to right
                    posR.assign(localRegion.nCells, -1);
                    localRegion.estimateR(posR, intervals, indexes, woDistL, targetCell, cellTypeMaps);
                    for (unsigned i = 0; i < localRegion.nCells; i++) {
                        if (posR[i] >= 0) {
                            int shift = woDistL - posR[i];
                            int curX = localRegion.localCells[i].x + shift;
                            int gpX = localRegion.localCells[i].parent->oxLong() + shift;
                            if (curX >= gpX) {
                                if (curX <= rangeBoundR) critiPts.emplace_back(curX, 0, 1);
                            } else {
                                // 0, -1, 1
                                if (curX <= rangeBoundR) critiPts.emplace_back(curX, 0, -1);
                                if (gpX <= rangeBoundR) critiPts.emplace_back(gpX, 0, 2);
                            }
                        }
                    }

#ifdef MLL_FIND_DISPLACEMENT_CURVE
                    for (unsigned i = 0; i < localRegion.nCells; i++) {
                        if (posR[i] >= 0) {
                            LocalCell& cell = localRegion.localCells[i];
                            assert(posR[i] > cell.x);
                            int c = rangeBoundR - posR[i] + cell.x;  // x_t^R - disp_i^L
                            if (cell.ox <= cell.x) {
                                curve.addCurve(i, c, c, 'R');  // type 1
                            } else if (cell.ox < posR[i]) {
                                curve.addCurve(i, c, c + cell.ox - cell.x, 'R');  // type 2
                            } else {
                                curve.addCurve(i, c, posR[i], 'R');  // type 3
                            }
                        }
                    }
// curve.print();
#endif

                    critiPts.emplace_back(targetXLong, -1, 1);
                    placeX = getOptimalX(critiPts, targetCellType, rangeBoundL, rangeBoundR);
#else
                    int nBounds = 0;
                    for (unsigned r = btmRow; r <= topRow; ++r) {
                        optBounds[nBounds++] = intervals[r][indexes[r]].optPointL;
                        optBounds[nBounds++] = localRegion.hx;
                        optBounds[nBounds++] = intervals[r][indexes[r]].optPointR;
                        optBounds[nBounds++] = localRegion.lx;
                    }
                    optBounds[nBounds++] = targetXLong;
                    optBounds[nBounds++] = targetXLong;
                    // median of the critical points
                    sort(optBounds.begin(), optBounds.begin() + nBounds);
                    placeX = (optBounds[nBounds / 2] + optBounds[nBounds / 2 - 1]) / 2;
#endif
                }

                if (localRegion.useCount == DEBUG_ID) {
                    printlog(LOG_INFO, "opt range x = %d", placeX);
                    printlog(LOG_INFO, "opt range y = %d", placeY);
                    printlog(LOG_INFO, "-----------------------------");
                }
                placeX = max(rangeBoundL, min(rangeBoundR, placeX));

                // displacement from target position
                MLLCost targetCostX = 0;
                MLLCost targetCostY = 0;
                MLLCost targetCostPin = 0;
#ifdef MAINTAIN_HIST
                vector<unsigned> lHist = oHist;
                unsigned lMaxDisp = oMaxDisp;
#endif
                if (DPModule::MLLTotalDisp) {
                    targetCostX = abs(targetXFloat - placeX);
                    targetCostY = abs(targetYFloat - placeY);
                } else {
                    targetCostX = abs(targetXFloat - placeX) * weight / (double)counts[targetCell->h - 1];
                    targetCostY = abs(targetYFloat - placeY) * weight / (double)counts[targetCell->h - 1];
                    targetCostPin = cellTypeMaps[targetCell->type].score(placeY, placeX);
#ifdef MAINTAIN_HIST
                    updateHist(lHist, lMaxDisp, targetCell, placeX, placeY);
#endif
                }

                MLLCost localCostX = 0;
                MLLCost localCostY = 0;
                MLLCost localCostPin = 0;
#ifdef MLL_CONSIDER_N_NEIGHBOR_CELLS
                posL.assign(localRegion.nCells, -1);
                posR.assign(localRegion.nCells, -1);
                localRegion.estimate(posL, posR, intervals, indexes, placeX, targetCell, cellTypeMaps);
                for (unsigned i = 0; i < localRegion.nCells; ++i) {
                    LocalCell& cell = localRegion.localCells[i];
                    if (posL[i] >= 0) {
                        if (DPModule::MLLTotalDisp) {
                            localCostX += (abs(posL[i] - cell.parent->ox()) - abs(cell.x - cell.parent->ox()));
                        } else {
                            localCostX += (abs(posL[i] - cell.parent->ox()) - abs(cell.x - cell.parent->ox())) /
                                          (double)counts[cell.h - 1];
#ifdef MAINTAIN_HIST
                            updateHist(lHist, lMaxDisp, cell.parent, posL[i], cell.y);
#endif
                            const auto& ctm = cellTypeMaps[cell.parent->type];
                            localCostPin += ctm.score(cell.y, posL[i]) - ctm.score(cell.y, cell.x);
                        }
                    }
                    if (posR[i] >= 0) {
                        if (DPModule::MLLTotalDisp) {
                            localCostX += (abs(posR[i] - cell.parent->ox()) - abs(cell.x - cell.parent->ox()));
                        } else {
                            localCostX += (abs(posR[i] - cell.parent->ox()) - abs(cell.x - cell.parent->ox())) /
                                          (double)counts[cell.h - 1];
#ifdef MAINTAIN_HIST
                            updateHist(lHist, lMaxDisp, cell.parent, posR[i], cell.y);
#endif
                            const auto& ctm = cellTypeMaps[cell.parent->type];
                            localCostPin += ctm.score(cell.y, posR[i]) - ctm.score(cell.y, cell.x);
                        }
                    }
                }
#else
                for (unsigned r = btmRow; r <= topRow; ++r) {
                    LocalCell* lCell = intervals[r][indexes[r]].L.cell;
                    LocalCell* rCell = intervals[r][indexes[r]].R.cell;
                    if (lCell) {
                        localCostX += max(0, lCell->ox - placeX + lCell->w);
                    }
                    if (rCell) {
                        localCostX += max(0, placeX + targetW - rCell->ox);
                    }
                }
#endif
                MLLCost costx = targetCostX + localCostX;
                MLLCost costy = targetCostY + localCostY;
                MLLCost costPin = targetCostPin + localCostPin;
                MLLCost cost = 0;
                if (DPModule::MLLTotalDisp) {
                    cost = costx * siteW + costy * siteH;
                } else {
#ifndef MAINTAIN_HIST
                    cost = costx * siteW + costy * siteH + costPin / (double)counts[1];
#else
                    cost = costx * siteW + costy * siteH + lMaxDisp * siteW * 100 +
                           costPin * DPModule::MLLPinCost * 100 / (double)counts[0];
#endif
                }

                if (cost < bestCost) {
                    bestX = placeX;
                    bestY = placeY;
#ifdef MLL_VERIFY_ESTIMATION
                    bestTargetCostX = targetCostX;
                    bestTargetCostY = targetCostY;
                    bestLocalCostX = localCostX;
                    bestLocalCostY = localCostY;
                    bestPosL = posL;
                    bestPosR = posR;
#endif
                    bestCost = cost;
                    //  bestCostMaxDisp = mDisp;
                    for (unsigned r = 0; r != nRows; ++r) {
                        bestIntervals[r] = indexes[r];
                    }
                }
            }
        }
    }

    if (bestCost == MLLCostInf) {
        return false;
    }

    if (localRegion.useCount == DEBUG_ID) {
        printlog(LOG_INFO, "best x,y = (%d,%d)", bestX, bestY);
    }

    // insert the new cell to the position
    LocalCell newCell(targetCell);
    newCell.ox(bestX);
    newCell.oy(bestY);
    newCell.x = bestX;
    newCell.y = bestY;

    const unsigned newCellIndex = localRegion.nCells++;
    localRegion.localCells[newCellIndex] = newCell;
    localRegion.localCells[newCellIndex].i = newCellIndex;

    for (unsigned r = 0; r != nRows; ++r) {
        if (bestIntervals[r] < 0) {
            continue;
        }
        const LocalCell* rCell = intervals[r][bestIntervals[r]].R.cell;
        if (rCell == NULL) {
            LocalSegment& localSeg = localRegion.localSegments[r];
            localSeg.localCells[localSeg.nCells] = newCellIndex;
            localSeg.nCells++;
        } else {
            int cellRIndex = rCell->i;
            LocalSegment& localSeg = localRegion.localSegments[r];
            for (int i = localSeg.nCells - 1; i >= 0; i--) {
                localSeg.localCells[i + 1] = localSeg.localCells[i];
                if (localSeg.localCells[i] == cellRIndex) {
                    localSeg.localCells[i] = newCellIndex;
                    localSeg.nCells++;
                    break;
                }
            }
        }
    }

    // put all cell back to original position
    // localRegion.reset();
    for (unsigned i = 0; i < localRegion.nCells; i++) {
        pos[i] = localRegion.localCells[i].x;
    }

    localRegion.placeSpreadL(newCellIndex, pos, cellTypeMaps);
    localRegion.placeSpreadR(newCellIndex, pos, cellTypeMaps);
#ifdef MLL_VERIFY_ESTIMATION
    bool match = true;
    for (unsigned i = 0; i < localRegion.nCells - 1; i++) {
        if (bestPosL[i] >= 0 && localRegion.localCells[i].x != bestPosL[i]) {
            printlog(LOG_ERROR, "mismatch L : %d : %d != %d", i, localRegion.localCells[i].x, bestPosL[i]);
            match = false;
        }
        if (bestPosR[i] >= 0 && localRegion.localCells[i].x != bestPosR[i]) {
            printlog(LOG_ERROR, "mismatch R : %d : %d != %d", i, localRegion.localCells[i].x, bestPosR[i]);
            match = false;
        }
    }
    int estimateDisp = 0;
    int actualDisp = 0;
    for (unsigned i = 0; i < localRegion.nCells - 1; i++) {
        if (bestPosL[i] >= 0) {
            estimateDisp += std::abs(localRegion.localCells[i].ox - bestPosL[i]);
        }
        if (bestPosR[i] >= 0) {
            estimateDisp += std::abs(localRegion.localCells[i].ox - bestPosR[i]);
        }
    }
    for (unsigned i = 0; i < localRegion.nCells - 1; i++) {
        actualDisp += std::abs(localRegion.localCells[i].x - localRegion.localCells[i].ox);
    }
    int goldenDisp = (bestCost - bestTargetCostX * siteW - bestTargetCostY * siteH) / siteW;
    if (goldenDisp != estimateDisp) {
        match = false;
    }
    if (estimateDisp != actualDisp) {
        match = false;
    }
    if (!match) {
        printlog(LOG_ERROR, "estimated : %d", estimateDisp);
        printlog(LOG_ERROR, "actual    : %d", actualDisp);
        printlog(LOG_ERROR, "golden    : %d", goldenDisp);
        printlog(LOG_ERROR, "target    : %d,%d", bestTargetCostX, bestTargetCostY);
        printlog(LOG_ERROR, "local     : %d,%d", bestLocalCostX, bestLocalCostY);
        getchar();
    }
#endif

    m.target.x(bestX);
    m.target.y(bestY);

    // log local cell movement
    int nCells = localRegion.nCells - 1;
    for (int i = 0; i < nCells; i++) {
        LocalCell& cell = localRegion.localCells[i];
        if (cell.fixed) {
            assert(cell.x == cell.ox() && cell.y == cell.oy());
            continue;
        }
        assert(cell.y == cell.parent->ly());
        if (cell.x != cell.parent->lx() || cell.x != pos[cell.i]) {
            m.legals.push_back(CellMove(cell.parent, pos[cell.i], cell.y));
        }
    }

    return true;
}
