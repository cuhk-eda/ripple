#include "ripplegr.h"

#include <lemon/kruskal.h>
#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <sys/stat.h>

constexpr double dmdMultiplierX = 1;  // dmdMultiplier for vertical congestion, xbar: 2.1 - 2.7
constexpr double dmdMultiplierY = 1;  // dmdMultiplier for horizontal congestion, xbar: 2.1 - 2.7

std::tuple<double, double> RippleGR::getTotalTracks() {
    double totalTracksH = 0;
    double totalTracksV = 0;
    for (auto track : database.tracks) {
        double totalValue = track->num * track->numLayers();
        if (track->direction == 'v') {
            totalTracksV += totalValue / database.grGrid.gcellNX;
        } else if (track->direction == 'h') {
            totalTracksH += totalValue / database.grGrid.gcellNY;
        }
    }
    return std::make_tuple(totalTracksH, totalTracksV);
}

std::tuple<double, double> RippleGR::getWireSpace() {
    auto [totalTracksH, totalTracksV] = getTotalTracks();
    double wireSpaceH = static_cast<double>(database.grGrid.gcellStepY) / totalTracksH;
    double wireSpaceV = static_cast<double>(database.grGrid.gcellStepX) / totalTracksV;
    return std::make_tuple(wireSpaceH, wireSpaceV);
}

void RippleGR::initDemandSupplyMap() {
    for (unsigned x = 0; x != database.grGrid.gcellNX; ++x) {
        for (unsigned y = 0; y != database.grGrid.gcellNY; ++y) {
            for (unsigned int z = 0; z < database.getNumRLayers(); ++z) {
                db::GCell* gcell = database.grGrid.getGCell(x, y, z);
                gcell->demandX = 0.0;
                gcell->demandY = 0.0;
                gcell->supplyX = 0.0;
                gcell->supplyY = 0.0;
            }
        }
    }
}

