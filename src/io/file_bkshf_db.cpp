#include <set>

#include "../db/db.h"
#include "../global.h"
#include "io.h"

using namespace db;

class BookshelfData {
public:
    int nCells;
    int nTypes;
    unsigned nNets;
    int nRows;

    std::string format;
    map<string, int> cellMap;
    map<string, int> typeMap;
    vector<string> cellName;
    vector<int> cellX;
    vector<int> cellY;
    vector<char> cellFixed;
    vector<string> typeName;
    vector<int> cellType;
    vector<int> typeWidth;
    vector<int> typeHeight;
    vector<char> typeFixed;
    map<string, int> typePinMap;
    vector<int> typeNPins;
    vector<vector<string>> typePinName;
    vector<vector<char>> typePinDir;
    vector<vector<double>> typePinX;
    vector<vector<double>> typePinY;
    vector<string> netName;
    vector<vector<int>> netCells;
    vector<vector<int>> netPins;
    vector<int> rowX;
    vector<int> rowY;
    vector<int> rowSites;
    int siteWidth;
    int siteHeight;

    int gridNX;
    int gridNY;
    int gridNZ;
    vector<int> capV;
    vector<int> capH;
    vector<int> wireWidth;
    vector<int> wireSpace;
    vector<int> viaSpace;
    int gridOriginX;
    int gridOriginY;
    int tileW;
    int tileH;
    double blockagePorosity;

    BookshelfData() {
        nCells = 0;
        nTypes = 0;
        nNets = 0;
        nRows = 0;
    }

    void clearData() {
        cellMap.clear();
        typeMap.clear();
        cellName.clear();
        cellX.clear();
        cellY.clear();
        cellFixed.clear();
        typeName.clear();
        cellType.clear();
        typeWidth.clear();
        typeHeight.clear();
        typePinMap.clear();
        typeNPins.clear();
        typePinName.clear();
        typePinDir.clear();
        typePinX.clear();
        typePinY.clear();
        netName.clear();
        netCells.clear();
        netPins.clear();
        rowX.clear();
        rowY.clear();
        rowSites.clear();
    }

    void scaleData(int scale) {
        for (int i = 0; i < nCells; i++) {
            cellX[i] *= scale;
            cellY[i] *= scale;
        }
        for (int i = 0; i < nTypes; i++) {
            typeWidth[i] *= scale;
            typeHeight[i] *= scale;
            for (int j = 0; j < typeNPins[i]; j++) {
                typePinX[i][j] *= scale;
                typePinY[i][j] *= scale;
            }
        }
        for (int i = 0; i < nRows; i++) {
            rowX[i] *= scale;
            rowY[i] *= scale;
        }
        siteWidth *= scale;
        siteHeight *= scale;
    }

    void estimateSiteSize() {
        set<int> sizeSet;
        set<int> heightSet;

        for (int i = 0; i < nCells; i++) {
            if (cellFixed[i] == (char)1) {
                continue;
            }
            int type = cellType[i];
            int typeW = typeWidth[type];
            int typeH = typeHeight[type];
            if (sizeSet.count(typeW) == 0) {
                sizeSet.insert(typeW);
            }
            if (sizeSet.count(typeH) == 0) {
                sizeSet.insert(typeH);
            }
            if (heightSet.count(typeH) == 0) {
                heightSet.insert(typeH);
            }
        }

        vector<int> sizes;
        vector<int> heights;
        set<int>::iterator ii = sizeSet.begin();
        set<int>::iterator ie = sizeSet.end();
        for (; ii != ie; ++ii) {
            sizes.push_back(*ii);
        }
        ii = heightSet.begin();
        ie = heightSet.end();
        for (; ii != ie; ++ii) {
            heights.push_back(*ii);
        }
        siteWidth = gcd(sizes);
        siteHeight = gcd(heights);
        printlog(LOG_INFO, "estimate site size = %d x %d", siteWidth, siteHeight);
        ii = heightSet.begin();
        ie = heightSet.end();
        for (; ii != ie; ++ii) {
            printlog(LOG_INFO, "standard cell heights: %d rows", (*ii) / siteHeight);
        }
    }
    int gcd(vector<int>& nums) {
        if (nums.size() == 0) {
            return 0;
        }
        if (nums.size() == 1) {
            return nums[0];
        }
        int primes[20] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71};
        int greatestFactor = 1;
        for (bool factorFound = true; factorFound;) {
            factorFound = false;
            for (int f = 0; f < 10; f++) {
                int factor = primes[f];
                bool factorValid = true;
                for (int i = 0; i < (int)nums.size(); i++) {
                    int num = nums[i];
                    // printlog(LOG_INFO, "%d : %d", i, num);
                    if (num % factor != 0) {
                        factorValid = false;
                        break;
                    }
                }
                if (!factorValid) {
                    continue;
                }
                greatestFactor *= factor;
                for (int i = 0; i < (int)nums.size(); i++) {
                    nums[i] /= factor;
                }
                factorFound = true;
                break;
            }
        }
        return greatestFactor;
    }
};

