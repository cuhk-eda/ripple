#include "db.h"
using namespace db;

#include "../ut/utils.h"

/* get layer by name */
Layer* Database::getLayer(const string& name) {
    for (Layer& layer : layers) {
        if (layer.name() == name) {
            return &layer;
        }
    }
    return nullptr;
}

/* get routing layer by index : 0=M1 */
Layer* Database::getRLayer(const int index) {
    for (Layer& layer : layers) {
        if (layer.rIndex == index) {
            return &layer;
        }
    }
    return nullptr;
}

/* get cut layer by index : 0=M1/2 */
const Layer* Database::getCLayer(const unsigned index) const {
    for (const Layer& layer : layers) {
        if (layer.cIndex == static_cast<int>(index)) {
            return &layer;
        }
    }
    return nullptr;
}

/* get cell type by name */
CellType* Database::getCellType(const string& name) {
    unordered_map<string, CellType*>::iterator mi = name_celltypes.find(name);
    if (mi == name_celltypes.end()) {
        return nullptr;
    }
    return mi->second;
}

Cell* Database::getCell(const string& name) {
    unordered_map<string, Cell*>::iterator mi = name_cells.find(name);
    if (mi == name_cells.end()) {
        return nullptr;
    }
    return mi->second;
}

Net* Database::getNet(const string& name) {
    unordered_map<string, Net*>::iterator mi = name_nets.find(name);
    if (mi == name_nets.end()) {
        return nullptr;
    }
    return mi->second;
}

Region* Database::getRegion(const string& name) {
    for (Region* region : regions) {
        if (region->name() == name) {
            return region;
        }
    }
    return nullptr;
}

Region* Database::getRegion(const unsigned char id) {
    if (id == Region::InvalidRegion) {
        return nullptr;
    }
    return regions[id];
}

NDR* Database::getNDR(const string& name) const {
    map<string, NDR*>::const_iterator mi = ndrs.find(name);
    if (mi == ndrs.end()) {
        return nullptr;
    }
    return mi->second;
}

IOPin* Database::getIOPin(const string& name) const {
    unordered_map<string, IOPin*>::const_iterator mi = name_iopins.find(name);
    if (mi == name_iopins.end()) {
        return nullptr;
    }
    return mi->second;
}

ViaType* Database::getViaType(const string& name) const {
    unordered_map<string, ViaType*>::const_iterator mi = name_viatypes.find(name);
    if (mi == name_viatypes.end()) {
        return nullptr;
    }
    return mi->second;
}

int Database::getContainedSites(const int lx, const int ly, const int hx, const int hy, int& slx, int& sly, int& shx, int& shy) const {
    slx = binContainedL(lx, coreLX, coreHX, siteW);
    sly = binContainedL(ly, coreLY, coreHY, siteH);
    shx = binContainedR(hx, coreLX, coreHX, siteW);
    shy = binContainedR(hy, coreLY, coreHY, siteH);
    if (slx > shx || sly > shy) {
        return 0;
    }
    return (shx - slx + 1) * (shy - sly + 1);
}

int Database::getOverlappedSites(const int lx, const int ly, const int hx, const int hy, int& slx, int& sly, int& shx, int& shy) const {
    slx = binOverlappedL(lx, coreLX, coreHX, siteW);
    sly = binOverlappedL(ly, coreLY, coreHY, siteH);
    shx = binOverlappedR(hx, coreLX, coreHX, siteW);
    shy = binOverlappedR(hy, coreLY, coreHY, siteH);
    if (slx > shx || sly > shy) {
        return 0;
    }
    return (shx - slx + 1) * (shy - sly + 1);
}

unsigned Database::getNumRLayers() const {
    unsigned numRLayers = 0;
    for (const Layer& layer : layers) {
        if (layer.rIndex != -1) {
            numRLayers++;
        }
    }
    return numRLayers;
}

unsigned Database::getNumCLayers() const {
    unsigned numCLayers = 0;
    for (const Layer& layer : layers) {
        if (layer.cIndex != -1) {
            numCLayers++;
        }
    }
    return numCLayers;
}
