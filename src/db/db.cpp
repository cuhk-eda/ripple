#include "db.h"

#include "../global.h"

using namespace db;

Database database;

/***** Database *****/
Database::Database() {
    clear();
    _buffer = new char[_bufferCapacity];
}

Database::~Database() {
    delete[] _buffer;
    _buffer = nullptr;
    clear();
    // for regions.push_back(new Region("default"));
    CLEAR_POINTER_LIST(regions);
}

void Database::clear() {
    clearDesign();
    clearLibrary();
    clearTechnology();
}

void Database::clearTechnology() {
    CLEAR_POINTER_LIST(viatypes);
    CLEAR_POINTER_MAP(ndrs);
}

void Database::clearDesign() {
    CLEAR_POINTER_LIST(cells);
    CLEAR_POINTER_LIST(iopins);
    CLEAR_POINTER_LIST(nets);
    CLEAR_POINTER_LIST(rows);
    CLEAR_POINTER_LIST(regions);
    CLEAR_POINTER_LIST(snets);
    CLEAR_POINTER_LIST(tracks);

    DBU_Micron = -1.0;
    designName = "";
    _placements.resize(1);
    _activePlacement = 0;
    regions.push_back(new Region("default"));
}

long long Database::getCellArea(Region* region) const {
    long long cellArea = 0;
    for (const Cell* cell : cells) {
        if (region && cell->region != region) {
            continue;
        }
        int w = cell->width() / siteW;
        int h = cell->height() / siteH;
        cellArea += w * h;
    }
    return cellArea;
}

long long Database::getFreeArea(Region* region) const {
    unsigned nRegions = getNumRegions();
    long long freeArea = 0;
    for (unsigned i = 0; i != nRegions; ++i) {
        if (region && region != regions[i]) {
            continue;
        }
        freeArea += siteMap.nRegionSites[i];
    }
    return freeArea;
}

bool Database::placed() {
    for (Cell* cell : cells) {
        if (!cell->placed()) {
            return false;
        }
    }
    return true;
}

bool Database::detailedRouted() {
    for (Net* net : nets) {
        if (!net->detailedRouted()) {
            return false;
        }
    }
    return true;
}

bool Database::globalRouted() {
    for (Net* net : nets) {
        if (!net->globalRouted()) {
            return false;
        }
    }
    return true;
}

void Database::SetupLayers() {
#ifndef NDEBUG
    int maxRLayer = 0;
    int maxCLayer = 0;
    for (const Layer& layer : layers) {
        if (layer.isRouteLayer()) {
            assert(layer.rIndex == maxRLayer++);
        } else if (layer.isCutLayer()) {
            assert(layer.cIndex == maxCLayer++);
        }
    }
#endif
}

void Database::SetupCellLibrary() {
    for (auto celltype : celltypes) {
        if (!celltype->stdcell) {
            continue;
        }
        for (const PinType* pin : celltype->pins) {
            if (pin->type() != 'p' && pin->type() != 'g') {
                continue;
            }
            for (const Geometry& shape : pin->shapes) {
                if (shape.ly < 0 && shape.hy > 0) {
                    celltype->_botPower = pin->type();
                }
                if (shape.ly < celltype->height && shape.hy > celltype->height) {
                    celltype->_topPower = pin->type();
                }
            }
            if (celltype->_botPower != 'x' && celltype->_topPower != 'x') {
                break;
            }
        }
    }
}

/*
site height as the smallest row height
site width as the smallest step x in rows
*/
void Database::SetupFloorplan() {
    coreLX = INT_MAX;
    coreHX = INT_MIN;
    coreLY = INT_MAX;
    coreHY = INT_MIN;
    siteW = INT_MAX;
    siteH = INT_MAX;

    std::sort(rows.begin(), rows.end(), [](const Row* a, const Row* b) {
        return (a->y() == b->y()) ? (a->x() < b->x()) : (a->y() < b->y());
    });

    for (CellType* celltype : celltypes) {
        if (!celltype->stdcell) {
            continue;
        }
        siteH = std::min(siteH, celltype->height);
    }

    for (Row* row : rows) {
        siteW = std::min(siteW, row->xStep());
        coreLX = std::min(row->x(), coreLX);
        coreLY = std::min(row->y(), coreLY);
        coreHX = std::max(row->x() + (int)row->width(), coreHX);
        coreHY = std::max(row->y() + siteH, coreHY);
    }

    for (Site& site : sites) {
        if (site.siteClassName() == "CORE") {
            if (siteW != (unsigned)site.width()) {
                printlog(LOG_WARN,
                         "siteW %d in DEF is inconsistent with siteW %d in LEF.",
                         static_cast<int>(siteW),
                         static_cast<int>(site.width()));
            }
            if (siteH != site.height()) {
                printlog(LOG_WARN,
                         "siteH %d in DEF is inconsistent with siteH %d in LEF.",
                         static_cast<int>(siteH),
                         static_cast<int>(site.height()));
            }
            break;
        }
    }
}