BookshelfData bsData;

bool isBookshelfSymbol(unsigned char c) {
    static char symbols[256] = {0};
    static bool inited = false;
    if (!inited) {
        symbols[(int)'('] = 1;
        symbols[(int)')'] = 1;
        // symbols[(int)'['] = 1;
        // symbols[(int)']'] = 1;
        symbols[(int)','] = 1;
        // symbols[(int)'.'] = 1;
        symbols[(int)':'] = 1;
        symbols[(int)';'] = 1;
        // symbols[(int)'/'] = 1;
        symbols[(int)'#'] = 1;
        symbols[(int)'{'] = 1;
        symbols[(int)'}'] = 1;
        symbols[(int)'*'] = 1;
        symbols[(int)'\"'] = 1;
        symbols[(int)'\\'] = 1;

        symbols[(int)' '] = 2;
        symbols[(int)'\t'] = 2;
        symbols[(int)'\n'] = 2;
        symbols[(int)'\r'] = 2;
        inited = true;
    }
    return symbols[(int)c] != 0;
}

bool readBSLine(istream& is, vector<string>& tokens) {
    tokens.clear();
    string line;
    while (is && tokens.empty()) {
        // read next line in
        getline(is, line);

        char token[1024] = {0};
        int lineLen = (int)line.size();
        int tokenLen = 0;
        for (int i = 0; i < lineLen; i++) {
            char c = line[i];
            if (c == '#') {
                break;
            }
            if (isBookshelfSymbol(c)) {
                if (tokenLen > 0) {
                    token[tokenLen] = (char)0;
                    tokens.push_back(string(token));
                    token[0] = (char)0;
                    tokenLen = 0;
                }
            } else {
                token[tokenLen++] = c;
                if (tokenLen > 1024) {
                    // TODO: unhandled error
                    tokens.clear();
                    return false;
                }
            }
        }
        // line finished, something else in token
        if (tokenLen > 0) {
            token[tokenLen] = (char)0;
            tokens.push_back(string(token));
            tokenLen = 0;
        }
    }
    return !tokens.empty();
}

void printTokens(vector<string>& tokens) {
    for (auto const& token : tokens) {
        std::cout << token << " : ";
    }
    std::cout << std::endl;
}

