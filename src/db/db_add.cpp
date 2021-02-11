#include "db.h"
using namespace db;

#include "../ut/utils.h"

Layer& Database::addLayer(const string& name, const char type) {
    layers.emplace_back(name, type);
    Layer& newlayer = layers.back();
    if (layers.size() == 1) {
        if (type == 'r') {
            newlayer.rIndex = 0;
        }
    } else {
        Layer& oldlayer = layers[layers.size() - 2];
        oldlayer._above = &newlayer;
        newlayer._below = &oldlayer;
        if (type == 'r') {
            newlayer.rIndex = oldlayer.cIndex + 1;
        } else {
            newlayer.cIndex = oldlayer.rIndex;
        }
    }
    return newlayer;
}

ViaType* Database::addViaType(const string& name, bool isDef) {
    ViaType* viatype = getViaType(name);
    if (viatype) {
        printlog(LOG_WARN, "via type re-defined: %s", name.c_str());
        return viatype;
    }
    viatype = new ViaType(name, isDef);
    name_viatypes[name] = viatype;
    viatypes.push_back(viatype);
    return viatype;
}

CellType* Database::addCellType(const string& name, unsigned libcell) {
    CellType* celltype = getCellType(name);
    if (celltype) {
        printlog(LOG_WARN, "cell type re-defined: %s", name.c_str());
        return celltype;
    }
    celltype = new CellType(name, libcell);
#ifdef _GNUC_4_8_
    name_celltypes.emplace(name, celltype);
#else
    name_celltypes[name] = celltype;
#endif
    celltypes.push_back(celltype);
    return celltype;
}

Cell* Database::addCell(const string& name, CellType* type) {
    Cell* cell = getCell(name);
    if (cell) {
        printlog(LOG_WARN, "cell re-defined: %s", name.c_str());
        if (!cell->ctype()) {
            cell->ctype(type);
        }
        return cell;
    }
    cell = new Cell(name, type);
#ifdef _GNUC_4_8_
    name_cells.emplace(name, cell);
#else
    name_cells[name] = cell;
#endif
    cells.push_back(cell);
    return cell;
}

IOPin* Database::addIOPin(const string& name, const string& netName, const char direction) {
    IOPin* iopin = getIOPin(name);
    if (iopin) {
        printlog(LOG_WARN, "IO pin re-defined: %s", name.c_str());
        return iopin;
    }
    iopin = new IOPin(name, netName, direction);
    name_iopins[name] = iopin;
    iopins.push_back(iopin);
    return iopin;
}

Net* Database::addNet(const string& name, const NDR* ndr) {
    Net* net = getNet(name);
    if (net) {
        printlog(LOG_WARN, "Net re-defined: %s", name.c_str());
        return net;
    }
    net = new Net(name, ndr);
    name_nets[name] = net;
    nets.push_back(net);
    return net;
}

Row* Database::addRow(const string& name,
                      const string& macro,
                      const int x,
                      const int y,
                      const unsigned xNum,
                      const unsigned yNum,
                      const bool flip,
                      const unsigned xStep,
                      const unsigned yStep) {
    Row* newrow = new Row(name, macro, x, y, xNum, yNum, flip, xStep, yStep);
    rows.push_back(newrow);
    return newrow;
}

Track* Database::addTrack(char direction, double start, double num, double step) {
    Track* newtrack = new Track(direction, start, num, step);
    tracks.push_back(newtrack);
    return newtrack;
}

Region* Database::addRegion(const string& name, const char type) {
    Region* region = getRegion(name);
    if (region) {
        printlog(LOG_WARN, "Region re-defined: %s", name.c_str());
        return region;
    }
    region = new Region(name, type);
    regions.push_back(region);
    return region;
}

NDR* Database::addNDR(const string& name, const bool hardSpacing) {
    NDR* ndr = getNDR(name);
    if (ndr) {
        printlog(LOG_WARN, "NDR re-defined: %s", name.c_str());
        return ndr;
    }
    ndr = new NDR(name, hardSpacing);
    ndrs.emplace(name, ndr);
    return ndr;
}

SNet* Database::addSNet(const string& name) {
    SNet* newsnet = new SNet(name);
    snets.push_back(newsnet);
    return newsnet;
}
