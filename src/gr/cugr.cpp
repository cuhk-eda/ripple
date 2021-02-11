#include "cugr.h"

#include <sys/stat.h>

#include "../gp/gp_data.h"

string TIMESTAMP;
string logFolder;
string tmpOutputDef;
string command;
string commandRm;
string outputFileName;
int numLayers;
vector<char> dir;

void initializeVariable() {
    logFolder = db::DBModule::LogFolderName;
    if (mkdir(logFolder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            printlog(LOG_ERROR, "Unable to create folder: %s", logFolder.c_str());
        }
    }
    TIMESTAMP = logFolder + "/gp_" + utils::Timer::getCurTimeStr(false) + "_cugr";
    tmpOutputDef = TIMESTAMP + "/" + io::IOModule::DefPlacement;
    CUGRSetting cugrSetting;
    outputFileName = TIMESTAMP + "/" + "cugr_congestion.txt";  // From cugr
    command = "./iccad19gr -lef " + io::IOModule::LefCell + " -def " + tmpOutputDef + " -output " + TIMESTAMP + "/" +
              "tmp.guide -threads " + to_string(cugrSetting.threads) + " -rrrIters " +
              to_string(cugrSetting.iterations) + " -cgmap " + outputFileName;

    numLayers = database.getNumRLayers();
    dir.resize(numLayers, 'x');
    for (int z = 0; z != numLayers; ++z) {
        dir[z] = database.getRLayer(z)->track.direction;
    }
}

bool cugr() {
    initializeVariable();
    writeDEF(io::IOModule::DefCell, tmpOutputDef);

    printlog(LOG_INFO, "%s\n", command.c_str());
    const int status = system(command.c_str());
    printlog(LOG_INFO, "The value returned was: %d.\n", status);
    printlog(LOG_INFO, "Loading CUGR's output file!");
    readCUGROutputTxt(outputFileName);
    return true;
}

bool readCUGROutputTxt(const string& inputTxt) {
    ifstream ifs(inputTxt.c_str());
    if (!ifs.good()) {
        printlog(LOG_ERROR, "Unable to open TXT: %s", inputTxt.c_str());
        return false;
    }
    int numHRouteLayer = 0;
    int numVRouteLayer = 0;
    for (auto curLayerDir : dir) {
        if (curLayerDir == 'h') {
            numHRouteLayer++;
        } else if (curLayerDir == 'v') {
            numVRouteLayer++;
        }
    }
    string line;
    int xIndex = 0;
    int yIndex = 0;
    int zIndex = 0;
    int lineCounter = 0;
    while (getline(ifs, line)) {
        istringstream iss(line);
        if (line[0] == '#') {
            vector<string> strings;
            istringstream issLineStr(line);
            string subStr;
            while (getline(issLineStr, subStr, ' ')) {
                strings.push_back(subStr);
            }
            if (!strings[1].compare("layer:")) {
                printlog(LOG_INFO, "num of layers: %d", std::stoi(strings.back()));
                if (std::stoi(strings.back()) != numLayers) {
                    printlog(LOG_ERROR, "The num of layers in ripple is different from CUGR");
                    return false;
                }
            } else if (!strings[1].compare("gcell(x):")) {
                printlog(LOG_INFO, "num of gcell(x): %lu", std::stoul(strings.back()));
                if (std::stoul(strings.back()) != database.grGrid.gcellNX) {
                    printlog(LOG_ERROR, "The num of gcell(x) in ripple is different from CUGR");
                    return false;
                }
            } else if (!strings[1].compare("gcell(y):")) {
                printlog(LOG_INFO, "num of gcell(y): %lu", std::stoul(strings.back()));
                if (std::stoul(strings.back()) != database.grGrid.gcellNY) {
                    printlog(LOG_ERROR, "The num of gcell(y) in ripple is different from CUGR");
                    return false;
                }
            } else {
                printlog(LOG_ERROR, "TXT File Format error: %s", inputTxt.c_str());
                return false;
            }
            continue;
        } else {
            xIndex = lineCounter % database.grGrid.gcellNX;
            yIndex = 0;
            zIndex = lineCounter / database.grGrid.gcellNX;
            vector<string> strings;
            istringstream issLineStr(line);
            string subStr;
            while (getline(issLineStr, subStr, ')')) {
                strings.push_back(subStr);
            }
            assert(strings.size() == database.grGrid.gcellNY);
            for (auto str : strings) {
                std::size_t found = str.find("(");
                if (found != string::npos) {
                    string threeElmTupleStr = str.substr(found + 1, string::npos);
                    istringstream issThreeElmTupleStr(threeElmTupleStr);
                    vector<int> items;
                    string itemStr;
                    while (getline(issThreeElmTupleStr, itemStr, ' ')) {
                        // items[0] capacity; items[1] numWire; items[2] numVia
                        if (*itemStr.rbegin() == ',') {
                            itemStr.pop_back();
                        }
                        items.push_back(std::stoi(itemStr));
                    }
                    db::GCell* gcell = database.grGrid.getGCell(xIndex, yIndex, zIndex);
                    if (dir[zIndex] == 'v') {
                        gcell->demandX = static_cast<float>(items[1]) + 1.5 * sqrt(static_cast<float>(items[2]));
                        // NOTE: use mean vertical demand value here
                        // gcell->demandX = (static_cast<float>(items[1]) + 1.5 * sqrt(static_cast<float>(items[2]))) / numHRouteLayer;
                        gcell->demandY = 0.0;
                        gcell->supplyX = items[0];
                        gcell->supplyY = 0;
                    } else if (dir[zIndex] == 'h') {
                        gcell->demandX = 0.0;
                        gcell->demandY = static_cast<float>(items[1]) + 1.5 * sqrt(static_cast<float>(items[2]));
                        // gcell->demandY = (static_cast<float>(items[1]) + 1.5 * sqrt(static_cast<float>(items[2]))) / numVRouteLayer;
                        gcell->supplyX = 0;
                        gcell->supplyY = items[0];
                    } else {
                        printlog(LOG_ERROR, "Direction error: %c", dir[zIndex]);
                        return false;
                    }
                }
                yIndex++;
            }
            lineCounter++;
        }
    }
    return true;
}

