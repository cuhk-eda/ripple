#include "dp_data.h"
#include "dp_global.h"

using namespace dp;

#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <lemon/network_simplex.h>
using namespace lemon;

using Graph = ListDigraph;

unsigned DPlacer::minCostFlow() {
    typedef double Value;
    typedef int Cost;
    typedef NetworkSimplex<Graph, Value, Cost> MCF;
    const Value INFTY =
        numeric_limits<Value>::has_infinity ? numeric_limits<Value>::infinity() : numeric_limits<Value>::max();

    Graph g;
    Graph::ArcMap<Value> capacity(g);
    Graph::ArcMap<Cost> cost(g);
    Graph::NodeMap<Value> supply(g);
    vector<Graph::Node> nodes;
    vector<Graph::Arc> arcs;

    unordered_map<Cell*, unsigned> cellID;

    Graph::Node nz = g.addNode();
    Graph::Node nn;
    Graph::Node np;
    supply[nz] = 0;

    if (!DPModule::MLLTotalDisp) {
        nn = g.addNode();
        np = g.addNode();
        supply[nn] = 0;  //  1;
        supply[np] = 0;  //  -1;
    }

    unsigned nArcs = 0;

    unsigned nCells = cells.size();
    unsigned maxDy = 0;
    for (unsigned i = 0; i != nCells; ++i) {
        Cell* cell = cells[i];
        cellID[cell] = i;
        double weight = 1;
        if (!DPModule::MLLTotalDisp) {
            weight /= (double)counts[cell->h - 1];
        }
        //  x_i
        nodes.push_back(g.addNode());
        supply[nodes[i]] = 0;

        //  x_i^- - x_i   <= -x_i'
        arcs.push_back(g.addArc(nz, nodes[i]));
        capacity[arcs[nArcs]] = weight;
        cost[arcs[nArcs++]] = -cell->oxLong();
        //  x_i   - x_i^+ <=  x_i'
        arcs.push_back(g.addArc(nodes[i], nz));
        capacity[arcs[nArcs]] = weight;
        cost[arcs[nArcs++]] = cell->oxLong();

        if (!DPModule::MLLTotalDisp) {
            const long dy = lround(abs(cell->ly() - cell->oy()) * siteH / siteW);
            maxDy = max<long>(maxDy, dy);

            //  x_0^- - x_i   <= -x_i'
            arcs.push_back(g.addArc(nn, nodes[i]));
            capacity[arcs[nArcs]] = INFTY;
            cost[arcs[nArcs++]] = -cell->oxLong() - dy;
            //  x_i   - x_0^+ <=  x_i'
            arcs.push_back(g.addArc(nodes[i], np));
            capacity[arcs[nArcs]] = INFTY;
            cost[arcs[nArcs++]] = cell->oxLong() - dy;

            const pair<unsigned, unsigned>* seg = cellTypeMaps[cell->type].getSegmentAt(cell->ly(), cell->lx());
            if (seg) {
                //  x_0^- - x_i   <= -l_i
                arcs.push_back(g.addArc(nz, nodes[i]));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = -seg->first;
                //  x_i   - x_0^+ <=  r_i
                arcs.push_back(g.addArc(nodes[i], nz));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = seg->second - 1;
            } else {
                //  x_0^- - x_i   <= -l_i
                arcs.push_back(g.addArc(nz, nodes[i]));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = -cell->lx();
                //  x_i   - x_0^+ <=  r_i
                arcs.push_back(g.addArc(nodes[i], nz));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = cell->lx();
            }
        }
    }

    if (!DPModule::MLLTotalDisp) {
        //   x_0^- <= 0
        arcs.push_back(g.addArc(nz, nn));
        capacity[arcs[nArcs]] = 0.04;
        cost[arcs[nArcs++]] = maxDy;
        //  -x_0^+ <=  0
        arcs.push_back(g.addArc(np, nz));
        capacity[arcs[nArcs]] = 0.04;
        cost[arcs[nArcs++]] = maxDy;
    }

    for (const vector<Segment>& segs : segments) {
        for (const Segment& seg : segs) {
            unsigned nSegCells = seg.cells.size();
            if (nSegCells) {
                Cell* cellR = seg.cells.get(0);
                Cell* cellL = seg.cells.get(nSegCells - 1);
                int iR = cellID[cellR];
                int iL = cellID[cellL];
                int L = seg.boundL();
                int R = seg.boundR() - cellL->w;
                //  x_0^- - x_i   <= -l_i
                arcs.push_back(g.addArc(nz, nodes[iR]));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = -L;
                //  x_i   - x_0^+ <=  r_i
                arcs.push_back(g.addArc(nodes[iL], nz));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = R;
                for (int i = 0; i < (int)nSegCells - 1; i++) {
                    cellL = seg.cells.get(i);
                    cellR = seg.cells.get(i + 1);
                    iL = cellID[cellL];
                    iR = cellID[cellR];
                    arcs.push_back(g.addArc(nodes[iL], nodes[iR]));
                    capacity[arcs[nArcs]] = INFTY;
                    cost[arcs[nArcs++]] = -cellL->w - Cell::getSpace(cellL, cellR);
                }
            }
        }
    }

    MCF mcf(g);
    mcf.upperMap(capacity).costMap(cost).supplyMap(supply);
    MCF::ProblemType ret = mcf.run(MCF::FIRST_ELIGIBLE);
    Value maxFlow = 0;
    Value minFlow = INFTY;
    Cost z = 0;
    Move m;
    switch (ret) {
        case MCF::INFEASIBLE:
            printlog(LOG_ERROR, "Infeasible");
            return 0;
        case MCF::OPTIMAL:
            for (Graph::ArcIt a(g); a != INVALID; ++a) {
                maxFlow = max(maxFlow, mcf.flow(a));
                minFlow = min(minFlow, mcf.flow(a));
            }
            z = mcf.potential(nz);
#ifndef NDEBUG
            printlog(LOG_INFO,
                     "nCells: %d\tTotal flow cost: %f\tMin flow: %f\tMax flow: %f",
                     nCells,
                     (double)mcf.totalCost(),
                     minFlow,
                     maxFlow);
            if (!DPModule::MLLTotalDisp) {
                printlog(LOG_INFO, "nn: %d\tnp: %d", z - mcf.potential(nn), z - mcf.potential(np));
            }
#endif
            for (unsigned i = 0; i != nCells; ++i) {
                m.legals.emplace_back(cells[i], z - mcf.potential(nodes[i]), cells[i]->ly());
            }
            doLegalMove(m, false);
            return 1;
        case MCF::UNBOUNDED:
            printlog(LOG_ERROR, "Unbounded");
            return 0;
        default:
            printlog(LOG_ERROR, "Unrecognized problem type.");
            return 0;
    }
}

