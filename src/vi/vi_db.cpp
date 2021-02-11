#include "../db/db.h"
using namespace db;

#include "vi.h"
using namespace vi;

#include "../ut/utils.h"

void Geometry::draw(Visualizer* v, const Layer& L) const {
    if ((L.rIndex >= 0 || L.cIndex >= 0) && layer != L) {
        return;
    }

    if (layer.rIndex >= 0) {
        v->setFillColor(v->scheme.metalFill[layer.rIndex]);
        v->setLineColor(v->scheme.metalLine[layer.rIndex]);
    } else if (layer.cIndex >= 0) {
        v->setFillColor(v->scheme.metalFill[layer.cIndex]);
        v->setLineColor(v->scheme.metalLine[layer.cIndex]);
    }
    const int lx = globalX();
    const int ly = globalY();
    const int hx = globalX() + boundR();
    const int hy = globalY() + boundT();
    v->drawRect(lx, ly, hx, hy, true, true);
}

void Cell::draw(Visualizer* v) const {
    if (!(region->id)) {
        v->setFillColor(v->scheme.instanceFill);
        v->setLineColor(v->scheme.instanceLine);
    } else {
        int r = (region->id * 100) % 256;
        int g = (region->id * 200) % 256;
        v->setFillColor(Color(r, g, 255));
        v->setLineColor(Color(r / 2, g / 2, 128));
    }
    int lx = globalX();
    int ly = globalY();
    int hx = globalX() + boundR();
    int hy = globalY() + boundT();
    // printlog(LOG_INFO, "cell : %d %d %d %d", lx, ly, hx, hy);
    v->drawRect(lx, ly, hx, hy, true, true);
}

void IOPin::draw(Visualizer* v, Layer& L) const { type->shapes[0].draw(v, L); }

void Pin::draw(Visualizer* v, const Layer& L) const {
    for (const Geometry& shape : type->shapes) {
        shape.draw(v, L);
    }
}

void Net::draw(Visualizer* v, const Layer& L) const {
    for (const Pin* pin : pins) {
        pin->draw(v, L);
    }
}

void SNet::draw(Visualizer* v, Layer& L) const {
    for (const Geometry& shape : shapes) {
        shape.draw(v, L);
    }
    for (const Via& via : vias) {
        via.draw(v, L);
    }
}

void Row::draw(Visualizer* v) const {
    v->setFillColor(v->scheme.rowFill);
    v->setLineColor(v->scheme.rowLine);
    v->drawRect(_x, _y, _x + _xStep * _xNum, _y + _yStep * _yNum, true, true);
}

void Via::draw(Visualizer* v, Layer& L) const {
    for (const Geometry& rect : type->rects) {
        rect.draw(v);
    }
}

void Region::draw(Visualizer* v) const {
    int nRects = rects.size();
    for (int i = 0; i < nRects; i++) {
        v->setFillColor(v->scheme.regionFill);
        v->setLineColor(Color(0, 0, 0));
        v->drawRect(rects[i].lx, rects[i].ly, rects[i].hx, rects[i].hy, true, true);
    }
}

void SiteMap::draw(Visualizer* v) const {
    int lx, ly, hx, hy;
    for (int x = 0; x < siteNX; x++) {
        for (int y = 0; y < siteNY; y++) {
            unsigned char regionID = getRegion(x, y);
            bool blocked = getSiteMap(x, y, SiteMap::SiteBlocked);
            if (blocked) {
                v->setFillColor(Color(0, 0, 0));
                getSiteBound(x, y, lx, ly, hx, hy);
                v->drawRect(lx, ly, hx, hy, true, false);
            } else if (regionID == Region::InvalidRegion) {
                v->setFillColor(Color(255, 0, 0));
                getSiteBound(x, y, lx, ly, hx, hy);
                v->drawRect(lx, ly, hx, hy, true, false);
            } else if (regionID > 0) {
                v->setFillColor(v->scheme.regionFill);
                getSiteBound(x, y, lx, ly, hx, hy);
                v->drawRect(lx, ly, hx, hy, true, false);
            }
        }
    }
}

void Database::draw(Visualizer* v) const {
    v->reset(v->scheme.background);
    int nRows = rows.size();
    for (int i = 0; i < nRows; i++) {
        rows[i]->draw(v);
    }
    int nCells = cells.size();
    for (int i = 0; i < nCells; i++) {
        cells[i]->draw(v);
    }

    /*
    int nRegions = regions.size();
    for(int i=1; i<nRegions; i++){
        regions[i]->draw(v);
    }
    */

    // siteMap.draw(v);

    /* draw metal objects */
    unsigned nLayers = layers.size();
    for (unsigned i = 0; i != nLayers; ++i) {
        const Layer* layer = nullptr;
        if (i % 2) {
            layer = database.getCLayer(i / 2);
        } else {
            layer = database.getRLayer(i / 2);
        }
        if (!layer) {
            continue;
        }
        // printlog(LOG_INFO, "draw layer %s", layer->name.c_str());

        for (const Net* net : nets) {
            net->draw(v, *layer);
        }
    }
    /*
    v->setFillColor(Color(255,0,0));
    for(int x=0; x<grGrid.trackNX; x++){
        for(int y=0; y<grGrid.trackNY; y++){
            int dx = grGrid.trackL + x * grGrid.trackStepX;
            int dy = grGrid.trackB + y * grGrid.trackStepY;
            if(grGrid.getRGrid(x, y, 2) == (char)1){
                v->drawPoint(dx, dy, 1);
            }
        }
    }
    */
}