void RippleGR::calcDemandMap() {
    const unsigned nz = database.getNumRLayers();
    vector<vector<vector<double>>> tempDemandX(database.grGrid.gcellNX,
                                               vector<vector<double>>(database.grGrid.gcellNY, vector<double>(nz, 0)));
    vector<vector<vector<double>>> tempDemandY(database.grGrid.gcellNX,
                                               vector<vector<double>>(database.grGrid.gcellNY, vector<double>(nz, 0)));
    auto [wireSpaceH, wireSpaceV] = getWireSpace();

    typedef lemon::ListGraph LGraph;
    typedef LGraph::Edge LEdge;
    typedef LGraph::Node LNode;
    typedef LGraph::NodeIt LNodeIt;
    typedef LGraph::EdgeIt LEdgeIt;
    typedef LGraph::EdgeMap<int> LECostMap;
    typedef LGraph::EdgeMap<bool> LEBoolMap;
    using lemon::INVALID;
    LGraph g;

    for (auto net : database.nets) {
        g.clear();
        vector<LNode> nodeVec;
        vector<LEdge> edgeVec;
        vector<int> manhattenDistVec;

        auto netBBox = utils::BoxT<double>();
        for (auto pin : net->pins) {
            utils::BoxT<double> cellBBox = pin->getPinParentBBox();
            netBBox.Set(netBBox.UnionWith(cellBBox));
            nodeVec.push_back(g.addNode());
        }
        for (unsigned int i = 0; i < net->pins.size(); i++) {
            for (unsigned int j = i; j < net->pins.size(); j++) {
                if (i == j) continue;  // complete graph
                edgeVec.push_back(g.addEdge(nodeVec[i], nodeVec[j]));
                utils::PointT<int> cp1 = net->pins[i]->getPinParentCenter();
                utils::PointT<int> cp2 = net->pins[j]->getPinParentCenter();
                manhattenDistVec.push_back(utils::Dist<int>(cp1, cp2));
            }
        }

        LECostMap edgeCostMap(g);
        for (auto& edge : edgeVec) {
            int pos = &edge - &edgeVec[0];
            edgeCostMap.set(edge, manhattenDistVec[pos]);
        }
        LEBoolMap boolTreeMap(g);
        kruskal(g, edgeCostMap, boolTreeMap);  // get MST

        vector<std::pair<int, int>> subNets;  // store decomposed net
        for (LEdgeIt edgeit(g); edgeit != INVALID; ++edgeit) {
            if (boolTreeMap[edgeit]) {
                int p1Index = -1;
                int p2Index = -1;
                for (auto& tempNode : nodeVec) {
                    int idx = &tempNode - &nodeVec[0];
                    if (tempNode == g.u(edgeit)) {
                        p1Index = idx;
                    }
                    if (tempNode == g.v(edgeit)) {
                        p2Index = idx;
                    }
                    if (p1Index != -1 && p2Index != -1) break;
                }
                if (p1Index == -1) {
                    printlog(LOG_ERROR, "Cannot find Node %d in Nets %s", g.id(g.u(edgeit)), net->name.c_str());
                }
                if (p2Index == -1) {
                    printlog(LOG_ERROR, "Cannot find Node %d in Nets %s", g.id(g.v(edgeit)), net->name.c_str());
                }
                subNets.push_back(std::make_pair(p1Index, p2Index));
            }
        }

        int netLxIndex = (netBBox.lx() - database.grGrid.gcellL) / database.grGrid.gcellStepX;
        int netLyIndex = (netBBox.ly() - database.grGrid.gcellB) / database.grGrid.gcellStepY;
        int netHxIndex = (netBBox.hx() - database.grGrid.gcellL) / database.grGrid.gcellStepX;
        int netHyIndex = (netBBox.hy() - database.grGrid.gcellB) / database.grGrid.gcellStepY;
        netLxIndex = std::max(netLxIndex, 0);
        netLyIndex = std::max(netLyIndex, 0);
        netHxIndex = std::min<int>(netHxIndex, database.grGrid.gcellNX - 1);
        netHyIndex = std::min<int>(netHyIndex, database.grGrid.gcellNY - 1);

        for (auto [p1Idx, p2Idx] : subNets) {
            utils::BoxT<double> p1BBox = net->pins[p1Idx]->getPinParentBBox();
            utils::BoxT<double> p2BBox = net->pins[p2Idx]->getPinParentBBox();
            auto subNetBBox = p1BBox.UnionWith(p2BBox);
            int subNetLxIndex = (subNetBBox.lx() - database.grGrid.gcellL) / database.grGrid.gcellStepX;
            int subNetLyIndex = (subNetBBox.ly() - database.grGrid.gcellB) / database.grGrid.gcellStepY;
            int subNetHxIndex = (subNetBBox.hx() - database.grGrid.gcellL) / database.grGrid.gcellStepX;
            int subNetHyIndex = (subNetBBox.hy() - database.grGrid.gcellB) / database.grGrid.gcellStepY;
            int lxIdx = std::max(subNetLxIndex, 0);
            int lyIdx = std::max(subNetLyIndex, 0);
            int hxIdx = std::min<int>(subNetHxIndex, database.grGrid.gcellNX - 1);
            int hyIdx = std::min<int>(subNetHyIndex, database.grGrid.gcellNY - 1);
            double subNetBBoxArea = subNetBBox.area();
            for (int i = lxIdx; i <= hxIdx; i++) {
                for (int j = lyIdx; j <= hyIdx; j++) {
                    // for (int k = 0; k < nz; k++) {
                    // NOTE: Lixin LIU: only calculate layer 0's demand and supply for reducing computation without
                    // sacrificing quality since there is a summation of all layers' demand/supply when calculating
                    // the congestion map
                    for (int k = 0; k < 1; k++) {
                        db::GCell* gcell = database.grGrid.getGCell(i, j, k);
                        utils::BoxT<double> gcellBBox(gcell->lx, gcell->ly, gcell->hx, gcell->hy);
                        double intersectArea = subNetBBox.IntersectWith(gcellBBox).area();
                        // allocate demand
                        const double subDemandY =
                            intersectArea / subNetBBoxArea * wireSpaceV * subNetBBox.height() * dmdMultiplierX;
                        const double subDemandX =
                            intersectArea / subNetBBoxArea * wireSpaceH * subNetBBox.width() * dmdMultiplierY;
                        // follow the paper setting (max function)

                        // NOTE: (for xbar) multiplying by database.getNumRLayers() may be a little bit different from
                        // Xu He's version
                        tempDemandX[i][j][k] = std::max(subDemandX, tempDemandX[i][j][k]);
                        tempDemandY[i][j][k] = std::max(subDemandY, tempDemandY[i][j][k]);
                        // tempDemandX[i][j][k] += subDemandX;
                        // tempDemandY[i][j][k] += subDemandY;
                    }
                }
            }
        }

        for (int i = netLxIndex; i <= netHxIndex; i++) {
            for (int j = netLyIndex; j <= netHyIndex; j++) {
                // for (int k = 0; k < nz; k++) {
                // NOTE: only calculate layer 0's demand and supply for reducing computation
                for (int k = 0; k < 1; k++) {
                    db::GCell* gcell = database.grGrid.getGCell(i, j, k);
                    // update demand
                    gcell->demandX += tempDemandX[i][j][k];
                    gcell->demandY += tempDemandY[i][j][k];
                    tempDemandX[i][j][k] = 0.0;
                    tempDemandY[i][j][k] = 0.0;
                }
            }
        }
    }
}