void DPlacer::localMinCostFlow(LocalRegion& localRegion) {
    typedef double Value;
    typedef int Cost;
    typedef NetworkSimplex<Graph, Value, Cost> MCF;
    const Value INFTY =
        numeric_limits<Value>::has_infinity ? numeric_limits<Value>::infinity() : numeric_limits<Value>::max();

    Graph g;
    Graph::ArcMap<Value> capacity(g);
    Graph::ArcMap<Cost> cost(g);
    Graph::NodeMap<Value> supply(g);
    vector<Graph::Node> nodes;
    vector<Graph::Arc> arcs;

    unordered_map<LocalCell*, int> cellID;

    Graph::Node nz = g.addNode();
    supply[nz] = 0;

    unsigned nCells = localRegion.nCells;
    unsigned nArcs = 0;
    for (unsigned i = 0; i != nCells; ++i) {
        LocalCell* cell = &localRegion.localCells[i];
        cellID[cell] = i;
        nodes.push_back(g.addNode());
        supply[nodes[i]] = 0;
        //  x_i^- - x_i   <= -x_i'
        arcs.push_back(g.addArc(nz, nodes[i]));
        capacity[arcs[nArcs]] = 1;
        if (cell->parent) {
            cost[arcs[nArcs++]] = -cell->parent->oxLong();
        } else {
            cost[arcs[nArcs++]] = -cell->oxLong();
        }
        //  x_i   - x_i^+ <=  x_i'
        arcs.push_back(g.addArc(nodes[i], nz));
        capacity[arcs[nArcs]] = 1;
        if (cell->parent) {
            cost[arcs[nArcs++]] = cell->parent->oxLong();
        } else {
            cost[arcs[nArcs++]] = cell->oxLong();
        }
        if (cell->fixed) {
            //  x_0^- - x_i   <= -l_i
            arcs.push_back(g.addArc(nz, nodes[i]));
            capacity[arcs[nArcs]] = INFTY;
            cost[arcs[nArcs++]] = -cell->oxLong();
            //  x_i   - x_0^+ <=  r_i
            arcs.push_back(g.addArc(nodes[i], nz));
            capacity[arcs[nArcs]] = INFTY;
            cost[arcs[nArcs++]] = cell->oxLong();
        }
    }

    for (unsigned i = 0; i != localRegion.nSegments; ++i) {
        LocalSegment seg = localRegion.localSegments[i];
        int nSegCells = seg.nCells;
        if (nSegCells) {
            LocalCell* cellR = &localRegion.localCells[seg.localCells[0]];
            LocalCell* cellL = &localRegion.localCells[seg.localCells[nSegCells - 1]];
            int iR = cellID[cellR];
            int iL = cellID[cellL];
            int L = seg.x;
            int R = seg.x + seg.w - cellL->w;
            //  x_0^- - x_i   <= -l_i
            arcs.push_back(g.addArc(nz, nodes[iR]));
            capacity[arcs[nArcs]] = INFTY;
            cost[arcs[nArcs++]] = -L;
            //  x_i   - x_0^+ <=  r_i
            arcs.push_back(g.addArc(nodes[iL], nz));
            capacity[arcs[nArcs]] = INFTY;
            cost[arcs[nArcs++]] = R;
            for (int j = 0; j < nSegCells - 1; ++j) {
                cellL = &localRegion.localCells[seg.localCells[j]];
                cellR = &localRegion.localCells[seg.localCells[j + 1]];
                iL = cellID[cellL];
                iR = cellID[cellR];
                arcs.push_back(g.addArc(nodes[iL], nodes[iR]));
                capacity[arcs[nArcs]] = INFTY;
                cost[arcs[nArcs++]] = -cellL->w;
            }
        }
    }

    MCF mcf(g);
    mcf.upperMap(capacity).costMap(cost).supplyMap(supply);
    MCF::ProblemType ret = mcf.run(MCF::FIRST_ELIGIBLE);

    switch (ret) {
        case MCF::INFEASIBLE:
            printlog(LOG_ERROR, "Infeasible");
            break;
        case MCF::OPTIMAL:
            break;
        case MCF::UNBOUNDED:
            printlog(LOG_ERROR, "Unbounded");
            break;
        default:
            break;
    }

    for (unsigned i = 0; i != nCells; i++) {
        LocalCell& cell = localRegion.localCells[i];
        cell.x = -mcf.potential(nodes[i]);
        cell.ox(cell.x);
    }
}