void Database::SetupRegions() {
    // setup default region
    regions[0]->addRect(coreLX, coreLY, coreHX, coreHY);

    if (!DBModule::EnableFence) {
        for (size_t i = 1; i < regions.size(); ++i) {
            delete regions[i];
        }
        regions.resize(1);
    }

    unsigned numRegions = regions.size();
    // setup region members
    // convert rect to horizontal slices
    for (unsigned char i = 0; i != numRegions; ++i) {
        regions[i]->id = i;
        regions[i]->resetRects();
    }
    // associate cells to regions according to cell name
    for (Region* region : regions) {
        //	each member group name
        for (string member : region->members) {
            if (member[member.length() - 1] == '*') {
                // member group name involve wildcard
                member = member.substr(0, member.length() - 1);
                for (Cell* cell : cells) {
                    if (cell->name().substr(0, member.length()) == member) {
                        cell->region = region;
                    }
                }
            } else {
                // member group name is a particular cell name
                Cell* cell = getCell(member);
                if (!cell) {
                    printlog(
                        LOG_ERROR, "cell name (%s) not found for group (%s)", member.c_str(), region->name().c_str());
                }
                cell->region = region;
            }
        }
    }
    // associate remaining cells to default region
    for (Cell* cell : cells) {
        if (!cell->region) {
            cell->region = regions[0];
        }
    }
}

