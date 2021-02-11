#include "dp.h"
#include "dp_data.h"
using namespace dp;

#include "../ut/utils.h"
#include "../vi/vi.h"

#include <experimental/iterator>

namespace vi {
Visualizer vis;
}

using namespace vi;

void LocalRegion::draw(string filename, int targetCell)
{
    printlog(LOG_INFO, "local region: (%d,%d),(%d,%d)", lx, ly, hx, hy);

    int sw = DPModule::dplacer->siteW;
    int sh = DPModule::dplacer->siteH;
    vis.setWindow(500, 0);
    vis.setViewport(lx * sw, ly * sh, hx * sw, hy * sh);
    vis.reset(vi::Color::BLACK);

    vis.setFillColor(vi::Color::VERY_DARK_GRAY);
    vis.setLineColor(vi::Color::DARK_GRAY);
    for (int r = ly; r < hy; r++) {
        for (int s = 0; s < (int)DPModule::dplacer->segments[r].size(); s++) {
            Segment& seg = DPModule::dplacer->segments[r][s];
            if (seg.x + (int)seg.w < lx) {
                continue;
            }
            if (seg.x > hx) {
                continue;
            }
            //printlog(LOG_INFO, "segment: (%d,%d:%d)", seg.x, seg.x+seg.w, seg.y);
            vis.drawRect(
                seg.x * sw,
                seg.y * sh,
                (seg.x + seg.w) * sw,
                (seg.y + 1) * sh,
                true, true);
        }
    }
    for (unsigned s = 0; s != nSegments; ++s) {
        LocalSegment& lSegment = localSegments[s];
        vis.setFillColor(vi::Color::PURE_BLUE);
        vis.setLineColor(vi::Color::DARK_GRAY);
        printlog(LOG_INFO, "local segment : (%d %d %d)", lSegment.x, lSegment.x + lSegment.w, lSegment.y);
        vis.drawRect(
            lSegment.x * sw,
            lSegment.y * sh,
            (lSegment.x + lSegment.w) * sw,
            (lSegment.y + 1) * sh, false, true);
        copy(lSegment.localCells.begin(), lSegment.localCells.begin() + lSegment.nCells, experimental::make_ostream_joiner(cout, ", "));
        cout << endl;
    }
    for (unsigned i = 0; i < nCells; i++) {
        LocalCell& lCell = localCells[i];
        if (lCell.fixed) {
            vis.setFillColor(vi::Color::BLACK);
            vis.setLineColor(vi::Color::WHITE);
            vis.drawRect(
                lCell.x * sw,
                lCell.y * sh,
                (lCell.x + lCell.w) * sw,
                (lCell.y + lCell.h) * sh,
                true, true);
            printlog(LOG_INFO, "fixed cell: (%d,%d),(%d,%d)", lCell.x, lCell.y, lCell.x + lCell.w, lCell.y + lCell.h);
        } else {
            if ((int)i == targetCell) {
                vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_RED);
                vis.setLineColor(vi::Color::MODERATE_RED);
            } else {
                vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_LIME_GREEN);
                vis.setLineColor(vi::Color::MODERATE_LIME_GREEN);
            }
            vis.drawRect(
                lCell.x * sw,
                lCell.y * sh,
                (lCell.x + lCell.w) * sw,
                (lCell.y + lCell.h) * sh,
                ((int)i != targetCell), true);
            vis.drawText(lCell.i,
                lCell.x * sw + lCell.w * sw / 2,
                lCell.y * sh + lCell.h * sh / 2);
            printlog(LOG_INFO, "local cell: (%d,%d),(%d,%d)", lCell.x, lCell.y, lCell.x + lCell.w, lCell.y + lCell.h);
        }
    }

    vis.save(filename);
}

