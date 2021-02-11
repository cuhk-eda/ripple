#include "dp_data.h"

#include "dp_global.h"
using namespace dp;

int Pin::maxCellPinRIndex = -1;

DPlacer::DPlacer() {
    multiRowMode = true;

#ifdef MAINTAIN_HIST
    hist.reserve(1);
    maxDisp = 0;
#endif

    counts.resize(8, 0);
}

DPlacer::~DPlacer() {
    if (tQ) {
        delete tQ;
        tQ = nullptr;
    }
    if (rQ) {
        delete rQ;
        rQ = nullptr;
    }
    if (tPtok) {
        delete tPtok;
        tPtok = nullptr;
    }

    CLEAR_POINTER_LIST(cells);
    CLEAR_POINTER_LIST(nets);
    CLEAR_POINTER_LIST(pins);
}

void DPlacer::getAvail(
    const vector<vector<bool>>& hNR, vector<bool>& isSeg, unsigned maxH, unsigned spaceL, unsigned spaceR) {
    vector<bool> hA(maxH - 1 + spaceL, false);
    for (const vector<bool>& v : hNR) {
        bool a = false;
        for (bool b : v) {
            if (b) {
                a = true;
                break;
            }
        }
        hA.push_back(a);
    }
    for (unsigned i = 0; i != spaceR; ++i) {
        hA.push_back(false);
    }
    unsigned core = hNR.size();
    isSeg.resize(core, true);
    for (unsigned i = 0; i != core; ++i) {
        bool a = false;
        for (unsigned j = 0; j != maxH + spaceL + spaceR; ++j) {
            if (hA[i + j]) {
                a = true;
                break;
            }
        }
        if (a) {
            continue;
        }
        isSeg[i] = false;
    }
}

bool DPlacer::checkGeoMap(const db::GeoMap& geoMap, const int lx, const int ly, const int hx, const int hy) const {
    for (const auto& [ key, geo ] : geoMap) {
        const char direction = geo.layer.direction;
        const int spacing = geo.layer.spacing;
        switch (direction) {
            case 'h':
                if (geo.ly < hy + spacing && geo.hy + spacing > ly) {
                    return true;
                }
                break;
            case 'v':
                if (geo.lx < hx + spacing && geo.hx + spacing > lx) {
                    return true;
                }
                break;
            default:
                printlog(LOG_ERROR, "unrecognized layer direction %c", direction);
                break;
        }
    }
    return false;
}

void DPlacer::getCellTypeMapFromBound(CellTypeMap<MLLCost>* cellTypeMapFx, const db::CellType* dbType, const unsigned i, const int lx, const int ly, const int hx, const int hy, const int rIndex) {
    //  snet bounds
    int slx = INT_MAX;
    int sly = INT_MAX;
    int shx = INT_MIN;
    int shy = INT_MIN;
    int srIndex = -1;
    // absolute pin
    int plx = INT_MAX;
    int ply = INT_MAX;
    int phx = INT_MIN;
    int phy = INT_MIN;
    // flipped relative pin bounds
    int flx = INT_MAX;
    int fhx = INT_MIN;
    int sdx = 0;
    int sdy = 0;

    for (unsigned j = 0; j != snetGeoMaps.size(); ++j) {
        if (rIndex != (int)j && rIndex != (int)j + 1) {
            continue;
        }
        for (const db::GeoMap& geoMap : snetGeoMaps[j]) {
            if (geoMap.empty()) {
                continue;
            }

            const int spacing = geoMap.begin()->second.layer.spacing;
            switch (geoMap.begin()->second.layer.direction) {
                case 'h':
                    sly = geoMap.ly;
                    shy = geoMap.hy;
                    if (geoMap.size() == 1) {
                        sdy = INT_MAX;
                    } else {
                        sdy = geoMap.front2().ly - sly;
                    }
                    for (unsigned k = 0; k != (unsigned)coreHY; ++k) {
                        switch (rowFlip(i, k)) {
                            case 'a':
                            case 'n':
                                ply = siteH * k + dbLY + ly;
                                phy = siteH * k + dbLY + hy;
                                break;
                            case 'y':
                                ply = siteH * k + dbLY + dbType->height - hy;
                                phy = siteH * k + dbLY + dbType->height - ly;
                                break;
                            default:
                                continue;
                        }
                        int pmy = (phy + spacing - sly) % sdy;
                        if (phy + spacing > sly && ply < shy + spacing && pmy > 0 && pmy < static_cast<int>(geoMap.begin()->second.h() + hy - ly) + spacing * 2) {
                            assert(checkGeoMap(geoMap, lx, ply, hx, phy));
                            cellTypeMaps[i].accH(k, j + rIndex, 1);
                        } else {
                            assert(!checkGeoMap(geoMap, lx, ply, hx, phy));
                        }
                    }
                    break;
                case 'v':
                    slx = geoMap.lx;
                    shx = geoMap.hx;
                    if (geoMap.size() == 1) {
                        sdx = INT_MAX;
                    } else {
                        sdx = geoMap.front2().lx - slx;
                    }
                    for (unsigned k = 0; k != (unsigned)coreHX; ++k) {
                        plx = siteW * k + dbLX + lx;
                        phx = siteW * k + dbLX + hx;
                        flx = siteW * k + dbLX + dbType->width - hx;
                        fhx = siteW * k + dbLX + dbType->width - lx;
                        int pmx = (phx + spacing - slx) % sdx;
                        int fmx = (fhx + spacing - slx) % sdx;
                        if (phx + spacing > slx && plx < shx + spacing && pmx > 0 && pmx < static_cast<int>(geoMap.begin()->second.w() + hx - lx) + spacing * 2) {
                            assert(checkGeoMap(geoMap, plx, ly, phx, hy));
                            cellTypeMaps[i].accV(k, j + rIndex, 1);
                        } else {
                            assert(!checkGeoMap(geoMap, plx, ly, phx, hy));
                        }
                        if (cellTypeMapFx) {
                            if (fhx + spacing > slx && flx < shx + spacing && fmx > 0 && fmx < static_cast<int>(geoMap.begin()->second.w() + hx - lx) + spacing * 2) {
                                assert(checkGeoMap(geoMap, flx, ly, fhx, hy));
                                cellTypeMapFx->accV(k, j + rIndex, 1);
                            } else {
                                assert(!checkGeoMap(geoMap, flx, ly, fhx, hy));
                            }
                        }
                    }
                    break;
                default:
                    printlog(LOG_ERROR, "unrecognized layer direction %c", geoMap.begin()->second.layer.direction);
                    break;
            }
        }
    }

    for (db::IOPin* iopin : database.iopins) {
        iopin->getBounds(slx, sly, shx, shy, srIndex);
        if (srIndex != rIndex && srIndex != rIndex + 1) {
            continue;
        }
        const int spacing = database.getRLayer(srIndex)->spacing;
        const int clx = max(0.5, floor((slx - spacing - hx - dbLX) / static_cast<double>(siteW) + 1));
        const int cly = max(0.5, floor((sly - spacing - hy - dbLY) / static_cast<double>(siteH) + 1));
        const int flx =
            max(0.5, floor((slx - spacing + lx - dbType->width - dbLX) / static_cast<double>(siteW) + 1));
        const int fly =
            max(0.5, floor((sly - spacing + ly - dbType->height - dbLY) / static_cast<double>(siteH) + 1));
        const int chx = min(coreHX - 0.5, ceil((shx + spacing - lx - dbLX) / static_cast<double>(siteW) - 1));
        const int chy = min(coreHY - 0.5, ceil((shy + spacing - ly - dbLY) / static_cast<double>(siteH) - 1));
        const int fhx = min(coreHX - 0.5,
                            ceil((shx + spacing + hx - dbType->width - dbLX) / static_cast<double>(siteW) - 1));
        const int fhy = min(
            coreHY - 0.5, ceil((shy + spacing + hy - dbType->height - dbLY) / static_cast<double>(siteH) - 1));
        for (int j = clx; j <= chx; ++j) {
            for (int k = cly; k <= chy; ++k) {
                char flip = rowFlip(i, k);
                if (flip == 'a' || flip == 'n') {
                    cellTypeMaps[i].accIO(k, j, srIndex + rIndex - 1, 1);
                }
            }
            for (int k = fly; k <= fhy; ++k) {
                char flip = rowFlip(i, k);
                if (flip == 'y') {
                    cellTypeMaps[i].accIO(k, j, srIndex + rIndex - 1, 1);
                }
            }
        }
        if (cellTypeMapFx) {
            for (int j = flx; j <= fhx; ++j) {
                for (int k = cly; k <= chy; ++k) {
                    char flip = rowFlip(i, k);
                    if (flip == 'a' || flip == 'n') {
                        cellTypeMapFx->accIO(k, j, srIndex + rIndex - 1, 1);
                    }
                }
                for (int k = fly; k <= fhy; ++k) {
                    char flip = rowFlip(i, k);
                    if (flip == 'y') {
                        cellTypeMapFx->accIO(k, j, srIndex + rIndex - 1, 1);
                    }
                }
            }
        }
    }
}