void Database::SetupSiteMap() {
    // set up site map
    nSitesX = (coreHX - coreLX) / siteW;
    nSitesY = (coreHY - coreLY) / siteH;
    siteMap.siteL = coreLX;
    siteMap.siteR = coreHX;
    siteMap.siteB = coreLY;
    siteMap.siteT = coreHY;
    siteMap.siteStepX = siteW;
    siteMap.siteStepY = siteH;
    siteMap.siteNX = nSitesX;
    siteMap.siteNY = nSitesY;
    siteMap.initSiteMap(nSitesX, nSitesY);

    if (!maxDisp) {
        maxDisp = nSitesX;
    }

    // mark site partially overlapped by fence
    int nRegions = regions.size();
    for (int i = 1; i < nRegions; i++) {
        // skipped the default region
        Region* region = regions[i];

        printlog(LOG_VERBOSE, "region : %s", region->name().c_str());
        // partially overlap at left/right
        vector<Rectangle> hSlices = region->rects;
        Rectangle::sliceH(hSlices);
        for (int j = 0; j < (int)hSlices.size(); j++) {
            int lx = hSlices[j].lx;
            int hx = hSlices[j].hx;
            int olx = binOverlappedL(lx, siteMap.siteL, siteMap.siteR, siteMap.siteStepX);
            int ohx = binOverlappedR(hx, siteMap.siteL, siteMap.siteR, siteMap.siteStepX);
            int clx = binContainedL(lx, siteMap.siteL, siteMap.siteR, siteMap.siteStepX);
            int chx = binContainedR(hx, siteMap.siteL, siteMap.siteR, siteMap.siteStepX);
            int sly = binOverlappedL(hSlices[j].ly, siteMap.siteB, siteMap.siteT, siteMap.siteStepY);
            int shy = binOverlappedR(hSlices[j].hy, siteMap.siteB, siteMap.siteT, siteMap.siteStepY);
            if (olx != clx) {
                for (int y = sly; y <= shy; y++) {
                    siteMap.blockRegion(olx, y);
                }
            }
            if (ohx != chx) {
                for (int y = sly; y <= shy; y++) {
                    siteMap.blockRegion(ohx, y);
                }
            }
        }
        //  partially overlap at bottom/top
        vector<Rectangle> vSlices = region->rects;
        Rectangle::sliceV(vSlices);
        for (const Rectangle& vSlice : vSlices) {
            int ly = vSlice.ly;
            int hy = vSlice.hy;
            const unsigned oly = binOverlappedL(ly, siteMap.siteB, siteMap.siteT, siteMap.siteStepY);
            const unsigned ohy = binOverlappedR(hy, siteMap.siteB, siteMap.siteT, siteMap.siteStepY);
            const int cly = binContainedL(ly, siteMap.siteB, siteMap.siteT, siteMap.siteStepY);
            const int chy = binContainedR(hy, siteMap.siteB, siteMap.siteT, siteMap.siteStepY);
            const unsigned slx = binOverlappedL(vSlice.lx, siteMap.siteL, siteMap.siteR, siteMap.siteStepX);
            const unsigned shx = binOverlappedR(vSlice.hx, siteMap.siteL, siteMap.siteR, siteMap.siteStepX);
            if ((int)oly != cly) {
                for (unsigned x = slx; x <= shx; ++x) {
                    siteMap.blockRegion(x, oly);
                }
            }
            if ((int)ohy != chy) {
                for (unsigned x = slx; x <= shx; ++x) {
                    siteMap.blockRegion(x, ohy);
                }
            }
        }

        for (const Rectangle& rect : hSlices) {
            siteMap.setRegion(rect.lx, rect.ly, rect.hx, rect.hy, region->id);
        }
        for (const Rectangle& rect : vSlices) {
            siteMap.setRegion(rect.lx, rect.ly, rect.hx, rect.hy, region->id);
        }
    }

    // mark all sites blocked
    siteMap.setSites(coreLX, coreLY, coreHX, coreHY, SiteMap::SiteBlocked);

    // mark rows as non-blocked
    for (const Row* row : rows) {
        int lx = row->x();
        int ly = row->y();
        int hx = row->x() + row->width();
        int hy = row->y() + siteH;
        siteMap.unsetSites(lx, ly, hx, hy, SiteMap::SiteBlocked);
    }

    // mark blocked sites
    for (const Cell* cell : cells) {
        if (!cell->fixed()) {
            continue;
        }
        int lx = cell->lx();
        int ly = cell->ly();
        int hx = cell->hx();
        int hy = cell->hy();

        siteMap.setSites(lx, ly, hx, hy, SiteMap::SiteBlocked);
        siteMap.blockRegion(lx, ly, hx, hy);
    }

    for (const Rectangle& placeBlockage : placeBlockages) {
        int lx = placeBlockage.lx;
        int ly = placeBlockage.ly;
        int hx = placeBlockage.hx;
        int hy = placeBlockage.hy;
        siteMap.setSites(lx, ly, hx, hy, SiteMap::SiteBlocked);
        siteMap.blockRegion(lx, ly, hx, hy);
    }

    for (const SNet* snet : snets) {
        for (const Geometry& geo : snet->shapes) {
            switch (geo.layer.rIndex) {
                case 1:
                    if (geo.layer.direction == 'v') {
                        siteMap.setSites(geo.lx, geo.ly, geo.hx, geo.hy, SiteMap::SiteM2Blocked);
                    }
                    break;
                case 2:
                    siteMap.setSites(geo.lx, geo.ly, geo.hx, geo.hy, SiteMap::SiteM3Blocked);
                    break;
                default:
                    break;
            }
        }
    }

    for (const db::IOPin* iopin : iopins) {
        for (const Geometry& shape : iopin->type->shapes) {
            switch (shape.layer.rIndex) {
                case 0:
                    break;
                case 1:
                    siteMap.setSites(shape.lx + iopin->x,
                                     shape.ly + iopin->y,
                                     shape.hx + iopin->x,
                                     shape.hy + iopin->y,
                                     SiteMap::SiteM2BlockedIOPin);
                    break;
                case 2:
                    break;
                default:
                    break;
            }
        }
    }

    siteMap.nSites = siteMap.siteNX * siteMap.siteNY;
    siteMap.nPlaceable = 0;
    siteMap.nRegionSites.resize(regions.size(), 0);
    for (int y = 0; y < siteMap.siteNY; y++) {
        for (int x = 0; x < siteMap.siteNX; x++) {
            if (siteMap.getSiteMap(x, y, SiteMap::SiteBlocked) || siteMap.getSiteMap(x, y, SiteMap::SiteM2Blocked) ||
                siteMap.getSiteMap(x, y, SiteMap::SiteM2BlockedIOPin)) {
                continue;
            }
            siteMap.nPlaceable++;
            unsigned char region = siteMap.getRegion(x, y);
            if (region != Region::InvalidRegion) {
                siteMap.nRegionSites[region]++;
            }
        }
    }

    printlog(LOG_VERBOSE, "core area: %ld", siteMap.nSites);
    printlog(LOG_VERBOSE,
             "placeable: %ld (%lf%%)",
             siteMap.nPlaceable,
             (double)siteMap.nPlaceable / (double)siteMap.nSites * 100.0);
    for (int i = 0; i < (int)regions.size(); i++) {
        printlog(LOG_VERBOSE,
                 "region %d : %ld (%lf%%)",
                 i,
                 siteMap.nRegionSites[i],
                 (double)siteMap.nRegionSites[i] / (double)siteMap.nPlaceable);
    }
}