void RippleGR::calcSupplyMap(double BlockPorosity) {
    double oriSupply = database.grGrid.gcellStepX * database.grGrid.gcellStepY;
    for (unsigned x = 0; x != database.grGrid.gcellNX; ++x) {
        for (unsigned y = 0; y != database.grGrid.gcellNY; ++y) {
            // for (unsigned int z = 0; z < database.getNumRLayers(); z++) {
            // NOTE: only calculate layer 0's demand and supply for reducing computation
            for (unsigned int z = 0; z < 1; z++) {
                db::GCell* gcell = database.grGrid.getGCell(x, y, z);
                gcell->supplyX = oriSupply;
                gcell->supplyY = oriSupply;
            }
        }
    }
    if (database.routeBlockages.size() > 0) {
        auto [totalTracksH, totalTracksV] = getTotalTracks();
        std::unordered_map<string, int> layer_name2track_num;
        for (auto track : database.tracks) {
            int trackNum = track->num;
            for (auto layerName : track->getLayers()) {
                std::pair<std::unordered_map<string, int>::iterator, bool> mapInsRet =
                    layer_name2track_num.insert(std::make_pair(layerName, trackNum));
                if (!mapInsRet.second) {
                    printlog(
                        LOG_ERROR, "Fail to insert net(%s, %d) to layer_name2track_num", layerName.c_str(), trackNum);
                }
            }
        }
        for (auto& blk : database.routeBlockages) {
            std::unordered_map<string, int>::const_iterator mt{layer_name2track_num.find(blk.layer.name())};
            if (mt == layer_name2track_num.end()) {
                printlog(LOG_ERROR, "Cannot find layer_name %s in layer_name2track_num", blk.layer.name().c_str());
            }
            double bRatio{mt->second * (1 - BlockPorosity)};
            if (blk.layer.direction == 'h') {
                bRatio /= totalTracksH;
            } else if (blk.layer.direction == 'v') {
                bRatio /= totalTracksV;
            }
            utils::BoxT<double> blkBBox(blk.lx, blk.ly, blk.hx, blk.hy);
            int blkLxIndex = (blkBBox.lx() - database.grGrid.gcellL) / database.grGrid.gcellStepX;
            int blkLyIndex = (blkBBox.ly() - database.grGrid.gcellB) / database.grGrid.gcellStepY;
            int blkHxIndex = (blkBBox.hx() - database.grGrid.gcellL) / database.grGrid.gcellStepX;
            int blkHyIndex = (blkBBox.hy() - database.grGrid.gcellB) / database.grGrid.gcellStepY;
            blkLxIndex = std::max(blkLxIndex, 0);
            blkLyIndex = std::max(blkLyIndex, 0);
            blkHxIndex = std::min<int>(blkHxIndex, database.grGrid.gcellNX - 1);
            blkHyIndex = std::min<int>(blkHyIndex, database.grGrid.gcellNY - 1);
            for (int i = blkLxIndex; i <= blkHxIndex; i++) {
                for (int j = blkLyIndex; j <= blkHyIndex; j++) {
                    // for (unsigned int k = 0; k < database.getNumRLayers(); k++) {
                    // NOTE: only calculate layer 0's demand and supply for reducing computation
                    for (unsigned int k = 0; k < 1; k++) {
                        db::GCell* gcell = database.grGrid.getGCell(i, j, k);
                        utils::BoxT<double> gcellBBox(gcell->lx, gcell->ly, gcell->hx, gcell->hy);
                        double intersectArea = blkBBox.IntersectWith(gcellBBox).area();
                        double blockSupply = intersectArea * bRatio;
                        if (blk.layer.direction == 'h') {
                            gcell->supplyY -= blockSupply;
                        } else if (blk.layer.direction == 'v') {
                            gcell->supplyX -= blockSupply;
                        }
                    }
                }
            }
        }
    }
}