int DPlacer::getCellTypeMap(const vector<db::CellType*>& dbCellTypes, unsigned& index, mutex& indexMtx) {
    //  relative pin bounds
    int lx = INT_MAX;
    int ly = INT_MAX;
    int hx = INT_MIN;
    int hy = INT_MIN;
    while (true) {
        indexMtx.lock();
        if (index == dbCellTypes.size()) {
            indexMtx.unlock();
            return 0;
        }
        unsigned i = index;
        ++index;
        indexMtx.unlock();
        const db::CellType* dbType = dbCellTypes[i];
        CellTypeMap<MLLCost>* cellTypeMapFx = nullptr;
        if (dbType->edgetypeL == dbType->edgetypeR) {
            cellTypeMapFx = new CellTypeMap(cellTypeMaps[i]);
        }

        for (const db::PinType* pin : dbType->pins) {
            char type = pin->type();
            if (type == 'p' || type == 'g') {
                continue;
            }

            pin->getBounds(lx, ly, hx, hy);
            getCellTypeMapFromBound(cellTypeMapFx, dbType, i, lx, ly, hx, hy, pin->shapes[0].layer.rIndex);
        }

        for (const db::Geometry& obs : dbType->obs()) {
            getCellTypeMapFromBound(cellTypeMapFx, dbType, i, obs.lx, obs.ly, obs.hx, obs.hy, obs.layer.rIndex);
        }

        cellTypeMaps[i].update(cellTypeMapFx);
        delete cellTypeMapFx;
    }
}