void Database::SetupRows() {
    // verify row flipping conflict
    bool flipCheckPass = true;
    std::vector<char> flip(nSitesY, 0);
    for (Row* row : rows) {
        char isFlip = (row->flip() ? 1 : 2);
        int y = (row->y() - coreLY) / siteH;
        if (flip[y] == 0) {
            flip[y] = isFlip;
        } else if (flip[y] != isFlip) {
            printlog(LOG_ERROR, "row flip conflict %d : %d", y, isFlip);
            flipCheckPass = false;
        }
    }

    if (!flipCheckPass) {
        printlog(LOG_ERROR, "row flip checking fail");
    }

    if (rows.size() != nSitesY) {
        printlog(LOG_ERROR, "resize rows %d->%d", (int)rows.size(), nSitesY);
        for (Row*& row : rows) {
            delete row;
            row = nullptr;
        }
    }
    rows.resize(nSitesY, nullptr);

    // NOTE: currently we only support one step size
    const int stepX = (coreHX - coreLX) / nSitesX;
    // NOTE: currently we only support horizontal row

    for (unsigned y = 0; y != nSitesY; ++y) {
        rows[y] = new Row("core_SITE_ROW_" + to_string(y), "core", coreLX, coreLY + y * siteH);
        rows[y]->xStep(stepX);
        rows[y]->yStep(0);
        rows[y]->xNum(nSitesX);
        rows[y]->yNum(1);
        rows[y]->flip(flip[y] == 1);
    }

    // set row power-rail
    bool topNormal = true;
    bool botNormal = true;
    bool shrNormal = true;
    for (unsigned y = 0; y < nSitesY; y++) {
        Row* row = rows[y];
        int ly = row->y();
        int hy = row->y() + siteH;
        if (!powerNet.getRowPower(ly, hy, row->_topPower, row->_botPower)) {
            if (topNormal && row->topPower() == 'x') {
                if (y + 1 == nSitesY) {
                    printlog(LOG_WARN, "Top power rail of the row at y=%d is not connected to power rail", row->y());
                } else {
                    printlog(LOG_ERROR, "Top power rail of the row at y=%d is not connected to power rail", row->y());
                    topNormal = false;
                }
            }
            if (botNormal && row->botPower() == 'x') {
                if (y) {
                    printlog(
                        LOG_ERROR, "Bottom power rail of the row at y=%d is not connected to power rail", row->y());
                    botNormal = false;
                } else {
                    printlog(LOG_WARN, "Bottom power rail of the row at y=%d is not connected to power rail", row->y());
                }
            }
        }
        if (shrNormal && row->topPower() == row->botPower()) {
            printlog(LOG_ERROR,
                     "Top and Bottom power rail of the row at y=%d share the same power %c",
                     row->y(),
                     row->topPower());
            shrNormal = false;
        }
    }
}

void Database::SetupRowSegments() {
    for (Row* row : rows) {
        int xL = row->getSiteL(database.coreLX, database.siteW);
        int xR = row->getSiteR(database.coreLX, database.siteW);
        int y = row->getSiteB(database.coreLY, database.siteH);
        RowSegment segment;
        bool b1 = true;
        bool b2 = true;
        Region* r1 = NULL;
        Region* r2 = NULL;
        for (int x = xL; x <= xR; x++) {
            if (x == xR) {
                b2 = true;
                r2 = NULL;
            } else {
                b2 = siteMap.getSiteMap(x, y, SiteMap::SiteBlocked);
                if (DBModule::EnablePG) {
                    b2 = b2 || siteMap.getSiteMap(x, y, SiteMap::SiteM2Blocked);
                }
                if (DBModule::EnableIOPin) {
                    b2 = b2 || siteMap.getSiteMap(x, y, SiteMap::SiteM2BlockedIOPin);
                }
                if (DBModule::EnableFence) {
                    r2 = getRegion(siteMap.getRegion(x, y));
                } else {
                    r2 = regions[0];
                }
            }
            if ((b1 || !r1) && !b2) {
                segment.x = coreLX + x * siteW;
                segment.w = siteW;
                segment.region = r2;
            } else if (!b1 && r1 && (b2 || !r2)) {
                row->segments.push_back(segment);
            } else if (!b1 && !b2 && (r1 == r2)) {
                segment.w += siteW;
            } else if (!b1 && !b2 && (r1 != r2)) {
                row->segments.push_back(segment);
                segment.x = coreLX + x * siteW;
                segment.w = siteW;
                segment.region = r2;
            }
            b1 = b2;
            r1 = r2;
        }
    }
}