bool Database::readBSAux(const std::string& auxFile, const std::string& plFile) {
    std::string directory;
    unsigned found = auxFile.find_last_of("/\\");
    if (found == auxFile.npos) {
        directory = "./";
    } else {
        directory = auxFile.substr(0, found);
        directory += "/";
    }
    printlog(LOG_INFO, "dir = %s", directory.c_str());

    ifstream fs(auxFile.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", auxFile.c_str());
        return false;
    }

    vector<string> tokens;
    readBSLine(fs, tokens);
    fs.close();

    std::string fileNodes;
    std::string fileNets;
    std::string fileScl;
    std::string fileRoute;
    std::string fileShapes;
    std::string fileWts;
    std::string filePl;

    for (int i = 1; i < (int)tokens.size(); i++) {
        const string& file = tokens[i];
        const size_t dotPos = file.find_last_of(".");
        if (dotPos == string::npos) {
            continue;
        }

        string ext = file.substr(dotPos);
        if (ext == ".nodes") {
            fileNodes = directory + file;
        } else if (ext == ".nets") {
            fileNets = directory + file;
        } else if (ext == ".scl") {
            fileScl = directory + file;
        } else if (ext == ".route") {
            fileRoute = directory + file;
        } else if (ext == ".shapes") {
            fileShapes = directory + file;
        } else if (ext == ".wts") {
            fileWts = directory + file;
        } else if (ext == ".pl") {
            filePl = directory + file;
        } else {
            printlog(LOG_ERROR, "unrecognized file extension: %s", ext.c_str());
        }
    }
    // step 1: read floorplan, rows from:
    //  scl - rows
    // step 2: read cell types from:
    //  nets - pin name, pin offset
    //  nodes - with, height, cell type name
    // step 3: read cell from:
    //  nodes - cell name, type
    // step 4: read placement:
    //  pl - cell fix/movable, position

    bsData.format = io::IOModule::BookshelfVariety;
    bsData.nCells = 0;
    bsData.nTypes = 0;
    bsData.nNets = 0;
    bsData.nRows = 0;
    // bsData.siteWidth = 240;
    // bsData.siteHeight = 2880;

    readBSNodes(fileNodes);

    bsData.estimateSiteSize();

    if (bsData.format == "iccad2012") {
        readBSNets(fileNets);
        readBSRoute(fileRoute);
        readBSShapes(fileShapes);
        readBSWts(fileWts);
        // readBSPl    ( filePl    );
        readBSPl(plFile);
        readBSScl(fileScl);
    } else if (bsData.format == "wu2016" || bsData.format == "lin2016") {
        readBSNets(fileNets);
        // readBSRoute ( fileRoute );
        // readBSShapes( fileShapes);
        readBSWts(fileWts);
        // readBSPl    ( filePl    );
        readBSPl(plFile);
        readBSScl(fileScl);
    }

    if (bsData.siteWidth < 10) {
        bsData.scaleData(100);
        // database.scale = 100;
    } else if (bsData.siteWidth < 100) {
        bsData.scaleData(10);
        // database.scale = 10;
    } else {
        // database.scale = 1;
    }

    database.dieLX = INT_MAX;
    database.dieLY = INT_MAX;
    database.dieHX = INT_MIN;
    database.dieHY = INT_MIN;
    for (int i = 0; i < bsData.nRows; i++) {
        Row* row = database.addRow("core_SITE_ROW_" + to_string(i), "core", bsData.rowX[i], bsData.rowY[i]);
        row->xStep(bsData.siteWidth);
        row->yStep(bsData.siteHeight);
        row->xNum(bsData.rowSites[i]);
        row->yNum(1);
        row->flip((i % 2) == 1);
        database.dieLX = std::min(database.dieLX, row->x());
        database.dieLY = std::min(database.dieLY, row->y());
        database.dieHX = std::max(database.dieHX, row->x() + (int)row->width());
        database.dieHY = std::max(database.dieHY, row->y() + bsData.siteHeight);
    }

    int defaultPitch = bsData.siteWidth;
    int defaultWidth = bsData.siteWidth / 2;
    int defaultSpace = bsData.siteWidth - defaultWidth;

    unsigned nLayers = 9;
    for (unsigned i = 0; i != nLayers; ++i) {
        Layer& layer = database.addLayer(string("M").append(to_string(i + 1)), 'r');
        layer.direction = 'h';
        layer.pitch = defaultPitch;
        layer.offset = defaultSpace;
        layer.width = defaultWidth;
        layer.spacing = defaultSpace;
        if (!i) {
            layer.track.direction = 'x';
        } else if (i % 2) {
            layer.track.direction = 'v';
        } else {
            layer.track.direction = 'h';
        }
        if (i % 2) {
            layer.track.start = database.dieLX + (layer.pitch / 2);
            layer.track.num = (database.dieHX - database.dieLX) / layer.pitch;
        } else {
            layer.track.start = database.dieLY + (layer.pitch / 2);
            layer.track.num = (database.dieHY - database.dieLY) / layer.pitch;
        }
        layer.track.step = layer.pitch;

        if (i + 1 == nLayers) {
            break;
        } else {
            database.addLayer(string("N").append(to_string(i + 1)), 'c');
        }
    }

    const Layer& layer = database.layers[0];

    for (int i = 0; i < bsData.nTypes; i++) {
        unsigned libcell = rtimer.lLib.addCell(bsData.typeName[i]);
        CellType* celltype = database.addCellType(bsData.typeName[i], libcell);
        celltype->width = bsData.typeWidth[i];
        celltype->height = bsData.typeHeight[i];
        celltype->stdcell = (!bsData.typeFixed[i]);
        for (int j = 0; j < bsData.typeNPins[i]; ++j) {
            char direction = 'x';
            switch (bsData.typePinDir[i][j]) {
                case 'I':
                    direction = 'i';
                    break;
                case 'O':
                    direction = 'o';
                    break;
                default:
                    printlog(LOG_ERROR, "unknown pin direction: %c", bsData.typePinDir[i][j]);
                    break;
            }
            PinType* pintype = celltype->addPin(bsData.typePinName[i][j], direction, 's');
            pintype->addShape(layer, bsData.typePinX[i][j], bsData.typePinY[i][j]);
        }
    }

    bool isWu2016 = (bsData.format == "wu2016");
    for (int i = 0; i < bsData.nCells; i++) {
        int typeID = bsData.cellType[i];
        if (typeID < 0) {
            IOPin* iopin = database.addIOPin(bsData.cellName[i]);
            switch (typeID) {
                case -1:
                    iopin->type->direction('o');
                    break;
                case -2:
                    iopin->type->direction('i');
                    break;
                default:
                    iopin->type->direction('x');
                    break;
            }
            iopin->type->addShape(layer, bsData.cellX[i], bsData.cellY[i]);
        } else {
            string celltypename(bsData.typeName[typeID]);
            Cell* cell = database.addCell(bsData.cellName[i], database.getCellType(celltypename));
            cell->place(bsData.cellX[i], bsData.cellY[i], false, false);
            cell->fixed((bsData.cellFixed[i] == (char)1));
            if (!cell->fixed() && isWu2016 && cell->height() > 2 * bsData.siteHeight) {
                printlog(LOG_ERROR,
                         "cell %s [w=%d h=%d] is forced to be fixed at (%d,%d)",
                         cell->name().c_str(),
                         cell->width(),
                         cell->height(),
                         cell->lx(),
                         cell->ly());
                cell->highlighted = true;
                cell->fixed(true);
            }
        }
    }

    for (unsigned i = 0; i != bsData.nNets; ++i) {
        Net* net = database.addNet(bsData.netName[i]);
        for (unsigned j = 0; j != bsData.netCells[i].size(); ++j) {
            Pin* pin = nullptr;
            int cellID = bsData.netCells[i][j];
            if (bsData.cellType[cellID] < 0) {
                IOPin* iopin = database.getIOPin(bsData.cellName[cellID]);
                pin = iopin->pin;
            } else {
                Cell* cell = database.getCell(bsData.cellName[cellID]);
                pin = cell->pin(bsData.netPins[i][j]);
            }
            pin->net = net;
            net->addPin(pin);
        }
    }

    bsData.clearData();

    return true;
}
bool Database::readBSNodes(const std::string& file) {
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    // int nNodes = 0;
    // int nTerminals = 0;
    vector<string> tokens;
    while (readBSLine(fs, tokens)) {
        // printlog(LOG_INFO, "%d : %s", i++, tokens[0].c_str());
        if (tokens[0] == "UCLA") {
            continue;
        } else if (tokens[0] == "NumNodes") {
            // nNodes = atoi(tokens[1].c_str());
        } else if (tokens[0] == "NumTerminals") {
            // nTerminals = atoi(tokens[1].c_str());
        } else if (tokens.size() >= 3) {
            string cName = tokens[0];
            int cWidth = atoi(tokens[1].c_str());
            int cHeight = atoi(tokens[2].c_str());
            bool cFixed = false;
            string cType = cName;
            if (tokens.size() >= 4) {
                cType = tokens[3];
                //  cout << cName << '\t' << cType << endl;
            }
            if (cType == "terminal" && cWidth > 1 && cHeight > 1) {
                // printlog(LOG_INFO, "read terminal");
                cType = cName;
                cFixed = true;
            }
            if (cType == "terminal_NI") {
                cType = cName;
                cFixed = true;
            }
            int typeID = -1;
            if (cType == "terminal") {
                // printlog(LOG_INFO, "read terminal");
                typeID = -1;
                cFixed = true;
            } else if (bsData.typeMap.find(cType) == bsData.typeMap.end()) {
                typeID = bsData.nTypes++;
                bsData.typeMap[cType] = typeID;
                bsData.typeName.push_back(cType);
                bsData.typeWidth.push_back(cWidth);
                bsData.typeHeight.push_back(cHeight);
                bsData.typeFixed.push_back((char)(cFixed ? 1 : 0));
                bsData.typeNPins.push_back(0);
                bsData.typePinName.push_back(vector<string>());
                bsData.typePinDir.push_back(vector<char>());
                bsData.typePinX.push_back(vector<double>());
                bsData.typePinY.push_back(vector<double>());
                //  cout << cType << '\t' << tokens.size() << endl;
            } else {
                typeID = bsData.typeMap[cType];
                assert(cWidth == bsData.typeWidth[typeID]);
                assert(cHeight == bsData.typeHeight[typeID]);
            }
            int cellID = bsData.nCells++;
            bsData.cellMap[cName] = cellID;
            bsData.cellType.push_back(typeID);
            bsData.cellFixed.push_back((char)(cFixed ? 1 : 0));
            bsData.cellName.push_back(cName);
            bsData.cellX.push_back(0);
            bsData.cellY.push_back(0);
        }
    }

    fs.close();
    return true;
}