void DPlacer::input() {
    vector<int> cellLookup;
    unordered_map<db::CellType*, int> dpCellTypes;

    siteW = database.siteW;
    siteH = database.siteH;
    dbLX = database.coreLX;
    dbLY = database.coreLY;
    dbHX = database.coreHX;
    dbHY = database.coreHY;
    coreLX = 0;
    coreLY = 0;
    coreHX = (dbHX - dbLX) / siteW;
    coreHY = (dbHY - dbLY) / siteH;

    nRegions = database.getNumRegions();

    binW = siteH / siteW * 9;
    binH = 9;
    density.setup(coreLX, coreLY, coreHX, coreHY, binW, binH, nRegions);

    totalCellArea.resize(nRegions, 0);
    totalFreeArea.resize(nRegions, 0);

    unsigned nSRCells = 0;
    unsigned nMRCells = 0;

    // search for the first valid power rail
    // top-power-bottom-ground => 'e'
    // top-ground-bottom-power => 'o'
    bool evenRowBottomIsGround = true;
    for (unsigned r = 0; r != database.rows.size(); ++r) {
        db::Row* dbRow = database.rows[r];
        if (!dbRow->isPowerValid()) {
            continue;
        }
        if (dbRow->topPower() == 'p') {
            evenRowBottomIsGround = (r % 2 == 0);
            break;
        } else {
            evenRowBottomIsGround = (r % 2 == 1);
            break;
        }
    }

    unsigned maxW = 0;
    unsigned maxH = 0;
    unordered_map<string, db::CellType*> dbCellTypesMap;
    unsigned nDBCells = database.cells.size();
    for (unsigned i = 0; i != nDBCells; ++i) {
        db::Cell* dbCell = database.cells[i];
        if (dbCell->fixed()) {
            continue;
        }

        int dpType = -1;
        db::CellType* dbType = dbCell->ctype();
        db::CellType* dbClass = nullptr;
        unordered_map<string, db::CellType*>::const_iterator ct = dbCellTypesMap.find(dbType->name);
        if (ct != dbCellTypesMap.end()) {
            dbClass = ct->second;
        }
        if (!dbClass) {
            for (db::CellType* dbCellType : dbCellTypes) {
                if (*dbCellType == *dbType) {
                    dbClass = dbCellType;
                    dbCellTypesMap.emplace(dbType->name, dbClass);
#ifndef NDEBUG
                    printlog(LOG_WARN, "Merge cell type %s to %s", dbType->name.c_str(), dbClass->name.c_str());
#endif
                    break;
                }
            }
        }
        if (!dbClass) {
            dpType = nDPTypes++;
            dpCellTypes[dbType] = dpType;
            dbCellTypes.push_back(dbType);
            dbCellTypesMap.emplace(dbType->name, dbType);
            _row.push_back(0);
            if (!DPModule::MLLPGAlign) {
                setOddRowFlip(dpType);
                setOddRowNoFlip(dpType);
                setEvenRowFlip(dpType);
                setEvenRowNoFlip(dpType);
            } else if (dbType->height / siteH % 2 == 0) {
                if ((dbType->botPower() == 'g' && !evenRowBottomIsGround) ||
                    (dbType->botPower() == 'p' && evenRowBottomIsGround)) {
                    // odd row match => don't care on odd, no place on even
                    setOddRowFlip(dpType);
                    setOddRowNoFlip(dpType);
                } else {
                    // even row match => don't care on even, no place on odd
                    setEvenRowFlip(dpType);
                    setEvenRowNoFlip(dpType);
                }
            } else {
                if ((dbType->botPower() == 'g' && !evenRowBottomIsGround) ||
                    (dbType->botPower() == 'p' && evenRowBottomIsGround)) {
                    // odd row match => no flip on odd, flip on even
                    setOddRowNoFlip(dpType);
                    setEvenRowFlip(dpType);
                } else {
                    // even row match => no flip on even, flip on odd
                    setEvenRowNoFlip(dpType);
                    setOddRowFlip(dpType);
                }
            }
        } else {
            dpType = dpCellTypes[dbClass];
        }
        dp::Cell* dpCell = addCell(dbCell, _row[dpType]);
        dpCell->id = i;
        dpCell->type = dpType;
        maxW = max(maxW, dpCell->w);
        maxH = max(maxH, dpCell->h);

        if (dpCell->h > 1) {
            ++nMRCells;
            //  multiRowMode = true;
        } else {
            ++nSRCells;
        }

        totalCellArea[dpCell->fid] += dpCell->w * dpCell->h;

        dpCell->segments.resize(dpCell->h, nullptr);
        dbCellMap[dbCell] = dpCell;
    }

#ifndef NDEBUG
    printlog(LOG_INFO, "h1 = %u h2 = %u h3 = %u h4 = %u", counts[0], counts[1], counts[2], counts[3]);
#endif

    Cell::setSpace(dbCellTypes);

    //  input nets and pins
    unsigned nDPNets = 0;
    for (db::Net* dbNet : database.nets) {
        Net* dpNet = addNet();
        dpNet->i = nDPNets++;
        for (db::Pin* dbPin : dbNet->pins) {
            if (dbPin->cell) {
                Pin* dpPin = addPin();

                int lx, ly, hx, hy;
                dbPin->type->getBounds(lx, ly, hx, hy);
                dpPin->net = dpNet;
                dpPin->cell = dbCellMap[dbPin->cell];
                if (dpPin->cell) {
                    dpPin->x = (hx + lx) * 0.5;
                    dpPin->y = (hy + ly) * 0.5;
                    dpPin->cell->pins.push_back(dpPin);
                    Pin::maxCellPinRIndex = max(Pin::maxCellPinRIndex, dbPin->type->shapes[0].layer.rIndex);
                } else {
                    dpPin->x = dbPin->cell->lx() - dbLX + (hx + lx) * 0.5;
                    dpPin->y = dbPin->cell->ly() - dbLY + (hy + ly) * 0.5;
                }
                dpNet->pins.push_back(dpPin);
            } else if (dbPin->iopin) {
                Pin* dpPin = addPin();

                int lx, ly, hx, hy;
                dbPin->type->getBounds(lx, ly, hx, hy);
                dpPin->x = dbPin->iopin->x - dbLX + (hx + lx) * 0.5;
                dpPin->y = dbPin->iopin->y - dbLY + (hy + ly) * 0.5;
                dpPin->net = dpNet;
                dpPin->cell = NULL;

                dpNet->pins.push_back(dpPin);
            } else {
                printlog(LOG_ERROR, "dangling pin detected");
            }
        }
    }

    //	calculate cell type map
    unsigned nSnets = database.snets.size();
    snetGeoMaps.resize(Pin::maxCellPinRIndex + 1, vector<db::GeoMap>(nSnets));
    for (unsigned i = 0; i != nSnets; ++i) {
        for (const db::Geometry& shape : database.snets[i]->shapes) {
            const int rIndex = shape.layer.rIndex;
            if (rIndex >= 1 && rIndex <= Pin::maxCellPinRIndex + 1) {
                db::GeoMap& geoMap = snetGeoMaps[rIndex - 1][i];
                switch (shape.layer.direction) {
                    case 'h':
                        geoMap.emplace(shape.cy(), shape);
                        break;
                    case 'v':
                        geoMap.emplace(shape.cx(), shape);
                        break;
                    default:
                        printlog(LOG_ERROR, "unrecognized layer direction %c", shape.layer.direction);
                        break;
                }
            }
        }
    }

#ifndef NDEBUG
    printlog(LOG_INFO, "nDPTypes = %u , coreHY = %u , coreHX = %u", nDPTypes, coreHY, coreHX);
#endif

    cellTypeMaps.resize(nDPTypes, CellTypeMap<MLLCost>(coreHY, coreHX, Pin::maxCellPinRIndex * 2 + 1, true));

    if (DPModule::EnablePinAcc) {
        unsigned index = 0;
        mutex indexMtx;
        vector<future<int>> futs;
        for (unsigned i = 1; i < DPModule::CPU; ++i) {
            futs.push_back(async(&DPlacer::getCellTypeMap, this, ref(dbCellTypes), ref(index), ref(indexMtx)));
        }
        getCellTypeMap(dbCellTypes, index, indexMtx);
        for (future<int>& fut : futs) {
            fut.get();
        }
    }

    std::sort(cells.begin(), cells.end(), [this](const Cell* a, const Cell* b) {
        //  area decreasing
        if ((a->w * a->h) != (b->w * b->h)) {
            return (a->w * a->h) > (b->w * b->h);
        }
        return cellTypeMaps[a->type].hSum() > cellTypeMaps[b->type].hSum();
    });

    for (unsigned i = 0; i < cells.size(); ++i) {
        cells[i]->i = i;
    }

    //	input segments
    long segArea = 0;
    unsigned nDBRows = database.rows.size();
    segments.resize(nDBRows);
    for (unsigned r = 0; r != nDBRows; ++r) {
        db::Row* dbRow = database.rows[r];
        //  Segment* dpSegment = nullptr;
        unsigned s = 0;
        for (const db::RowSegment& dbSegment : dbRow->segments) {
            addSegment(dbSegment, s, r, (dbSegment.x - dbLX) / database.siteW, dbSegment.w / database.siteW);
        }
        //  cout << endl;
    }

    for (vector<Segment>& segment : segments) {
        for (Segment& seg : segment) {
            seg.cells.clear();
            int sw = seg.w;
            segArea += sw;
        }
    }

#ifndef NDEBUG
    printlog(LOG_INFO,
             "\u250e\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2530\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2530\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2530\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2512");
    printlog(LOG_INFO, "\u2503  region  \u2503 cell area \u2503 free area \u2503 density \u2503");
    printlog(LOG_INFO,
             "\u2520\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2542\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2542\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2542\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2528");
#endif

    for (unsigned i = 0; i != nRegions; ++i) {
        db::Region* region = database.regions[i];
        region->density = totalCellArea[i] / (double)totalFreeArea[i];

#ifndef NDEBUG
        if (region->density >= 1.0) {
            printlog(LOG_ERROR,
                     "\u2503 %-8s \u2503 %9d \u2503 %9d \u2503  %5.1lf%% \u2503",
                     region->name().c_str(),
                     totalCellArea[i],
                     totalFreeArea[i],
                     region->density * 100.0);
        } else {
            printlog(LOG_INFO,
                     "\u2503 %-8s \u2503 %9d \u2503 %9d \u2503  %5.1lf%% \u2503",
                     region->name().c_str(),
                     totalCellArea[i],
                     totalFreeArea[i],
                     region->density * 100.0);
        }
#endif
    }

#ifndef NDEBUG
    printlog(LOG_INFO,
             "\u2516\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2538\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2538\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
             "\u2538\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u251a");
#endif
}

void DPlacer::output() {
    for (db::Cell* dbCell : database.cells) {
        if (dbCell->fixed()) {
            continue;
        }
        Cell* dpCell = dbCellMap[dbCell];
        if (!dpCell->placed) {
            printlog(LOG_ERROR, "cell %s not placed after DP", dbCell->name().c_str());
            continue;
        }
        int x = dpCell->lx() * siteW + dbLX;
        int y = dpCell->ly() * siteH + dbLY;
        dbCell->place(x, y, dpCell->fx(), dpCell->fy());
    }
}

#ifndef NDEBUG
void DPlacer::evalPin(vector<unsigned>& pin) {
    unsigned nLayers = cellTypeMaps[0].nLayers();
    pin.resize(nLayers, 0);
    for (Cell* cell : cells) {
        for (unsigned i = 0; i != nLayers; ++i) {
            pin[i] += cellTypeMaps[cell->type].n(cell->ly(), cell->lx())[i];
        }
    }
}
#endif