void Database::SetupTargetDensity() {
    const size_t nRegions = regions.size();
    vector<long long> regionCellArea(nRegions, 0);
    vector<int> regionCellCount(nRegions, 0);
    int nCells = cells.size();
    printlog(LOG_VERBOSE, "site size = %d:%d", siteW, siteH);
    for (int i = 0; i < nCells; i++) {
        if (cells[i]->fixed()) {
            continue;
        }
        int region = cells[i]->region->id;
        int width = cells[i]->width() / siteW;
        int height = cells[i]->height() / siteH;
        regionCellCount[region]++;
        regionCellArea[region] += width * height;
    }

    double defaultTargetDensity = 0.8;
    for (size_t i = 0; i != nRegions; ++i) {
        double regionDensity = (double)regionCellArea[i] / (double)siteMap.nRegionSites[i];
        printlog(LOG_VERBOSE,
                 "region %d : %8ld %8ld (%6d) %1.4lf",
                 i,
                 siteMap.nRegionSites[i],
                 regionCellArea[i],
                 regionCellCount[i],
                 regionDensity);
        if (regionDensity > defaultTargetDensity) {
            defaultTargetDensity = regionDensity * 1.05;
        }
    }
    if (defaultTargetDensity > 0.95) {
        defaultTargetDensity = 0.95;
    }

    printlog(LOG_VERBOSE, "default cell density = %lf", defaultTargetDensity);

    tdBins.setTDBin(defaultTargetDensity);
}

void Database::SetupNets() {
    /*
    for(Net *net : nets){
        std::stable_partition(net->pins.begin(), net->pins.end(), [](const Pin *pin){
            return pin->type->direction == 'o';
        });
    }
    */
}