bool Database::readBSNets(const std::string& file) {
    std::cout << "reading net" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    while (readBSLine(fs, tokens)) {
        if (tokens[0] == "UCLA") {
            continue;
        } else if (tokens[0] == "NumNets") {
            // printlog(LOG_INFO, "#nets : %d", atoi(tokens[1].c_str()));
            int numNets = atoi(tokens[1].c_str());
            bsData.netName.resize(numNets);
            bsData.netCells.resize(numNets);
            bsData.netPins.resize(numNets);
        } else if (tokens[0] == "NumPins") {
            // printlog(LOG_INFO, "#pins : %d", atoi(tokens[1].c_str()));
        } else if (tokens[0] == "NetDegree") {
            int degree = atoi(tokens[1].c_str());
            string nName = tokens[2];
            int netID = bsData.nNets++;
            bsData.netName[netID] = nName;
            for (int i = 0; i < degree; i++) {
                readBSLine(fs, tokens);
                string cName = tokens[0];
                if (bsData.cellMap.find(cName) == bsData.cellMap.end()) {
                    assert(false);
                    printlog(LOG_ERROR, "cell not found : %s", cName.c_str());
                    return false;
                }

                int cellID = bsData.cellMap[cName];
                int typeID = bsData.cellType[cellID];
                int typePinID = -1;

                char dir = tokens[1].c_str()[0];
                double pinX = 0;
                double pinY = 0;
                pinX = bsData.typeWidth[typeID] * 0.5 + (double)atof(tokens[2].c_str());
                pinY = bsData.typeHeight[typeID] * 0.5 + (double)atof(tokens[3].c_str());
                /*
                if(tokens.size() >= 6){
                    //pinX = (int)round(atof(tokens[4].c_str())*bsData.siteWidth);
                    //pinY = (int)round(atof(tokens[5].c_str())*bsData.siteWidth);
                }else{
                    pinX = bsData.typeWidth[typeID]  * 0.5 + (int)round(atof(tokens[2].c_str()));
                    pinY = bsData.typeHeight[typeID] * 0.5 + (int)round(atof(tokens[3].c_str()));
                }
                */
                string pinName = (tokens.size() < 7) ? "" : tokens[6];

                string tpName;
                if (pinName == "" && typeID >= 0) {
                    stringstream ss;
                    ss << bsData.typeNPins[typeID];
                    pinName = ss.str();
                    // printlog(LOG_INFO, "pinname = %s", pinName.c_str());
                }
                if (typeID >= 0) {
                    tpName.append(bsData.typeName[typeID]);
                    tpName.append(":");
                    tpName.append(pinName);
                }
                if (typeID == -1) {
                    // IOPin
                    if (dir == 'I') {
                        typeID = -1;
                    } else if (dir == 'O') {
                        typeID = -2;
                    } else {
                        typeID = -3;
                    }
                } else if (bsData.typePinMap.find(tpName) == bsData.typePinMap.end()) {
                    typePinID = bsData.typeNPins[typeID]++;
                    bsData.typePinMap[tpName] = typePinID;
                    bsData.typePinName[typeID].push_back(pinName);
                    bsData.typePinDir[typeID].push_back(dir);
                    bsData.typePinX[typeID].push_back(pinX);
                    bsData.typePinY[typeID].push_back(pinY);
                } else {
                    typePinID = bsData.typePinMap[tpName];
                    assert(bsData.typePinX[typeID][typePinID] == pinX);
                    assert(bsData.typePinY[typeID][typePinID] == pinY);
                    assert(bsData.typePinDir[typeID][typePinID] == dir);
                }
                bsData.netCells[netID].push_back(cellID);
                bsData.netPins[netID].push_back(typePinID);
            }
        }
    }

    fs.close();
    return true;
}