Cell* DPlacer::addCell(db::Cell* dbCell, char row) {
    Cell* dpCell = new Cell(dbCell, siteW, siteH, dbLX, dbLY, row);
    cells.push_back(dpCell);
    ++counts[dpCell->h - 1];
    return dpCell;
}

Net* DPlacer::addNet() {
    Net* newnet = new Net();
    nets.push_back(newnet);
    return newnet;
}

Pin* DPlacer::addPin() {
    Pin* newpin = new Pin();
    pins.push_back(newpin);
    return newpin;
}

Segment* DPlacer::addSegment(const db::RowSegment& dbSegment, unsigned& s, unsigned r, int dpsx, unsigned dpsw) {
    Segment dpSegment(s++, dpsx, r, dpsw);

    db::Row* dbRow = database.rows[r];
    dpSegment.flip = dbRow->flip();

    if (dbSegment.region) {
        dpSegment.fid = (int)dbSegment.region->id;
    } else {
        printlog(LOG_ERROR, "region is NULL");
        dpSegment.fid = 0;
    }
    // dpSegment.space = dpSegment.w;
    totalFreeArea[dpSegment.fid] += dpSegment.w;
    segments[r].push_back(dpSegment);
    return &segments[r].back();
}

bool DPlacer::getCellAt(
    Cell* cell, int x, int y, Cell*& cellL, Cell*& cellR, CellListIter& iterL, CellListIter& iterR, Segment*& seg) {
    seg = getSegmentAt(cell, x, y);
    if (!seg) {
        return false;
    }
    seg->getCellAt(x, y, cellL, cellR, iterL, iterR);
    return true;
}

void DPlacer::insertCell(Cell* cell) {
    for (unsigned y = 0; y != cell->h; ++y) {
        Segment* seg = getSegmentAt(cell, cell->lx(), cell->ly() + y);
        if (!seg) {
            printlog(LOG_ERROR,
                     "seg not found for cell %s of type %s at x = %d, y = %d",
                     cell->getDBCell()->name().c_str(),
                     cell->getDBCellType()->name.c_str(),
                     cell->lx(),
                     cell->ly() + y);
            return;
        }
        //  cout << "try lock seg\n";
        unique_lock<shared_mutex> lock(seg->mtx);
        //  cout << "success lock seg\n";
        seg->addCell(cell);
    }
    cell->placed = true;
}

void DPlacer::removeCell(Cell* cell) {
    if (!cell->placed) {
        return;
    }
    for (Segment* seg : cell->segments) {
        unique_lock<shared_mutex> lock(seg->mtx);
        seg->removeCell(cell);
    }
    cell->placed = false;
}
void DPlacer::moveCell(Cell* cell, int x, int y) {
    bool fx = cellTypeMaps[cell->type].fx(y, x);
    bool fy = false;
    char flip = cell->rowFlip(y);
    if (flip == 'x') {
        //	cell have no p/g pin
        fy = segments[y][0].flip;
        //  if don't care ('a'), do not change the flipping
    } else if (flip == 'y') {
        fy = true;
    }
    moveCell(cell, x, y, fx, fy);
}

void DPlacer::moveCell(Cell* cell, int x, int y, bool fx, bool fy) {
    removeCell(cell);
#ifdef MAINTAIN_HIST
    {
        unique_lock<shared_mutex> lock(histMtx);
        //  cout << "success lock hist\n";
        updateHist(hist, maxDisp, cell, x, y);
    }
#endif
    cell->lx(x);
    cell->ly(y);
    cell->fx(fx);
    cell->fy(fy);
    insertCell(cell);
}

void DPlacer::shiftCell(Cell* cell, int x, int y) {
#ifdef MAINTAIN_HIST
    {
        unique_lock<shared_mutex> lock(histMtx);
        //  cout << "success lock hist\n";
        updateHist(hist, maxDisp, cell, x, y);
    }
#endif
    cell->lx(x);
    cell->ly(y);
    cell->fx(cellTypeMaps[cell->type].fx(y, x));
}

#ifdef MAINTAIN_HIST
void DPlacer::updateHist(vector<unsigned>& hist, unsigned& maxDisp, Cell* cell, int x, int y) {
    const unsigned nDisp = abs(x - cell->ox()) + abs(y - cell->oy()) * siteH / siteW + 0.5;
    if (nDisp >= hist.size()) {
        hist.resize(max((size_t)nDisp + 1, hist.size() * 2));
    }
    ++hist[nDisp];
    maxDisp = max(maxDisp, nDisp);
    if (cell->placed) {
        const unsigned oDisp = abs(cell->lx() - cell->ox()) + abs(cell->ly() - cell->oy()) * siteH / siteW + 0.5;
        --hist[oDisp];
        if (oDisp == maxDisp && !hist[oDisp]) {
            for (; maxDisp; --maxDisp) {
                if (hist[maxDisp]) {
                    break;
                }
            }
        }
    }
}
#endif

Segment* DPlacer::getSegmentAt(const Cell* cell, const int x, const int y) {
    if (y < 0 || y >= static_cast<int>(segments.size())) {
        return nullptr;
    }
    for (Segment& seg : segments[y]) {
        if (seg.fid != cell->fid) {
            continue;
        }
        if (x < seg.boundL()) {
            return nullptr;
        }
        if (x + (int)cell->w <= seg.boundR()) {
            return &seg;
        }
    }
    return nullptr;
}

const Segment* DPlacer::getNearestSegment(Cell* cell, const int x, const int y) {
    db::Region* reg = database.regions[cell->fid];
    const int regx = max(lround(ceil((reg->lx - dbLX) / static_cast<double>(siteW))),
                         min<long>(lround(floor((reg->hx - dbLX) / static_cast<double>(siteW))) - 1, x));
    const int regy = max(lround(ceil((reg->ly - dbLY) / static_cast<double>(siteH))),
                         min<long>(lround(floor((reg->hy - dbLY) / static_cast<double>(siteH))) - 1, y));
    int bestDist = INT_MAX;
    const Segment* bestSeg = nullptr;

    for (int t = 1; t < coreHY * 2; ++t) {
        //  sequence: { 0, -1, 1, -2, 2... }
        const int dy = (t % 2) ? (t / 2) : -(t / 2);
        const int ry = regy + dy;
        if (ry < 0 || ry > coreHY - 1) {
            continue;
        }
        const int distY = abs(regy - ry);
        if (distY * siteH > bestDist) {
            break;
        }
        for (const Segment& seg : segments[ry]) {
            if (seg.fid != cell->fid) {
                continue;
            }
            if (seg.w < cell->w) {
                //  printlog(LOG_INFO, "segment width %u is less than cell width %u", seg.w, cell->w);
                continue;
            }
            int distX = 0;
            const int lx = seg.boundL();
            const int hx = seg.boundR();
            if (regx < lx) {
                distX = lx - regx;
            } else if (regx > hx) {
                distX = regx - hx;
            }
            int dist = distX * siteW + distY * siteH;
            if (dist < bestDist) {
                bestDist = dist;
                bestSeg = &seg;
            }
        }
    }
    return bestSeg;
}

Segment* DPlacer::getNearestSegmentInRow(Cell* cell, int x, int y) {
    if (y < 0 || y >= (int)segments.size()) {
        return nullptr;
    }
    int bestDist = INT_MAX;
    Segment* bestSeg = nullptr;
    for (Segment& seg : segments[y]) {
        if (seg.fid != cell->fid) {
            continue;
        }
        int lx = seg.boundL();
        int hx = seg.boundR();
        int dist = 0;
        if (x < lx) {
            dist = lx - x;
        } else if (x > hx) {
            dist = x - hx;
        }
        if (dist < bestDist) {
            bestDist = dist;
            bestSeg = &seg;
        }
    }
    return bestSeg;
}