void Database::SetupGRGrid() {
    unsigned numLayers = getNumRLayers();

    int minTrackH = INT_MAX;
    int minTrackV = INT_MAX;
    int maxTrackH = INT_MIN;
    int maxTrackV = INT_MIN;
    unsigned minPitchH = UINT_MAX;
    unsigned minPitchV = UINT_MAX;
    for (unsigned i = 0; i != numLayers; ++i) {
        Layer* layer = getRLayer(i);
        Track& track = layer->track;
        if (track.direction == 'x') {
            printlog(LOG_WARN, "track information missing for layer: %s", layer->name().c_str());
            continue;
        }

        printlog(LOG_VERBOSE,
                 "%s: start=%d step=%d num=%d dir=%c",
                 layer->name().c_str(),
                 track.start,
                 track.step,
                 track.num,
                 track.direction);

        if (track.direction == 'h') {
            minTrackH = min(minTrackH, track.start);
            maxTrackH = max(maxTrackH, track.start + (int)track.step * ((int)track.num - 1));
            minPitchH = min(minPitchH, track.step);
        } else if (track.direction == 'v') {
            minTrackV = min(minTrackV, track.start);
            maxTrackV = max(maxTrackV, track.start + (int)track.step * ((int)track.num - 1));
            minPitchV = min(minPitchV, track.step);
        }
    }

    // x = vertical, y = horizontal
    const int numTracksX = (maxTrackV - minTrackV) / minPitchV + 1;
    const int numTracksY = (maxTrackH - minTrackH) / minPitchH + 1;

    unsigned numGCellX = grGrid.gcellNX;
    unsigned numGCellY = grGrid.gcellNY;
    int stepGCellX = grGrid.gcellStepX;
    int stepGCellY = grGrid.gcellStepY;
    int gcellTrackX = (numTracksX - 1) / (numGCellX - 1);
    int gcellTrackY = (numTracksY - 1) / (numGCellY - 1);

#ifdef __GP__
    if (!numGCellX) {
        gcellTrackX = grSetting.gcellTrackX;
        numGCellX = (numTracksX - 1) / gcellTrackX + 1;
        stepGCellX = minPitchV * gcellTrackX;
        printlog(LOG_INFO,
                 "Can't detect GcellGrid X Settings in DEF, set gcellTrackX=%d numGCellX=%u step=%d",
                 gcellTrackX,
                 numGCellX,
                 stepGCellX);
    }

    if (!numGCellY) {
        gcellTrackY = grSetting.gcellTrackY;
        numGCellY = (numTracksY - 1) / gcellTrackY + 1;
        stepGCellY = minPitchH * gcellTrackY;
        printlog(LOG_INFO,
                 "Can't detect GcellGrid Y Settings in DEF, set gcellTrackY=%d numGCellY=%u step=%d",
                 gcellTrackY,
                 numGCellY,
                 stepGCellY);
    }
#endif

    printlog(LOG_VERBOSE, "tracks x=%d y=%d", gcellTrackX, gcellTrackY);
    printlog(LOG_VERBOSE, "gcells = %u x %u x %d", numGCellX, numGCellY, numLayers);

    printlog(
        LOG_VERBOSE, "GCell start=%d pitch=%d step=%d", minTrackH - minPitchH / 2, minPitchH, minPitchH * gcellTrackY);
    printlog(
        LOG_VERBOSE, "GCell start=%d pitch=%d step=%d", minTrackV - minPitchV / 2, minPitchV, minPitchV * gcellTrackX);
    printlog(LOG_VERBOSE, "Tracks H min=%d max=%d", minTrackH, maxTrackH);
    printlog(LOG_VERBOSE, "Tracks V min=%d max=%d", minTrackV, maxTrackV);

    grGrid.trackL = minTrackV;
    grGrid.trackR = maxTrackV;
    grGrid.trackB = minTrackH;
    grGrid.trackT = maxTrackH;
    grGrid.trackStepX = minPitchV;
    grGrid.trackStepY = minPitchH;
    grGrid.trackNX = numTracksX;
    grGrid.trackNY = numTracksY;

    grGrid.gcellL = minTrackV - minPitchV / 2;
    grGrid.gcellR = maxTrackV + minPitchV / 2;
    grGrid.gcellB = minTrackH - minPitchH / 2;
    grGrid.gcellT = maxTrackH + minPitchH / 2;
    grGrid.gcellStepX = stepGCellX;
    grGrid.gcellStepY = stepGCellY;
    grGrid.gcellNX = numGCellX;
    grGrid.gcellNY = numGCellY;

    grGrid.initGCells(numGCellX, numGCellY, numLayers);
    grGrid.initRGrids(numTracksX, numTracksY, numLayers);

    printlog(LOG_VERBOSE, "setting tracks begin");
    for (unsigned i = 0; i != numLayers; ++i) {
        Layer* layer = getRLayer(i);
        Track& track = layer->track;
        int start, step, end;
        if (track.direction == 'h') {
            start = (track.start - minTrackH) / minPitchH;
            step = track.step / minPitchH;
            end = start + (track.num - 1) * step;
            for (int x = 0; x < numTracksX; x++) {
                for (int y = start; y <= end; y += step) {
                    grGrid.setRGrid(x, y, i, (char)1);
                }
            }
        }
        if (track.direction == 'v') {
            start = (track.start - minTrackV) / minPitchV;
            step = track.step / minPitchV;
            end = start + (track.num - 1) * step;
            for (int x = start; x <= end; x += step) {
                for (int y = 0; y < numTracksY; y++) {
                    grGrid.setRGrid(x, y, i, (char)1);
                }
            }
        }
        printlog(LOG_VERBOSE, "layer %d done", i);
    }
    printlog(LOG_VERBOSE, "setting tracks done");

    for (const Geometry& blk : routeBlockages) {
        grGrid.addBlockage(blk.lx, blk.ly, blk.hx, blk.hy, blk.layer.rIndex);
    }

    // add pg blockage
    printlog(LOG_VERBOSE, "add PG blockages (%u)", snets.size());
    for (const SNet* snet : snets) {
        for (const Geometry& blk : snet->shapes) {
            if (blk.layer.isRouteLayer()) {
                grGrid.addBlockage(blk.lx, blk.ly, blk.hx, blk.hy, blk.layer.rIndex);
            }
        }
    }
    printlog(LOG_VERBOSE, "add PG blockages done");

    printlog(LOG_VERBOSE, "add macro blockages");
    // add macro blockages
    int nCells = cells.size();
    for (int i = 0; i < nCells; i++) {
        if (!cells[i]->fixed()) {
            continue;
        }
        int cx = cells[i]->lx();
        int cy = cells[i]->ly();
        for (const Geometry& blk : cells[i]->ctype()->obs()) {
            if (blk.layer.isRouteLayer()) {
                int lx = cx + blk.lx;
                int ly = cy + blk.ly;
                int hx = cx + blk.hx;
                int hy = cy + blk.hy;
                grGrid.addBlockage(lx, ly, hx, hy, blk.layer.rIndex);
            }
        }
    }
    printlog(LOG_VERBOSE, "add macro blockages done");

    printlog(LOG_VERBOSE, "add pin blockages");
    // add pin blockage?
    int nIOPins = iopins.size();
    for (int i = 0; i < nIOPins; i++) {
        int nShapes = iopins[i]->type->shapes.size();
        for (int j = 0; j < nShapes; j++) {
            Geometry& blk = iopins[i]->type->shapes[j];
            if (blk.layer.isRouteLayer()) {
                int lx = iopins[i]->x + blk.lx;
                int ly = iopins[i]->y + blk.ly;
                int hx = iopins[i]->x + blk.hx;
                int hy = iopins[i]->y + blk.hy;
                grGrid.addBlockage(lx, ly, hx, hy, blk.layer.rIndex);
            }
        }
    }
    printlog(LOG_VERBOSE, "add pin blockages done");

    printlog(LOG_VERBOSE, "setting GCell supply start");
    vector<char> dir(numLayers);
    for (unsigned z = 0; z != numLayers; ++z) {
        Layer* layer = getRLayer(z);
        dir[z] = layer->track.direction;
    }

    long long loopCount = 0;
    for (unsigned x = 0; x != grGrid.gcellNX; ++x) {
        const int xfm = x * gcellTrackX;
        const int xto = min(xfm + gcellTrackX - 1, numTracksX - 1);
        for (unsigned y = 0; y != grGrid.gcellNY; ++y) {
            const int yfm = y * gcellTrackY;
            const int yto = min(yfm + gcellTrackY - 1, numTracksY - 1);
            for (unsigned z = 0; z != numLayers; ++z) {
                GCell* gcell = grGrid.getGCell(x, y, z);
                if (dir[z] == 'h') {
                    if (xto >= numTracksX - 1) continue;

                    for (int ty = yfm; ty <= yto; ++ty) {
                        if (grGrid.getRGrid(xto, ty, z) == 1 && grGrid.getRGrid(xto + 1, ty, z) == 1) {
                            gcell->supplyX++;
                            loopCount++;
                        }
                    }
                } else {
                    if (yto >= numTracksY - 1) {
                        continue;
                    }
                    for (int tx = xfm; tx <= xto; ++tx) {
                        if (grGrid.getRGrid(tx, yto, z) == 1 && grGrid.getRGrid(tx, yto + 1, z) == 1) {
                            gcell->supplyY++;
                            loopCount++;
                        }
                    }
                }
            }
        }
    }
    printlog(LOG_VERBOSE, "setting GCell supply done (%ld)", loopCount);
}