int getCellPos(const double cellX,
               const double cellAW,
               const double scale,
               const int coreLX,
               const int coreHX,
               int& overflow_cnt) {
    const int min_x = coreLX;
    const int max_x = coreHX - lround(cellAW * scale);
    int x = lround((cellX - 0.5 * cellAW) * scale) + coreLX;
    if (x < min_x || x > max_x) {
        overflow_cnt++;
        x = max(min_x, min(x, max_x));
    }
    return x;
}

//********** Modify From "../io/file_lefdef_db.cpp" **************

//  #define WRITE_BUFFER
bool writeComponents(ofstream& ofs) {
    int nCells = database.cells.size();
    ofs << "COMPONENTS " << nCells << " ;" << endl;
    int overflow_counter = 0;
    int c_i = 0;
    for (int i = 0; i < nCells; i++) {
        db::Cell* cell = database.cells[i];
        int x = 0;
        int y = 0;
        double scale = database.siteW;
        if (cell->fixed()) {
            x = cell->lx();
            y = cell->ly();
        } else {
            // x = round((cellX[c_i] - 0.5 * cellAW[c_i]) * scale) + database.coreLX;
            // y = round((cellY[c_i] - 0.5 * cellAH[c_i]) * scale) + database.coreLY;
            // fix overflow cells
            x = getCellPos(cellX[c_i], cellAW[c_i], scale, database.coreLX, database.coreHX, overflow_counter);
            y = getCellPos(cellY[c_i], cellAH[c_i], scale, database.coreLY, database.coreHY, overflow_counter);
            c_i++;
        }
#ifdef WRITE_BUFFER
        ostringstream oss;
#else
        ofstream& oss = ofs;
#endif
        oss << "   - " << cell->name() << " " << cell->ctype()->name << endl;
        if (cell->fixed()) {
            oss << "      + FIXED ( " << x << " " << y << " ) " << cugrGetOrient(cell->flipX(), cell->flipY()) << " ;"
                << endl;
        } else if (cell->placed()) {
            oss << "      + PLACED ( " << x << " " << y << " ) " << cugrGetOrient(cell->flipX(), cell->flipY()) << " ;"
                << endl;
        } else {
            oss << "      + UNPLACED ;" << endl;
        }
        // #ifdef WRITE_BUFFER
        //         string lines = oss.str();
        //         writeBuffer(ofs, lines);
        // #endif
    }
    // #ifdef WRITE_BUFFER
    //     writeBufferFlush(ofs);
    // #endif
    ofs << "END COMPONENTS\n\n";
    if (overflow_counter > 0) {
        printlog(LOG_WARN, "#%d cells are placed outside the boundary.", overflow_counter);
    }
    return true;
}

bool writeDEF(const string& inputDef, const string& outputDef) {
    if (mkdir(TIMESTAMP.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
        printlog(LOG_ERROR, "Unable to create folder: %s", TIMESTAMP.c_str());
    }

    ifstream ifs(inputDef.c_str());
    if (!ifs.good()) {
        printlog(LOG_ERROR, "Unable to open DEF: %s", inputDef.c_str());
        return false;
    }

#ifndef NDEBUG
    printlog(LOG_INFO, "reading %s", inputDef.c_str());
#endif

    ofstream ofs(outputDef.c_str());
    if (!ofs.good()) {
        printlog(LOG_ERROR, "Unable to create DEF: %s", outputDef.c_str());
        return false;
    }
    printlog(LOG_INFO, "writing %s", outputDef.c_str());

    string line;
    while (getline(ifs, line)) {
        istringstream iss(line);
        string s;
        if (!(iss >> s)) {
            ofs << line << endl;
            continue;
        } else if (s != "COMPONENTS") {
            ofs << line << endl;
            continue;
        }
        writeComponents(ofs);
        while (getline(ifs, line)) {
            istringstream iss(line);
            if (iss >> s && s == "END") {
                break;
            }
        }
        // process pair (a,b)
    }
    printlog(LOG_INFO, "Finish writing %s !", outputDef.c_str());
    return true;
}

string cugrGetOrient(bool flipX, bool flipY) {
    if (flipX) {
        if (flipY) {
            return "S";
        } else {
            return "FN";
        }
    } else {
        if (flipY) {
            return "FS";
        } else {
            return "N";
        }
    }
}