#ifndef NEW_PLACEL_PLACER
double DPlacer::defineLocalRegion(LocalRegion& localRegion, Cell* targetCell, const Region& region) {
    localRegion.clear();

    const int lx = region.lx;
    const int ly = region.ly;
    const int hx = region.hx;
    const int hy = region.hy;
    localRegion.lx = lx;
    localRegion.ly = ly;
    localRegion.hx = hx;
    localRegion.hy = hy;
    localRegion.isMax(region.isMax());

    const unsigned nRows = hy - ly;
    const unsigned maxRowCells = (hx - lx);
    const unsigned maxLocalCells = std::min((unsigned)((hx - lx) * (hy - ly)), (unsigned)cells.size());
    const unsigned oldSize = localRegion.localSegments.size();
    if (oldSize < nRows) {
        localRegion.localSegments.resize(nRows);
        for (unsigned i = oldSize; i != nRows; ++i) {
            localRegion.localSegments[i].localCells.reserve(maxRowCells);
            localRegion.localSegments[i].nCells = 0;
        }
    }
    if (localRegion.localSegments[0].localCells.size() < maxRowCells) {
        for (LocalSegment& seg : localRegion.localSegments) {
            seg.localCells.reserve(maxRowCells);
        }
    }

    if (localRegion.localCells.size() < maxLocalCells) {
        localRegion.localCells.resize(maxLocalCells);
        localRegion.localCellList.reserve(maxLocalCells);
    }

    for (LocalSegment& seg : localRegion.localSegments) {
        seg.nCells = 0;
    }

    localRegion.nSegments = nRows;
    localRegion.nCells = 0;
    unsigned nLocalCells = 0;

    unsigned totalCellArea = 0;
    unsigned totalFreeArea = 0;

    for (int ry = ly; ry != hy; ++ry) {
        unsigned r = ry - ly;

        LocalSegment& localSeg = localRegion.localSegments[r];
        localSeg.x = lx;
        localSeg.y = ry;
        localSeg.w = hx - lx;

        int L = lx;
        int R = hx;
        if (localRegion.useCount == DEBUG_ID) {
            printlog(LOG_INFO, "%d : %d", localSeg.y, localSeg.nCells);
        }
        for (Segment& seg : segments[ry]) {
            if (seg.boundR() <= lx) {
                continue;
            }
            if (seg.fid != targetCell->fid) {
                continue;
            }

            R = min(seg.boundL(), hx);
            if (L < R) {
                int fixX = L;
                int fixY = ry;
                int fixW = R - L;
                int fixH = 1;
                localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
                localSeg.localCells[localSeg.nCells++] = nLocalCells++;
                if (localRegion.useCount == DEBUG_ID) {
                    printlog(
                        LOG_INFO, "add fix cell a %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
                }
            }
            L = min(seg.boundR(), hx);
            R = hx;
            if (seg.boundL() >= hx) {
                break;
            }

            totalCellArea += seg.setLocalRegion(localRegion, nLocalCells, targetCell->fid);
            totalFreeArea += max(0, L - max(seg.boundL(), lx));
        }
        if (L < R) {
            // for ending out-of-segment
            int fixX = L;
            int fixY = ry;
            int fixW = R - L;
            int fixH = 1;
            localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
            localSeg.localCells[localSeg.nCells++] = nLocalCells++;
            if (localRegion.useCount == DEBUG_ID) {
                printlog(LOG_INFO, "add fix cell e %d to row %d (%d)", nLocalCells - 1, localSeg.y, localSeg.nCells);
            }
        }
    }

    localRegion.nCells = nLocalCells;
    if (localRegion.useCount == DEBUG_ID) {
        printlog(LOG_INFO, "total #cells = %d", nLocalCells);
    }

    for (unsigned i = 0; i != nLocalCells; ++i) {
        localRegion.localCells[i].i = i;
#ifdef MLL_WEIGHTED_DISPLACEMENT
        localRegion.localCells[i].weight = 1.0 + 0.001 / (double)localRegion.nCells;
#endif
    }
    if (localRegion.useCount == DEBUG_ID) {
        localRegion.report();
    }
    return (totalCellArea + targetCell->w * targetCell->h) / (double)totalFreeArea;
}
#else
void DPlacer::defineLocalRegion(
    LocalRegion& localRegion, Cell* targetCell, int cx, int cy, int lx, int ly, int hx, int hy) {
    // cx and cy are not used
    localRegion.clear();
    localRegion.useCount++;

    localRegion.lx = lx;
    localRegion.ly = ly;
    localRegion.hx = hx;
    localRegion.hy = hy;

    const unsigned nRows = hy - ly;
    const unsigned maxRowCells = (hx - lx);
    const unsigned maxLocalCells = std::min((unsigned)((hx - lx) * (hy - ly)), (unsigned)cells.size());
    if (localRegion.localSegments.size() < nRows) {
        unsigned oldSize = localRegion.localSegments.size();
        unsigned newSize = nRows;
        localRegion.localSegments.resize(newSize);
        for (unsigned i = oldSize; i < newSize; i++) {
            localRegion.localSegments[i].localCells.resize(maxRowCells);
            localRegion.localSegments[i].nCells = 0;
        }
    }
    if (localRegion.localSegments[0].localCells.size() < maxRowCells) {
        for (auto& seg : localRegion.localSegments) {
            seg.localCells.resize(maxRowCells);
            seg.nCells = 0;
        }
    }

    if (localRegion.localCells.size() < maxLocalCells) {
        localRegion.localCells.resize(maxLocalCells);
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
        localRegion.localCellList.resize(maxLocalCells);
#endif
    }

    for (int r = 0; r < nRows; r++) {
        LocalSegment& localSeg = localRegion.localSegments[r];
        localSeg.nCells = 0;
        localSeg.x = lx;
        localSeg.y = ly + r;
        localSeg.w = hx - lx;
    }

    localRegion.nSegments = nRows;
    localRegion.nCells = 0;
    int nLocalCells = 0;

#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
    priority_queue<long, vector<long>> pq;
#endif

    for (int ry = ly; ry < hy; ry++) {
        int L = localRegion.lx;
        int R = localRegion.hx;
        int nRowSegs = segments[ry].size();
        for (int s = 0; s < nRowSegs; s++) {
            Segment& seg = segments[ry][s];
            if (seg.boundR() <= localRegion.lx) {
                continue;
            }
            if (seg.fid != targetCell->fid) {
                continue;
            }

            R = min(seg.boundL(), localRegion.hx);
            if (L < R) {
                int fixX = L;
                int fixY = ry;
                int fixW = R - L;
                int fixH = 1;
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
                localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
                pq.push(packInt(hx - fixX, nLocalCells++));
#else
                localRegion.localCells[nLocalCells++] = LocalCell(fixX, fixY, fixW, fixH);
#endif
            }
            L = min(seg.boundR(), localRegion.hx);
            R = localRegion.hx;
            if (seg.boundL() >= localRegion.hx) {
                break;
            }

            CellListIter iterL, iterR;
            Cell* cellL = NULL;
            Cell* cellR = NULL;
            CellListIter ci;

            shared_lock<shared_mutex> lock(seg->mtx);
            seg.getCellAt(lx, seg.y, cellL, cellR, iterL, iterR);
            if (cellL == NULL) {
                ci = iterR;
            } else {
                ci = iterL;
            }

            CellListIter ce = seg.cells.end();

            for (; ci != ce; ++ci) {
                Cell* cell = seg.cells.getCell(ci);
                if (cell->hx() <= lx) continue;
                if (cell->lx() >= hx) break;

                // overlap cells
                if (cell->lx() < lx && cell->hx() > lx) {
                    int fixX = lx;
                    int fixY = ry;
                    int fixW = min(cell->hx(), hx) - fixX;
                    int fixH = 1;
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
                    localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
                    pq.push(packInt(hx - fixX, nLocalCells++));
#else
                    localRegion.localCells[nLocalCells++] = LocalCell(fixX, fixY, fixW, fixH);
#endif
                    continue;
                }
                if (cell->hx() > hx && cell->lx() < hx) {
                    int fixX = max(cell->lx(), lx);
                    int fixY = ry;
                    int fixW = hx - fixX;
                    int fixH = 1;
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
                    localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
                    pq.push(packInt(hx - fixX, nLocalCells++));
#else
                    localRegion.localCells[nLocalCells++] = LocalCell(fixX, fixY, fixW, fixH);
#endif
                    continue;
                }
                if (cell->ly() < ly || cell->hy() > hy) {
                    int fixX = max(cell->lx(), lx);
                    int fixY = ry;
                    int fixW = min(cell->hx(), hx) - fixX;
                    int fixH = 1;
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
                    localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
                    pq.push(packInt(hx - fixX, nLocalCells++));
#else
                    localRegion.localCells[nLocalCells++] = LocalCell(fixX, fixY, fixW, fixH);
#endif
                    continue;
                }

                // real local cells
                if (cell->ly() == ry) {
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
                    localRegion.localCells[nLocalCells] = LocalCell(cell);
                    pq.push(packInt(hx - cell->lx(), nLocalCells++));
#else
                    localRegion.localCells[nLocalCells++] = LocalCell(cell);
#endif
                }
            }
        }
        if (L < R) {
            // for ending out-of-segment
            int fixX = L;
            int fixY = ry;
            int fixW = R - L;
            int fixH = 1;
#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
            localRegion.localCells[nLocalCells] = LocalCell(fixX, fixY, fixW, fixH);
            pq.push(packInt(hx - fixX, nLocalCells++));
#else
            localRegion.localCells[nLocalCells++] = LocalCell(fixX, fixY, fixW, fixH);
#endif
        }
    }

    localRegion.nCells = nLocalCells;

#ifdef NEW_PLACEL_PLACER_USE_CELL_LIST
    for (int i = 0; i < nLocalCells; i++) {
        localRegion.localCells[i].i = i;
    }
    for (int i = 0; !pq.empty(); pq.pop(), i++) {
        int x, id;
        unpackInt(x, id, pq.top());
        LocalCell& localCell = localRegion.localCells[id];
        localRegion.localCellList[i] = localCell.i;
        int r = localCell.y - ly;
        for (int h = 0; h < localCell.h; h++) {
            LocalSegment& localSeg = localRegion.localSegments[r + h];
            localSeg.localCells[localSeg.nCells++] = localCell.i;
        }
    }
/*
    vector<pair<int,int>> cellSort();
    for (unsigned i = 0; i != nLocalCells; ++i) {
        localRegion.localCells[i].i = i;
        cellSort.emplace_back(i, localRegion.localCells[i].x);
    }
    sort(cellSort.begin(), cellSort.end(), CmpAsce());
    for(unsigned i = 0; i != nLocalCells; ++i) {
        LocalCell &localCell = localRegion.localCells[cellSort[i].first];
        localRegion.localCellList[i] = localCell.i;
        int r = localCell.y - ly;
        for(int h=0; h<localCell.h; h++){
            LocalSegment &localSeg = localRegion.localSegments[r+h];
            localSeg.localCells[localSeg.nCells++] = localCell.i;
        }
    }
    */
#endif

#ifdef NEW_PLACEL_PLACER_USE_SORTED_CELLS
    vector<LocalCell>::iterator cb = localRegion.localCells.begin();
    vector<LocalCell>::iterator ce = localRegion.localCells.begin() + nLocalCells;
    // std::sort(cb, ce, LocalCell::CompareXInc);
    std::sort(db, ce, [](const LocalCell& a, const LocalCell& b) { return a.x < b.x; });

    for (int i = 0; i < nLocalCells; i++) {
        LocalCell& localCell = localRegion.localCells[i];
        localCell.i = i;
        int r = localCell.y - ly;
        for (int h = 0; h < localCell.h; h++) {
            LocalSegment& localSeg = localRegion.localSegments[r + h];
            localSeg.localCells[localSeg.nCells++] = i;
        }
    }
#endif
}
#endif

void DPlacer::updateNet(Move& move) {
    vector<Net*> updateList;
    vector<Pin*>::iterator pi, pe;

    pi = move.target.cell->pins.begin();
    pe = move.target.cell->pins.end();
    for (; pi != pe; ++pi) {
        updateList.push_back((*pi)->net);
    }
    for (int i = 0; i < (int)move.legals.size(); i++) {
        pi = move.legals[i].cell->pins.begin();
        pe = move.legals[i].cell->pins.end();
        for (; pi != pe; ++pi) {
            updateList.push_back((*pi)->net);
        }
    }

    sort(updateList.begin(), updateList.end());
    vector<Net*>::iterator end = unique(updateList.begin(), updateList.end());
    updateList.resize(distance(updateList.begin(), end));

    for (int i = 0; i < (int)updateList.size(); i++) {
        updateList[i]->update();
    }
}

long long DPlacer::getHPWLDiff(Move& move) {
    long long wl_diff = 0;
    for (const Pin* cellpin : move.target.cell->pins) {
        Net* net = cellpin->net;

        const int ox = cellpin->pinX();
        const int oy = cellpin->pinY();
        const int nx = move.target.xLong() * siteW + cellpin->x;
        const int ny = move.target.yLong() * siteH + cellpin->y;

        bool need_update = false;
        if (net->pins.size() <= 3 || net->box.lx == ox || net->box.hx == ox || net->box.ly == oy || net->box.hy == oy) {
            need_update = true;
        }

        Region region = net->box;
        if (need_update) {
            region.clear();
            vector<Pin*>::iterator npi = net->pins.begin();
            vector<Pin*>::iterator npe = net->pins.end();
            for (; npi != npe; ++npi) {
                Pin* pin = *npi;
                double px, py;
                if (pin == cellpin) {
                    px = nx;
                    py = ny;
                } else {
                    px = pin->x;
                    py = pin->y;
                    if (pin->cell) {
                        px += pin->cell->lx() * siteW;
                        py += pin->cell->ly() * siteH;
                    }
                }
                region.cover(px, py);
            }
        } else {
            region.cover(nx, ny);
        }
        wl_diff = wl_diff + region.width() + region.height() - net->hpwl();
    }
    return wl_diff;
}

double DPlacer::getScoreDiff(const Move& move) {
    // maintain the current average movement score
    static double meanForMP = -1.0;
    // maintain the current number of soft constraint violations
    static double svForMP = -1.0;
    // maintain the current max movement score
    static int maxForMP = 0;
    // calculate the original average movement score and the number of soft constraint violations
    if (meanForMP == -1.0 && svForMP == -1.0) {
        meanForMP = 0.0;
        svForMP = 0.0;
        unsigned disp = 0;
        vector<unsigned> Dist(counts.size(), 0);

        for (Cell* cell : cells) {
            svForMP += cellTypeMaps[cell->type].score(cell->ly(), cell->lx());
            disp = cell->displacementX() * siteW + cell->displacementY() * siteH;
            Dist[cell->h - 1] += disp;
            maxForMP = std::max(maxForMP, (int)disp);
        }

        for (unsigned i = 0; i < counts.size(); i++) {
            if (counts[i] > 0) {
                meanForMP += Dist[i] / (double)counts[i] / 4;
            }
        }
    }

    const double originalMean = meanForMP;
    const double originalSv = svForMP;
    const int originalMax = maxForMP;

    double movedMean = 0.0;
    double movedSv = 0.0;
    int movedMax = originalMax;
    int originalDisp = 0;
    int movedDisp = 0;

    //  the target cell
    originalDisp = move.target.cell->displacementX() * siteW + move.target.cell->displacementY() * siteH;
    movedDisp = abs(move.target.xLong() - move.target.cell->ox()) * siteW +
                abs(move.target.yLong() - move.target.cell->oy()) * siteH;
    movedMean = originalMean + (movedDisp - originalDisp) / (double)counts[move.target.cell->h - 1] / 4;
    movedSv = originalSv - cellTypeMaps[move.target.cell->type].score(move.target.cell->ly(), move.target.cell->lx()) +
              cellTypeMaps[move.target.cell->type].score(move.target.yLong(), move.target.xLong());
    movedMax = std::max(movedMax, movedDisp);
    //  the cells which need to move
    for (const CellMove& legal : move.legals) {
        originalDisp = legal.cell->displacementX() * siteW + legal.cell->displacementY() * siteH;
        movedDisp = abs(legal.xLong() - legal.cell->ox()) * siteW + abs(legal.yLong() - legal.cell->oy()) * siteH;
        movedMean = movedMean + (movedDisp - originalDisp) / (double)counts[legal.cell->h - 1] / 4;
        movedSv = movedSv - cellTypeMaps[legal.cell->type].score(legal.cell->ly(), legal.cell->lx()) +
                  cellTypeMaps[legal.cell->type].score(legal.yLong(), legal.xLong());
        movedMax = std::max(movedMax, movedDisp);
    }

    double originalScore = 0.0;
    double movedScore = 0.0;
    double deltaScore = 0.0;

    // originalScore = originalMean * (1 + min(0.2, originalSv / cells.size())) * (1 + originalMax / (double)100);
    // movedScore = movedMean * (1 + min(0.2, movedSv / cells.size())) * (1 + movedMax / (double)100);
    originalScore = min(0.2, originalSv / cells.size());
    movedScore = min(0.2, movedSv / cells.size());

    // printlog(LOG_INFO, "originalMean %lf,originalSv %lf,movedMean %lf,movedSv %lf", originalMean, min(0.2, originalSv
    // / cells.size()), movedMean, min(0.2, movedSv / cells.size()));
    //  calculate the delta score
    deltaScore = movedScore - originalScore;

    //  update meanForMP and svForMP if the target cell will be moved
    if (!move.target.cell->placed || deltaScore < 0.0) {
        meanForMP = movedMean;
        svForMP = movedSv;
        maxForMP = movedMax;
    }

    return deltaScore;
}

int DPlacer::checkOverlap(bool spaceAware) {
    int nCells = cells.size();
    for (int i = 0; i < nCells; i++) {
        cells[i]->overlap = 0;
    }
    vector<vector<Segment>>::iterator ri = segments.begin();
    vector<vector<Segment>>::iterator re = segments.end();
    for (; ri != re; ++ri) {
        vector<Segment>::iterator si = ri->begin();
        vector<Segment>::iterator se = ri->end();
        for (; si != se; ++si) {
            // int L = si->x;
            // int R = si->x+si->w;
            CellListIter ci = si->cells.begin();
            CellListIter ce = si->cells.end();
            for (; ci != ce; ++ci) {
                CellListIter cR = ci;
                CellListIter cL = cR++;
                Cell* cellL = si->cells.getCell(cL);
                if (cellL->lx() < si->boundL()) {
                    cellL->overlap = 1;
                    //  assert(false);
                }
                if (cellL->hx() > si->boundR()) {
                    cellL->overlap = 1;
                    //  assert(false);
                }
                if (cR == ce) {
                    break;
                }
                Cell* cellR = si->cells.getCell(cR);
                if (cellR->hx() > si->boundR()) {
                    cellR->overlap = 1;
                    //  assert(false);
                }
                int space = 0;
                if (spaceAware) {
                    space = Cell::getSpace(cellL, cellR);
                }
                if (cellL->hx() + space > cellR->lx()) {
                    cellL->overlap = 1;
                    cellR->overlap = 1;
                    //  assert(false);
                }
            }
        }
    }
    int nOverlaps = 0;
    for (int i = 0; i < nCells; i++) {
        nOverlaps += cells[i]->overlap;
    }
    return nOverlaps;
}

int DPlacer::getUnplaced() const {
    int numUnplaced = 0;
    for (const auto& cell : cells) {
        if (!cell->placed) {
            numUnplaced++;
        }
    }
    return numUnplaced;
}

unsigned DPlacer::getOverlapSum() const {
    unsigned sumOverlap = 0;
    for (const Cell* cell : cells) {
        if (cell->placed) {
            if (cellTypeMaps[cell->type].score(cell->ly(), cell->lx())) {
                ++sumOverlap;
            }
        }
    }
    return sumOverlap;
}

int DPlacer::getPinAccess() const {
    int numPinAccess = 0;
    //  cell bounds
    int clx = INT_MAX;
    int cly = INT_MAX;
    int chx = INT_MIN;
    int chy = INT_MIN;
    //  relative pin bounds
    int lx = INT_MAX;
    int ly = INT_MAX;
    int hx = INT_MIN;
    int hy = INT_MIN;
    //  absulte pin bounds
    int plx = INT_MAX;
    int ply = INT_MAX;
    int phx = INT_MIN;
    int phy = INT_MIN;
    int rIndex = -1;
    //  absulte shape bounds
    int slx = INT_MAX;
    int sly = INT_MAX;
    int shx = INT_MIN;
    int shy = INT_MIN;
    int sRIndex = -1;
    for (const Cell* cell : cells) {
        if (cell->placed) {
            clx = cell->lx() * siteW + dbLX;
            cly = cell->ly() * siteH + dbLY;
            chx = cell->hx() * siteW + dbLX;
            chy = cell->hy() * siteH + dbLY;
            for (const db::PinType* pin : cell->getDBCellType()->pins) {
                if (pin->type() == 'p' || pin->type() == 'g') {
                    continue;
                }
                pin->getBounds(lx, ly, hx, hy);
                if (cell->fx()) {
                    plx = chx - hx;
                    phx = chx - lx;
                } else {
                    plx = clx + lx;
                    phx = clx + hx;
                }
                if (cell->fy()) {
                    ply = chy - hy;
                    phy = chy - ly;
                } else {
                    ply = cly + ly;
                    phy = cly + hy;
                }
                rIndex = pin->shapes.front().layer.rIndex;
                for (const db::SNet* snet : database.snets) {
                    for (const db::Geometry& shape : snet->shapes) {
                        if (shape.layer.rIndex < rIndex || shape.layer.rIndex > rIndex + 1) {
                            continue;
                        }

                        if (shape.lx <= phx && shape.ly <= phy && shape.hx >= plx && shape.hy >= ply) {
                            ++numPinAccess;
                            break;
                        }
                    }
                }
                for (db::IOPin* iopin : database.iopins) {
                    iopin->getBounds(slx, sly, shx, shy, sRIndex);
                    if (sRIndex != rIndex && sRIndex != rIndex + 1) {
                        continue;
                    }
                    if (slx <= phx && sly <= phy && shx >= plx && shy >= ply) {
                        ++numPinAccess;
                        break;
                    }
                }
            }
        }
    }
    return numPinAccess;
}

void DPlacer::getDBDisplacement(long long& sum, vector<double>& mean, unsigned long long& max, const Cell*& maxCell) {
    sum = 0;
    max = 0;
    unsigned long long disp = 0;
    unsigned maxH = 1;
    for (int i = counts.size() - 1; i != -1; --i) {
        if (counts[i]) {
            maxH = i + 1;
            break;
        }
    }
    vector<unsigned long long> dist(maxH, 0);
    for (const Cell* cell : cells) {
        disp = lround(abs(cell->lx() - cell->ox()) * siteW + abs(cell->ly() - cell->oy()) * siteH);
        assert(cell->h <= maxH);
        dist[cell->h - 1] += disp;
        if (disp > max) {
            max = disp;
            maxCell = cell;
        }
    }
    for (unsigned i = 0; i != maxH; ++i) {
        const double count = counts[i];
        if (count > 0) {
            sum += dist[i];
            mean[0] += dist[i] / count / maxH;
            mean[i + 1] += dist[i] / count;
        }
    }
}

long long DPlacer::hpwl() {
    long long hpwl = 0;
    int nNets = nets.size();
    for (int i = 0; i < nNets; i++) {
        nets[i]->update();
        hpwl += nets[i]->hpwl();
    }
    return hpwl;
}

void DPStage::update(int iter) {
    assert(dp::DPModule::dplacer);
    if (iter < 0) {
        iter = ++_runIter;
    } else {
        _runIter = std::max(iter, _runIter);
    }
    _iterHPWL[iter] = DPModule::dplacer->hpwl();
    DPModule::dplacer->getDBDisplacement(_iterDBDisp[iter], _iterDBMean[iter], _iterDBMax[iter], _iterDBMaxCell[iter]);
    _overlaps[iter] = DPModule::dplacer->checkOverlap(true);
    _unplaced[iter] = DPModule::dplacer->getUnplaced();
    _overlapSum[iter] = DPModule::dplacer->getOverlapSum();
    if (DPModule::EnablePinAccRpt) {
        _pinAccess[iter] = DPModule::dplacer->getPinAccess();
    }
}

void DPFlow::getLastIter(int& lastStage, int& lastIter, int stage, int iter) {
    stage = (stage < 0 ? _stage : stage);
    iter = (iter < 0 ? _iter : iter);
    if (stage == 0 && iter == 0) {
        lastStage = stage;
        lastIter = iter;
        return;
    }
    if (iter == 0) {
        lastStage = stage - 1;
        lastIter = _stages[lastStage].runIter();
        return;
    }
    lastStage = stage;
    lastIter = iter - 1;
}
double DPFlow::hpwlDiff(int cmpStage, int cmpIter, int stage, int iter) {
    int nowStage = (stage < 0 ? _stage : stage);
    int nowIter = (iter < 0 ? _iter : iter);
    if (cmpStage == -1 || cmpIter == -1) {
        getLastIter(cmpStage, cmpIter, nowStage, nowIter);
    }
    long long hpwl0 = _stages[cmpStage].hpwl(cmpIter);
    long long hpwl1 = _stages[nowStage].hpwl(nowIter);
    return (double)(hpwl1 - hpwl0) / hpwl0;
}
double DPFlow::dispDiff(int cmpStage, int cmpIter, int stage, int iter) {
    int nowStage = (stage < 0 ? _stage : stage);
    int nowIter = (iter < 0 ? _iter : iter);
    if (cmpStage == -1 || cmpIter == -1) {
        getLastIter(cmpStage, cmpIter, nowStage, nowIter);
    }
    long long disp0 = _stages[cmpStage].dbDisp(cmpIter);
    long long disp1 = _stages[nowStage].dbDisp(nowIter);
    if (disp0 == 0) {
        return 0.0;
    }
    return (double)(disp1 - disp0) / disp0;
}
void DPFlow::update(int stage, int iter) {
    if (stage < 0) {
        stage = _stage;
    }
    if (iter < 0) {
        iter = _iter;
    }
    assert(stage < (int)_stages.size() && iter < (int)_stages[stage].maxIter());
    _stages[stage].update(iter);
}

void DPFlow::report(int stage, int iter) {
    if (stage < 0) {
        stage = _stage;
    }
    if (iter < 0) {
        iter = _iter;
    }
    assert(stage < (int)_stages.size() && iter < (int)_stages[stage].maxIter());

    int siteW = DPModule::dplacer->siteW;
    int siteH = DPModule::dplacer->siteH;

    long long hpwl = this->hpwl() / siteW;
    double hpwldiffi = hpwlDiff();
    double hpwldiffg = hpwlDiff(0, 0);
    int overlaps = this->overlap();
    int unplaced = this->unplaced();

#ifndef NDEBUG
    vector<unsigned> evalPin;
    const Cell* maxCell = this->dbMaxCell();
    if (!unplaced) {
        DPModule::dplacer->evalPin(evalPin);
        cout << _stages[stage].name() << '\t';
        for (unsigned i = 0; i != evalPin.size(); ++i) {
            cout << "O" << (i + 3) / 2 << i / 2 + 2 << " = " << evalPin[i] << '\t';
        }
        if (maxCell) {
            cout << maxCell->getDBCell()->name() << endl;
        } else {
            cout << endl;
        }
    }
#endif

    if (DPModule::EnablePinAccRpt) {
        printlog(LOG_INFO,
                 "[%s][%d] dbDisp = %lld dbMean = %.3lf dbMean1 = %.3lf dbMean2 = %.3lf dbMean3 = %.3lf dbMean4 = "
                 "%.3lf dbMax = %.1lf HPWL = %ld (iter=%.2lf%%) (total=%.2lf%%) overlap = %d unplaced = %d overlapSum "
                 "= %d pinAcc = %d",
                 _stages[stage].name().c_str(),
                 iter,
                 dbDisp() / siteW,
                 dbMean()[0] / siteH,
                 dbMean()[1] / siteH,
                 dbMean()[2] / siteH,
                 dbMean()[3] / siteH,
                 dbMean()[4] / siteH,
                 dbMax() / static_cast<double>(siteH),
                 hpwl,
                 hpwldiffi * 100.0,
                 hpwldiffg * 100.0,
                 overlaps,
                 unplaced,
                 overlapSum(),
                 pinAccess());
    } else {
        printlog(
            LOG_INFO,
            "[%s][%d] dbDisp = %lld dbMean = %.3lf dbMean1 = %.3lf dbMean2 = %.3lf dbMean3 = %.3lf dbMean4 = %.3lf "
            "dbMax = %.1lf HPWL = %ld (iter=%.2lf%%) (total=%.2lf%%) overlap = %d unplaced = %d overlapSum = %d",
            _stages[stage].name().c_str(),
            iter,
            dbDisp() / siteW,
            dbMean()[0] / siteH,
            dbMean()[1] / siteH,
            dbMean()[2] / siteH,
            dbMean()[3] / siteH,
            dbMean()[4] / siteH,
            dbMax() / static_cast<double>(siteH),
            hpwl,
            hpwldiffi * 100.0,
            hpwldiffg * 100.0,
            overlaps,
            unplaced,
            overlapSum());
    }
}