unsigned DPlacer::chainMoveCell(const unsigned fid, const unsigned type) {
    typedef int Value;
    typedef double Cost;
    typedef NetworkSimplex<Graph, Value, Cost> MCF;

    Graph g;
    Graph::ArcMap<Value> capacity(g);
    Graph::ArcMap<Cost> cost(g);
    Graph::NodeMap<Value> supply(g);
    vector<Graph::Node> nodes;
    vector<Graph::Arc> arcs;
    unordered_map<unsigned long long, Graph::Arc> arcMap;

    unsigned nNodes = 0;

    vector<Cell*> localCells;

    Graph::Node nn = g.addNode();
    Graph::Node np = g.addNode();

    Cost oDisp = 0;

    for (Cell* cell : cells) {
        if (cell->fid != (int)fid || cell->type != type) {
            continue;
        }

        localCells.push_back(cell);

        nodes.push_back(g.addNode());
        supply[nodes[2 * nNodes]] = 0;

        nodes.push_back(g.addNode());
        supply[nodes[2 * nNodes + 1]] = 0;

        arcs.push_back(g.addArc(nn, nodes[2 * nNodes]));
        capacity[arcs[2 * nNodes]] = 1;
        cost[arcs[2 * nNodes]] = 0;

        arcs.push_back(g.addArc(nodes[2 * nNodes + 1], np));
        capacity[arcs[2 * nNodes + 1]] = 1;
        cost[arcs[2 * nNodes + 1]] = 0;

        oDisp = max<Cost>(oDisp, abs(cell->lx() - cell->ox()) + abs(cell->ly() - cell->oy()) * siteH / siteW);
        ++nNodes;
    }

    if (nNodes > 7000) {
        //  return 0;
    }

    static Cost pmMaxDisp = max<Cost>(1, flow.stage(1).dbMax(0) / static_cast<Cost>(siteW));
    if (oDisp <= pmMaxDisp) {
        return 0;
    }

    for (unsigned i = 0; i != nNodes; ++i) {
        for (unsigned j = 0; j != nNodes; ++j) {
            Cost disp = abs(localCells[j]->lx() - localCells[i]->ox()) + abs(localCells[j]->ly() - localCells[i]->oy()) * siteH / siteW;
            if (nNodes > 8000) {
                if (i != j && disp > 100) {
                    continue;
                }
                if ((localCells[j]->lx() - localCells[i]->ox()) * (localCells[i]->lx() - localCells[i]->ox()) < 0 || (localCells[j]->ly() - localCells[i]->oy()) * (localCells[i]->ly() - localCells[i]->oy()) < 0) {
                    continue;
                }
            } else {
                if (disp > oDisp) {
                    continue;
                }
            }

            unsigned long long l = ((unsigned long long)i << 32) + j;
            arcMap.emplace(l, g.addArc(nodes[2 * i], nodes[2 * j + 1]));
            capacity[arcMap[l]] = 1;
            if (disp > pmMaxDisp) {
                cost[arcMap[l]] = pow(disp, 5) / pow(pmMaxDisp, 4);
            } else {
                cost[arcMap[l]] = disp;
            }
        }
    }

    supply[nn] = nNodes;
    supply[np] = -nNodes;

    MCF mcf(g);
    mcf.upperMap(capacity).costMap(cost).supplyMap(supply);
    MCF::ProblemType ret = mcf.run(MCF::FIRST_ELIGIBLE);

    unsigned nMoves = 0;
    Move m;
    switch (ret) {
        case MCF::INFEASIBLE:
            printlog(LOG_ERROR, "Infeasible");
            return 0;
        case MCF::OPTIMAL:
            for (Cell* cell : localCells) {
                cell->tx(cell->lx());
                cell->ty(cell->ly());
                removeCell(cell);
                assert(!cell->placed);
            }
            oDisp = 0;
            for (const auto& [hash, arc] : arcMap) {
                if (mcf.flow(arc) != 1) {
                    continue;
                }
                unsigned i = (hash & 0xFFFFFFFF00000000) >> 32;
                unsigned j = hash & 0x00000000FFFFFFFF;
                doLegalMove({localCells[i], localCells[j]->tx(), localCells[j]->ty()}, false);
                oDisp = max<Cost>(oDisp, abs(localCells[i]->lx() - localCells[i]->ox()) + abs(localCells[i]->ly() - localCells[i]->oy()) * siteH / siteW);
                ++nMoves;
            }
            pmMaxDisp = max(pmMaxDisp, oDisp);
            printlog(LOG_INFO,
                     "fid: %u\ttype: %s\tnCells: %u\tTotal flow cost: %lf\tMax Disp: %lf",
                     fid,
                     dbCellTypes[type]->name.c_str(),
                     nNodes,
                     static_cast<double>(mcf.totalCost()),
                     oDisp);
            return nMoves;
        case MCF::UNBOUNDED:
            printlog(LOG_ERROR, "Unbounded");
            return 0;
        default:
            printlog(LOG_ERROR, "Unrecognized problem type.");
            return 0;
    }
}