bool Database::readBSScl(const std::string& file) {
    std::cout << "reading scl" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    while (readBSLine(fs, tokens)) {
        if (tokens[0] == "UCLA") {
            continue;
        } else if (tokens[0] == "NumRows") {
            int numRows = atoi(tokens[1].c_str());
            bsData.rowX.resize(numRows);
            bsData.rowY.resize(numRows);
            bsData.rowSites.resize(numRows);
        } else if (tokens[0] == "Coordinate") {
            bsData.rowY[bsData.nRows] = atoi(tokens[1].c_str());
        } else if (tokens[0] == "SubrowOrigin" && tokens[2] == "NumSites") {
            bsData.rowX[bsData.nRows] = atoi(tokens[1].c_str());
            if (bsData.format == "wu2016") {
                bsData.rowSites[bsData.nRows] = atoi(tokens[3].c_str()) / bsData.siteWidth;
            } else {
                bsData.rowSites[bsData.nRows] = atoi(tokens[3].c_str());
            }
        } else if (tokens[0] == "Sitewidth") {
            // bsData.siteWidth = atoi(tokens[1].c_str());
        } else if (tokens[0] == "End") {
            bsData.nRows++;
        }
    }
    fs.close();
    return true;
}

bool Database::readBSRoute(const std::string& file) {
    std::cout << "reading route" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    const unsigned ReadingHeader = 0;
    const unsigned ReadingPinLayer = 1;
    const unsigned ReadingBlockages = 2;
    const unsigned ReadingCapAdjust = 3;
    unsigned status = ReadingHeader;
    while (readBSLine(fs, tokens)) {
        if (tokens[0] == "Grid") {
            bsData.gridNX = atoi(tokens[1].c_str());
            bsData.gridNY = atoi(tokens[2].c_str());
            bsData.gridNZ = atoi(tokens[3].c_str());
        } else if (tokens[0] == "VerticalCapacity") {
            for (unsigned i = 1; i < tokens.size(); i++) {
                bsData.capV.push_back(atoi(tokens[i].c_str()));
            }
        } else if (tokens[0] == "HorizontalCapacity") {
            for (unsigned i = 1; i < tokens.size(); i++) {
                bsData.capH.push_back(atoi(tokens[i].c_str()));
            }
        } else if (tokens[0] == "MinWireWidth") {
            for (unsigned i = 1; i < tokens.size(); i++) {
                bsData.wireWidth.push_back(atoi(tokens[i].c_str()));
            }
        } else if (tokens[0] == "MinWireSpacing") {
            for (unsigned i = 1; i < tokens.size(); i++) {
                bsData.wireSpace.push_back(atoi(tokens[i].c_str()));
            }
        } else if (tokens[0] == "ViaSpacing") {
            for (unsigned i = 1; i < tokens.size(); i++) {
                bsData.viaSpace.push_back(atoi(tokens[i].c_str()));
            }
        } else if (tokens[0] == "GridOrigin") {
            bsData.gridOriginX = atoi(tokens[1].c_str());
            bsData.gridOriginY = atoi(tokens[2].c_str());
        } else if (tokens[0] == "TileSize") {
            bsData.tileW = atoi(tokens[1].c_str());
            bsData.tileH = atoi(tokens[2].c_str());
        } else if (tokens[0] == "BlockagePorosity") {
            bsData.blockagePorosity = atof(tokens[1].c_str());
        } else if (tokens[0] == "NumNiTerminals") {
            status = ReadingPinLayer;
        } else if (tokens[0] == "NumBlockageNodes") {
            status = ReadingBlockages;
        } else if (tokens[0] == "NumEdgeCapacityAdjustments") {
            status = ReadingCapAdjust;
        } else if (status == ReadingPinLayer) {
            std::string pin = tokens[0];
            //  int layer = atoi(tokens[1].c_str());
            if (bsData.cellMap.find(pin) == bsData.cellMap.end()) {
                printlog(LOG_ERROR, "pin not found : %s", pin.c_str());
                getchar();
            }
            //  cout<<pin<<" "<<layer<<endl;
        } else if (status == ReadingBlockages) {
            std::string node = tokens[0];
            int maxLayer = 0;
            for (unsigned i = 2; i < tokens.size(); i++) {
                maxLayer = std::max(maxLayer, atoi(tokens[i].c_str()));
            }
            if (bsData.cellMap.find(node) == bsData.cellMap.end()) {
                printlog(LOG_ERROR, "node not found : %s", node.c_str());
                getchar();
            }
            //  cout<<node<<" "<<maxLayer<<endl;
        } else if (status == ReadingCapAdjust) {
            /*
            int fx = atoi(tokens[0].c_str());
            int fy = atoi(tokens[1].c_str());
            int fz = atoi(tokens[2].c_str());
            int tx = atoi(tokens[3].c_str());
            int ty = atoi(tokens[4].c_str());
            int tz = atoi(tokens[5].c_str());
            int cap = atoi(tokens[6].c_str());
            cout<<fx<<" "<<fy<<" "<<fz<<" "<<tx<<" "<<ty<<" "<<tz<<" "<<cap<<endl;
            */
        }
    }
    fs.close();
    return true;
}