void DPlacer::drawLayout(string filename)
{
    int sw = siteW;
    int sh = siteH;
    vis.setWindow(3000, 0);
    vis.setViewport(coreLX * sw, coreLY * sh, coreHX * sw, coreHY * sh);
    vis.reset(vi::Color::BLACK);
    vis.setFillColor(vi::Color::VERY_DARK_GRAY);
    vis.setLineColor(vi::Color::DARK_GRAY);
    for (int r = 0; r < (int)segments.size(); r++) {
        for (int s = 0; s < (int)segments[r].size(); s++) {
            Segment& segment = segments[r][s];
            vis.drawRect(
                segment.x * sw,
                segment.y * sh,
                (segment.x + segment.w) * sw,
                (segment.y + 1) * sh,
                true, true);
        }
    }
    vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_LIME_GREEN);
    vis.setLineColor(vi::Color::MODERATE_LIME_GREEN);
    for (int i = 0; i < (int)cells.size(); i++) {
        Cell* cell = cells[i];
        if (!cell->placed) {
            vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_RED);
            vis.setLineColor(vi::Color::MODERATE_RED);
            vis.drawRect(cell->tx() * sw, cell->ty() * sh, (cell->tx() + cell->w) * sw, (cell->ty() + cell->h) * sh, false, true);
        } else {
            vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_LIME_GREEN);
            vis.setLineColor(vi::Color::MODERATE_LIME_GREEN);
            vis.drawRect(
                cell->lx() * sw,
                cell->ly() * sh,
                cell->hx() * sw,
                cell->hy() * sh,
                true, true);
        }
    }
    vis.save(filename);
}

#define DRAW_DISP_LIGHT
void DPlacer::drawDisplacement(const string& filename, const Cell* targetCell) {
    vis.setWindow(6000, 0);
    vis.setViewport(coreLX * siteW, coreLY * siteH, coreHX * siteW, coreHY * siteH);
    vis.reset(vi::Color::BLACK);

#ifdef DRAW_DISP_LIGHT
    vis.setFillColor(vi::Color::WHITE);
#else
    vis.setFillColor(vi::Color::VERY_DARK_GRAY);
#endif

    vis.setLineColor(vi::Color::DARK_GRAY);
    for (const vector<Segment>& segment : segments) {
        for (const Segment& seg : segment) {
            vis.drawRect(
                seg.x * siteW,
                seg.y * siteH,
                (seg.x + seg.w) * siteW,
                (seg.y + 1) * siteH,
                true, true);
        }
    }
    for (const Cell* cell : cells) {
        if (!cell->placed) {
            continue;
        }
        if (cell->oxLong() == cell->lx() && cell->oyLong() == cell->ly()) {
#ifdef DRAW_DISP_LIGHT
            vis.setFillColor(vi::Color::VERY_LIGHT_GRAY);
            vis.setLineColor(vi::Color::VERY_DARK_GRAY);
#else
            vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_LIME_GREEN);
            vis.setLineColor(vi::Color::MODERATE_LIME_GREEN);
#endif
        } else if (targetCell && (cell->type != targetCell->type || cell->fid != targetCell->fid)) {
#ifdef DRAW_DISP_LIGHT
            vis.setFillColor(vi::Color::VERY_LIGHT_GRAY);
            vis.setLineColor(vi::Color::VERY_DARK_GRAY);
#else
            vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_LIME_GREEN);
            vis.setLineColor(vi::Color::MODERATE_LIME_GREEN);
#endif
        } else if (cellTypeMaps[cell->type].score(cell->ly(), cell->lx())) {
#ifdef DRAW_DISP_LIGHT
            vis.setFillColor(vi::Color::OMNI_SALMON);
            vis.setLineColor(vi::Color::VERY_DARK_DESATURATED_RED);
#else
            vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_RED);
            vis.setLineColor(vi::Color::MODERATE_RED);
#endif
        } else {
#ifdef DRAW_DISP_LIGHT
            vis.setFillColor(vi::Color::OMNI_SALMON5);
            vis.setLineColor(vi::Color::VERY_DARK_DESATURATED_RED);
#else
            vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_RED);
            vis.setLineColor(vi::Color::MODERATE_RED);