void Database::setup(bool liteMode) {
    SetupLayers();
    SetupCellLibrary();
    SetupFloorplan();
    SetupRegions();
    SetupSiteMap();
    SetupRows();
    SetupRowSegments();
    if (!liteMode) {
        SetupTargetDensity();
    }
    SetupNets();
    if (!liteMode) {
        SetupGRGrid();
    }
}

long long Database::getHPWL() {
    long long hpwl = 0;
    int nNets = getNumNets();
    for (int i = 0; i < nNets; i++) {
        int nPins = nets[i]->pins.size();
        if (nPins < 2) {
            continue;
        }
        int lx = INT_MAX;
        int ly = INT_MAX;
        int hx = INT_MIN;
        int hy = INT_MIN;
        for (int j = 0; j < nPins; j++) {
            Pin* pin = nets[i]->pins[j];
            int x, y;
            pin->getPinCenter(x, y);
            lx = min(lx, x);
            ly = min(ly, y);
            hx = max(hx, x);
            hy = max(hy, y);
        }
        hpwl += (hx - lx) + (hy - ly);
    }
    return hpwl;
}

void Database::errorCheck(bool autoFix) {
    vector<Row*>::iterator ri = rows.begin();
    vector<Row*>::iterator re = rows.end();
    bool e_row_exceed_die = false;
    bool w_non_uniform_site_width = false;
    bool w_non_horizontal_row = false;
    int sitewidth = -1;
    for (; ri != re; ++ri) {
        Row* row = *ri;
        if (sitewidth < 0) {
            sitewidth = row->xStep();
        } else if ((unsigned)sitewidth != row->xStep()) {
            w_non_uniform_site_width = true;
        }
        if (row->yNum() != 1) {
            w_non_horizontal_row = true;
        }
        if (row->x() < dieLX) {
            if (autoFix) {
                int exceedSites = ceil((dieLX - row->x()) / (double)row->xStep());
                row->shrinkXNum(exceedSites);
                row->shiftX(exceedSites * row->xStep());
            }
            e_row_exceed_die = true;
        }
        if (row->x() + (int)row->width() > dieHX) {
            if (autoFix) {
                row->xNum((dieHX - row->x()) / row->xStep());
            }
            e_row_exceed_die = true;
        }
    }
    if (e_row_exceed_die) {
        databaseIssues.push_back(E_ROW_EXCEED_DIE);
    }
    if (w_non_uniform_site_width) {
        databaseIssues.push_back(W_NON_UNIFORM_SITE_WIDTH);
    }
    if (w_non_horizontal_row) {
        databaseIssues.push_back(W_NON_HORIZONTAL_ROW);
    }

    bool e_no_net_driving_pin = false;
    bool e_multiple_net_driving_pin = false;
    for (int i = 0; i < (int)nets.size(); i++) {
        if (nets[i]->pins[0]->type->direction() != 'o') {
            e_no_net_driving_pin = true;
        }
        for (int j = 1; j < (int)nets[i]->pins.size(); j++) {
            if (nets[i]->pins[j]->type->direction() != 'i') {
                e_multiple_net_driving_pin = true;
            }
        }
    }
    if (e_no_net_driving_pin) {
        databaseIssues.push_back(E_NO_NET_DRIVING_PIN);
    }
    if (e_multiple_net_driving_pin) {
        databaseIssues.push_back(E_MULTIPLE_NET_DRIVING_PIN);
    }

    for (int i = 0; i < (int)databaseIssues.size(); i++) {
        switch (databaseIssues[i]) {
            case E_ROW_EXCEED_DIE:
                printlog(LOG_WARN, "row is placed out of die area");
                break;
            case W_NON_UNIFORM_SITE_WIDTH:
                printlog(LOG_WARN, "non uniform site width detected");
                break;
            case W_NON_HORIZONTAL_ROW:
                printlog(LOG_WARN, "non horizontal row detected");
                break;
            case E_NO_NET_DRIVING_PIN:
                printlog(LOG_WARN, "missing net driving pin");
                break;
            case E_MULTIPLE_NET_DRIVING_PIN:
                printlog(LOG_WARN, "multiple net driving pin");
                break;
            default:
                break;
        }
    }
}