void RippleGR::writeCongestionMap() {
    string logFolder = db::DBModule::LogFolderName;
    if (mkdir(logFolder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            printlog(LOG_ERROR, "Unable to create folder: %s", logFolder.c_str());
        }
    }
    string TIMESTAMP = logFolder + "/gp_" + utils::Timer::getCurTimeStr(false) + "_prob";
    if (mkdir(TIMESTAMP.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            printlog(LOG_ERROR, "Unable to create folder: %s", TIMESTAMP.c_str());
        }
    }
    string outputFileName = TIMESTAMP + "/" + "prob_congestion.txt";  // From cugr
    std::ofstream ofs(outputFileName);
    ofs << "# layer: " << database.getNumRLayers() << std::endl;
    ofs << "# gcell(x): " << database.grGrid.gcellNX << std::endl;
    ofs << "# gcell(y): " << database.grGrid.gcellNY << std::endl;
    for (unsigned int z = 0; z < database.getNumRLayers(); z++) {
        const db::Layer* layer = database.getRLayer(z);
        for (unsigned x = 0; x != database.grGrid.gcellNX; ++x) {
            for (unsigned y = 0; y != database.grGrid.gcellNY; ++y) {
                // only layer 0 contains demand/supply value
                db::GCell* gcell = database.grGrid.getGCell(x, y, 0);
                double supplyValue, demandValue;
                if (layer->direction == 'h') {
                    supplyValue = gcell->supplyY;
                    demandValue = gcell->demandY;
                } else {
                    supplyValue = gcell->supplyX;
                    demandValue = gcell->demandX;
                }
                ofs << supplyValue << "," << demandValue;
                if (y != database.grGrid.gcellNY - 1) ofs << " ";
            }
            if (x != database.grGrid.gcellNX - 1) ofs << "\t";
        }
        ofs << std::endl;
    }
    ofs.close();
    // std::cout << "write prob_congestion.txt" << std::endl;
}

bool ripplegr() {
    RippleGR rippleGRLauncher;
    rippleGRLauncher.initDemandSupplyMap();
    rippleGRLauncher.calcDemandMap();
    rippleGRLauncher.calcSupplyMap();
    rippleGRLauncher.writeCongestionMap();
    return true;
}

// int ripplegr(GRResult& result, bool ndr, bool reset) {
//     circuit.SetupGRGrid(grSetting.gcellTrackX, grSetting.gcellTrackY);