#endif
        }
        vis.drawRect(
            cell->lx() * siteW,
            cell->ly() * siteH,
            cell->hx() * siteW,
            cell->hy() * siteH,
            true, true);
    }

    for (const db::SNet* snet : database.snets) {
        for (const db::Geometry& shape : snet->shapes) {
            switch (shape.layer.rIndex) {
                case 1:
                    vis.setFillColor(vi::Color::OMNI_LIGHT_TEAL5);
                    break;
                case 2:
                    vis.setFillColor(vi::Color::OMNI_DEEP_SKY_BLUE5);
                    break;
                default:
                    continue;
            }
            vis.drawRect(
                    shape.lx - coreLX,
                    shape.ly - coreLY,
                    shape.hx - coreLX,
                    shape.hy - coreLY,
                    true, false);
        }
    }
    for (const db::IOPin* iopin : database.iopins) {
        for (const db::Geometry& shape : iopin->type->shapes) {
            switch (shape.layer.rIndex) {
                case 0:
                    vis.setFillColor(vi::Color::OMNI_FERN5);
                    break;
                case 1:
                    vis.setFillColor(vi::Color::OMNI_LIGHT_TEAL5);
                    break;
                case 2:
                    vis.setFillColor(vi::Color::OMNI_DEEP_SKY_BLUE5);
                    break;
                default:
                    continue;
            }
            vis.drawRect(
                    shape.lx + iopin->x - coreLX,
                    shape.ly + iopin->y - coreLY,
                    shape.hx + iopin->x - coreLX,
                    shape.hy + iopin->y - coreLY,
                    true, false);
        }
    }

    vis.setLineColor(vi::Color::OMNI_LIGHT_CAYENNE);
    for (const Cell* cell : cells) {
        if (!cell->placed) {
            continue;
        }
        if (cell->oxLong() == cell->lx() && cell->oyLong() == cell->ly()) {
            continue;
        }
        if (targetCell && (cell->type != targetCell->type || cell->fid != targetCell->fid)) {
            continue;
        }
        vis.drawLine(
            cell->ox() * siteW + cell->w * siteW / 2,
            cell->oy() * siteH + cell->h * siteH / 2,
            cell->lx() * siteW + cell->w * siteW / 2,
            cell->ly() * siteH + cell->h * siteH / 2);
    }

    vis.save(filename);
}

void DPlacer::drawMove(string filename, Region region, vector<Region>& pos)
{
    int sw = siteW;
    int sh = siteH;
    vis.setWindow(3000, 0);
    vis.setViewport(coreLX * sw, coreLY * sh, coreHX * sw, coreHY * sh);
    vis.reset(vi::Color::BLACK);
    /* draw segments */
    vis.setFillColor(vi::Color::VERY_DARK_GRAY);
    vis.setLineColor(vi::Color::DARK_GRAY);
    for (int r = 0; r < (int)segments.size(); r++) {
        for (int s = 0; s < (int)segments[r].size(); s++) {
            Segment& segment = segments[r][s];
            vis.drawRect(
                segment.x * sw,
                segment.y * sh,
                (segment.x + segment.w) * sw,
                (segment.y + 1) * sh,
                true, true);
        }
    }
    /* draw placed cells */
    for (int i = 0; i < (int)cells.size(); i++) {
        Cell* cell = cells[i];
        if (!cell->placed) {
            continue;
        }
        vis.setFillColor(vi::Color::VERY_DARK_DESATURATED_LIME_GREEN);
        vis.setLineColor(vi::Color::MODERATE_LIME_GREEN);
        vis.drawRect(cell->lx() * sw, cell->ly() * sh,
            cell->hx() * sw, cell->hy() * sh,
            true, true);
    }
    /* draw region */
    vis.setLineColor(vi::Color::PURE_RED);
    vis.drawRect(region.lx * sw, region.ly * sh,
        region.hx * sw, region.hy * sh,
        false, true);
    /* draw target position */
    for (int i = 0; i < (int)pos.size(); i++) {
        vis.setLineColor(vi::Color::PURE_RED);
        vis.drawRect(pos[i].lx * sw, pos[i].ly * sh,
            pos[i].hx * sw, pos[i].hy * sh,
            false, true);
    }
    vis.save(filename);
}

void Density::draw(string filename)
{
    int sw = DPModule::dplacer->siteW;
    int sh = DPModule::dplacer->siteH;
    vis.setWindow(3000, 0);
    vis.setViewport(lx * sw, ly * sh, hx * sw, hy * sh);
    vis.reset(vi::Color::BLACK);

    for (int x = 0; x < nBinsX; x++) {
        for (int y = 0; y < nBinsY; y++) {
            int lx = getBinBoundL(x) * sw;
            int ly = getBinBoundB(y) * sh;
            int hx = getBinBoundR(x) * sw;
            int hy = getBinBoundT(y) * sh;
            double density = getBinDensity(x, y);
            if (density > 0.98) {
                vis.setFillColor(vi::Color::PURE_RED);
            } else {
                int color = min(255, (int)(density * 255));
                vis.setFillColor(vi::Color(color, color, color));
            }
            vis.drawRect(lx, ly, hx, hy, true, false);
        }
    }
    vis.save(filename);
}