string DBModule::_name = "db";

bool DBModule::EdgeSpacing = true;
bool DBModule::EnableFence = true;
bool DBModule::EnablePG = false;
bool DBModule::EnableIOPin = true;
std::string DBModule::LogFolderName = "ripple_" + utils::Timer::getCurTimeStr();

void DBModule::registerOptions() {
    ripple::Shell::addOption(this, "edgeSpacing", &EdgeSpacing);
    ripple::Shell::addOption(this, "enableFence", &EnableFence);
    ripple::Shell::addOption(this, "enablePG", &EnablePG);
    ripple::Shell::addOption(this, "enableIOPin", &EnableIOPin);
}

void DBModule::showOptions() const {
    printlog(LOG_INFO, "edgeSpacing        : %s", EdgeSpacing ? "true" : "false");
    printlog(LOG_INFO, "enableFence        : %s", EnableFence ? "true" : "false");
    printlog(LOG_INFO, "enablePG           : %s", EnablePG ? "true" : "false");
    printlog(LOG_INFO, "enableIOPin        : %s", EnableIOPin ? "true" : "false");
}

bool DBModule::setup(bool liteMode) {
#ifndef NDEBUG
    ripple::Shell::showOptions(_name);
#endif

    database.setup(liteMode);
    return true;
}