//     double gcell_w = grSetting.gcellTrackX * circuit.droute_step_x;
//     double gcell_h = grSetting.gcellTrackY * circuit.droute_step_y;

//     //--clear data
//     for (unsigned i = 0; i < circuit.nets.size(); i++) {
//         circuit.groute_net_seg_h[i].clear();
//         circuit.groute_net_seg_v[i].clear();
//     }
//     for (unsigned y = 0; y < circuit.groute_num_y; y++) {
//         for (unsigned x = 0; x < circuit.groute_num_x; x++) {
//             circuit.groute_net_h[x][y].clear();
//             circuit.groute_net_v[x][y].clear();
//             circuit.groute_net_pin[x][y].clear();
//         }
//     }

//     for (int n = 0; n < (int)circuit.nets.size(); n++) {
//         for (unsigned p = 0; p < circuit.nets[n].pins.size(); p++) {
//             vector<long long> gcell;
//             FPin& pin = circuit.nets[n].pins[p];
//             double pin_x, pin_y;
//             int gx, gy, gz;
//             if (pin.cell >= 0) {
//                 int cell = pin.cell;
//                 int type = circuit.cells[pin.cell].type;
//                 double offset_x =
//                     0.5 * (circuit.celltypes[type].pins[pin.pin].hx + circuit.celltypes[type].pins[pin.pin].lx);
//                 double offset_y =
//                     0.5 * (circuit.celltypes[type].pins[pin.pin].hy + circuit.celltypes[type].pins[pin.pin].ly);
//                 pin_x = circuit.cells[cell].x + offset_x - circuit.groute_lx;
//                 pin_y = circuit.cells[cell].y + offset_y - circuit.groute_ly;
//                 gx = (int)(pin_x / gcell_w);
//                 gy = (int)(pin_y / gcell_h);
//                 gz = circuit.celltypes[type].pins[pin.pin].layer;
//             } else if (pin.io >= 0) {
//                 int io = pin.io;
//                 double offset_x = 0.5 * (circuit.iopins[io].hx + circuit.iopins[io].lx);
//                 double offset_y = 0.5 * (circuit.iopins[io].hy + circuit.iopins[io].ly);
//                 pin_x = circuit.iopins[io].x + offset_x - circuit.groute_lx;
//                 pin_y = circuit.iopins[io].y + offset_y - circuit.groute_ly;
//                 pin_x = max(0.0, min(circuit.groute_hx - circuit.groute_lx, pin_x));
//                 pin_y = max(0.0, min(circuit.groute_hy - circuit.groute_ly, pin_y));
//                 gx = (int)(pin_x / gcell_w);
//                 gy = (int)(pin_y / gcell_h);
//                 gz = circuit.iopins[io].layer;
//             } else {
//                 cerr << "unknown pin type at net " << circuit.nets[n].name << " pin " << p << endl;
//                 exit(1);
//             }
//             if (gx < 0 || gx >= (int)circuit.groute_num_x || gy < 0 || gy >= (int)circuit.groute_num_y || gz < 0 ||
//                 gz >= (int)circuit.groute_num_z) {
//                 cout << "gx=" << gx << "," << pin_x << "(" << circuit.groute_num_x * gcell_w << "," <<
//                 circuit.groute_hx
//                      << ")" << endl;
//                 cout << "gy=" << gy << "," << pin_y << "(" << circuit.groute_num_y * gcell_h << "," <<
//                 circuit.groute_hy
//                      << ")" << endl;
//                 cout << "gz=" << gz << "(" << circuit.groute_num_z << ")" << endl;
//                 cout << "cell=" << pin.cell << endl;
//                 cout << "io=" << pin.io << endl;
//             }
//         }
//     }

//     //--read routing demand
//     for (unsigned x = 0; x < circuit.groute_num_x; x++) {
//         for (unsigned y = 0; y < circuit.groute_num_y; y++) {
//             circuit.gr_demand_2d_h[x][y] = 0;
//             circuit.gr_demand_2d_v[x][y] = 0;
//         }
//     }
// }