unsigned DPlacer::localChainMoveCell(Cell* targetCell) {
    typedef int Value;
    typedef double Cost;
    typedef NetworkSimplex<Graph, Value, Cost> MCF;

    Graph g;
    Graph::ArcMap<Value> capacity(g);
    Graph::ArcMap<Cost> cost(g);
    Graph::NodeMap<Value> supply(g);
    vector<Graph::Node> nodes;
    vector<Graph::Arc> arcs;
    unordered_map<unsigned long long, Graph::Arc> arcMap;

    vector<Cell*> localCells;

    Graph::Node nn = g.addNode();
    Graph::Node np = g.addNode();

    Cost oDisp = 0;

    for (Cell* cell : cells) {
        if (cell->fid != targetCell->fid || cell->type != targetCell->type) {
            continue;
        }

        localCells.push_back(cell);
    }

    unsigned nNodes = localCells.size();
    if (nNodes > 2000) {
        const float mx = (targetCell->lx() + targetCell->ox()) / 2;
        const float my = (targetCell->ly() + targetCell->oy()) / 2;
        sort(localCells.begin(), localCells.end(), [&](const Cell* lhs, const Cell* rhs) {
                    return abs(lhs->lx() - mx) * siteW + abs(lhs->ly() - my) * siteH < abs(rhs->lx() - mx) * siteW + abs(rhs->ly() - my) * siteH;
                });
        nNodes = 2000;
        const unsigned radius = abs(localCells[nNodes - 1]->lx() - mx) * siteW + abs(localCells[nNodes - 1]->ly() - my) * siteH;
        for (; nNodes != localCells.size() && abs(localCells[nNodes]->lx() - mx) * siteW + abs(localCells[nNodes]->ly() - my) * siteH == radius; ++nNodes);
        localCells.erase(localCells.begin() + nNodes, localCells.end());
    }


    for (unsigned nodeIdx = 0; nodeIdx != nNodes; ++nodeIdx) {
        nodes.push_back(g.addNode());
        supply[nodes[2 * nodeIdx]] = 0;

        nodes.push_back(g.addNode());
        supply[nodes[2 * nodeIdx + 1]] = 0;

        arcs.push_back(g.addArc(nn, nodes[2 * nodeIdx]));
        capacity[arcs[2 * nodeIdx]] = 1;
        cost[arcs[2 * nodeIdx]] = 0;

        arcs.push_back(g.addArc(nodes[2 * nodeIdx + 1], np));
        capacity[arcs[2 * nodeIdx + 1]] = 1;
        cost[arcs[2 * nodeIdx + 1]] = 0;

        const Cell* cell = localCells[nodeIdx];
        oDisp = max<Cost>(oDisp, abs(cell->lx() - cell->ox()) + abs(cell->ly() - cell->oy()) * siteH / siteW);
    }

    static Cost pmMaxDisp = max<Cost>(10, flow.stage(1).dbMax(0) / static_cast<Cost>(siteW));
    if (oDisp <= pmMaxDisp) {
        return 0;
    }

    for (unsigned i = 0; i != nNodes; ++i) {
        for (unsigned j = 0; j != nNodes; ++j) {
            Cost disp = abs(localCells[j]->lx() - localCells[i]->ox()) + abs(localCells[j]->ly() - localCells[i]->oy()) * siteH / siteW;
            if (disp > oDisp) {
                continue;
            }

            if (nNodes > 1000 &&
                ((localCells[j]->lx() > localCells[i]->ox() + siteH / siteW && localCells[i]->lx() < localCells[i]->ox()) ||
                 (localCells[j]->lx() < localCells[i]->ox() - siteH / siteW && localCells[i]->lx() > localCells[i]->ox()) ||
                 (localCells[j]->ly() > localCells[i]->oy() + 1 && localCells[i]->ly() < localCells[i]->oy()) ||
                 (localCells[j]->ly() < localCells[i]->oy() - 1 && localCells[i]->ly() > localCells[i]->oy()))) {
                continue;
            }

            unsigned long long l = ((unsigned long long)i << 32) + j;
            arcMap.emplace(l, g.addArc(nodes[2 * i], nodes[2 * j + 1]));
            capacity[arcMap[l]] = 1;
            if (disp > pmMaxDisp) {
                cost[arcMap[l]] = pow(disp, 5) / pow(pmMaxDisp, 4);
            } else {
                cost[arcMap[l]] = disp;
            }
        }
    }

    supply[nn] = nNodes;
    supply[np] = -nNodes;

    MCF mcf(g);
    mcf.upperMap(capacity).costMap(cost).supplyMap(supply);
    MCF::ProblemType ret = mcf.run(MCF::FIRST_ELIGIBLE);

    unsigned nMoves = 0;
    Move m;
    Cost mDisp = 0;
    switch (ret) {
        case MCF::INFEASIBLE:
            printlog(LOG_ERROR, "Infeasible");
            return 0;
        case MCF::OPTIMAL:
            for (Cell* cell : localCells) {
                cell->tx(cell->lx());
                cell->ty(cell->ly());
                removeCell(cell);
                assert(!cell->placed);
            }
            mDisp = 0;
            for (const auto& [hash, arc] : arcMap) {
                if (mcf.flow(arc) != 1) {
                    continue;
                }
                unsigned i = (hash & 0xFFFFFFFF00000000) >> 32;
                unsigned j = hash & 0x00000000FFFFFFFF;
                doLegalMove({localCells[i], localCells[j]->tx(), localCells[j]->ty()}, false);
                mDisp = max<Cost>(mDisp, abs(localCells[i]->lx() - localCells[i]->ox()) + abs(localCells[i]->ly() - localCells[i]->oy()) * siteH / siteW);
                ++nMoves;
            }
            //  pmMaxDisp = max(pmMaxDisp, mDisp);
            printlog(LOG_INFO,
                     "cell: %s\tfid: %u\ttype: %s\tnCells: %u\tTotal flow cost: %lf\tMax Disp: %lf->%lf",
                     targetCell->getDBCell()->name().c_str(),
                     targetCell->fid,
                     targetCell->getDBCellType()->name.c_str(),
                     nNodes,
                     static_cast<double>(mcf.totalCost()),
                     oDisp, mDisp);
            if (mDisp == oDisp) {
                return 0;
            } else {
                return nMoves;
            }
        case MCF::UNBOUNDED:
            printlog(LOG_ERROR, "Unbounded");
            return 0;
        default:
            printlog(LOG_ERROR, "Unrecognized problem type.");
            return 0;
    }
}