bool Database::readBSShapes(const std::string& file) {
    std::cout << "reading shapes" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    fs.close();
    return true;
}

bool Database::readBSWts(const std::string& file) {
    std::cout << "reading weights" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    fs.close();
    return true;
}

bool Database::readBSPl(const std::string& file) {
    std::cout << "reading placement" << std::endl;
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    vector<string> tokens;
    while (readBSLine(fs, tokens)) {
        if (tokens[0] == "UCLA") {
            continue;
        } else if (tokens.size() >= 4) {
            map<string, int>::iterator itr = bsData.cellMap.find(tokens[0]);
            if (itr == bsData.cellMap.end()) {
                assert(false);
                printlog(LOG_ERROR, "cell not found: %s", tokens[0].c_str());
                return false;
            }
            int cell = itr->second;
            double x = atof(tokens[1].c_str());
            double y = atof(tokens[2].c_str());
            bsData.cellX[cell] = (int)round(x);
            bsData.cellY[cell] = (int)round(y);

            if (tokens.size() >= 5) {
                if (tokens[4] == "/FIXED" || tokens[4] == "/FIXED_NI") {
                    bsData.cellFixed[cell] = (char)1;
                }
            }
        }
    }
    fs.close();
    return true;
}

bool Database::writeBSPl(const std::string& file) {
    ofstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }
    fs << "UCLA pl 1.0\n";
    fs << "# User   : Chinese University of Hong Kong\n\n";
    for (auto cell : database.cells) {
        fs << '\t' << left << setw(60) << cell->name() << right << setw(8) << cell->lx() / siteW << setw(8)
           << cell->ly() / siteW << " : N";
        if (cell->fixed()) {
            if (cell->width() / siteW == 1 && cell->height() / siteW == 1) {
                fs << " /FIXED_NI";
            } else {
                fs << " /FIXED";
            }
        }
        fs << endl;
    }
    fs.close();
    return true;
}
