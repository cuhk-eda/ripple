#ifndef _DB_DB_H_
#define _DB_DB_H_

#include "../global.h"
#include "../tcl/tcl.h"
#include "../vi/vi.h"
using namespace vi;

#if __GNUC__ >= 4
#define _GNUC_4_
#if __GNUC_MINOR__ >= 8
#define _GNUC_4_8_
#endif
#if __GNUC__ >= 5
#define _GNUC_4_8_
#define _GNUC_5_
#endif
#endif

namespace db {
class Rectangle;
class Geometry;
class GeoMap;
class Cell;
class CellType;
class Pin;
class PinType;
class IOPin;
class CellPin;
class Net;
class Row;
class RowSegment;
class Track;
class Layer;
class Via;
class ViaType;
class Region;
class NDR;
class SNet;
class PowerNet;
class EdgeTypes;
class GCell;
class Placement;
}  // namespace db

#include "db_layer.h"
//
#include "db_cell.h"
#include "db_drc.h"
#include "db_geom.h"
#include "db_map.h"
#include "db_net.h"
#include "db_pin.h"
#include "db_place.h"
#include "db_region.h"
#include "db_route.h"
#include "db_row.h"
#include "db_snet.h"
#include "db_td.h"
#include "db_via.h"

namespace db {

#define CLEAR_POINTER_LIST(list) \
    {                            \
        for (auto obj : list) {  \
            delete obj;          \
        }                        \
        list.clear();            \
    }

#define CLEAR_POINTER_MAP(map) \
    {                          \
        for (auto p : map) {   \
            delete p.second;   \
        }                      \
        map.clear();           \
    }

class Database : virtual public Drawable {
public:
    enum IssueType {
        E_ROW_EXCEED_DIE,
        E_OVERLAP_ROWS,
        W_NON_UNIFORM_SITE_WIDTH,
        W_NON_HORIZONTAL_ROW,
        E_MULTIPLE_NET_DRIVING_PIN,
        E_NO_NET_DRIVING_PIN
    };

    unordered_map<string, CellType*> name_celltypes;
    unordered_map<string, Cell*> name_cells;
    unordered_map<string, Net*> name_nets;
    unordered_map<string, IOPin*> name_iopins;
    unordered_map<string, ViaType*> name_viatypes;

    vector<Layer> layers;
    vector<ViaType*> viatypes;
    vector<CellType*> celltypes;

    vector<Cell*> cells;
    vector<IOPin*> iopins;
    vector<Net*> nets;
    vector<Row*> rows;
    vector<Region*> regions;
    map<string, NDR*> ndrs;
    vector<SNet*> snets;
    vector<Track*> tracks;

    vector<Geometry> routeBlockages;
    vector<Rectangle> placeBlockages;

    PowerNet powerNet;

private:
    const size_t _bufferCapacity = 128 * 1024;
    size_t _bufferSize = 0;
    char* _buffer = nullptr;

    vector<Placement> _placements;
    unsigned _activePlacement;

public:
    unsigned siteW = 0;
    int siteH = 0;
    unsigned nSitesX = 0;
    unsigned nSitesY = 0;

    SiteMap siteMap;
    TDBins tdBins;  // local target density
    GRGrid grGrid;  // global routing grid

    EdgeTypes edgetypes;

    int dieLX, dieLY, dieHX, dieHY;
    int coreLX, coreLY, coreHX, coreHY;

    double maxDensity = 0;
    double maxDisp = 0;

    int LefConvertFactor;
    double DBU_Micron;
    double version;
    string designName;

    vector<IssueType> databaseIssues;

public:
    Database();
    ~Database();
    void clear();
    void clearTechnology();
    inline void clearLibrary() { CLEAR_POINTER_LIST(celltypes); }
    void clearDesign();

    Layer& addLayer(const string& name, const char type = 'x');
    ViaType* addViaType(const string& name, bool isDef);
    inline ViaType* addViaType(const string& name) { return addViaType(name, false); }
    CellType* addCellType(const string& name, unsigned libcell);
    void reserveCells(const size_t n) { cells.reserve(n); }
    Cell* addCell(const string& name, CellType* type = nullptr);
    IOPin* addIOPin(const string& name = "", const string& netName = "", const char direction = 'x');
    void reserveNets(const size_t n) { nets.reserve(n); }
    Net* addNet(const string& name = "", const NDR* ndr = nullptr);
    Row* addRow(const string& name,
                const string& macro,
                const int x,
                const int y,
                const unsigned xNum = 0,
                const unsigned yNum = 0,
                const bool flip = false,
                const unsigned xStep = 0,
                const unsigned yStep = 0);
    Track* addTrack(char direction, double start, double num, double step);
    Region* addRegion(const string& name = "", const char type = 'x');
    NDR* addNDR(const string& name, const bool hardSpacing);
    void reserveSNets(const size_t n) { snets.reserve(n); }
    SNet* addSNet(const string& name);

    Layer* getRLayer(const int index);
    const Layer* getCLayer(const unsigned index) const;
    Layer* getLayer(const string& name);
    CellType* getCellType(const string& name);
    Cell* getCell(const string& name);
    Net* getNet(const string& name);
    Region* getRegion(const string& name);
    Region* getRegion(const unsigned char id);
    NDR* getNDR(const string& name) const;
    IOPin* getIOPin(const string& name) const;
    ViaType* getViaType(const string& name) const;
    SNet* getSNet(const string& name);

    Placement& placement(unsigned i);
    Placement& placement();
    void setActivePlacement(unsigned i);

    unsigned getNumRLayers() const;
    unsigned getNumCLayers() const;
    inline unsigned getNumLayers() const { return layers.size(); }
    inline unsigned getNumCells() const { return cells.size(); }
    inline unsigned getNumNets() const { return nets.size(); }
    inline unsigned getNumRegions() const { return regions.size(); }
    inline unsigned getNumIOPins() const { return iopins.size(); }
    inline unsigned getNumCellTypes() const { return celltypes.size(); }

    inline int getCellTypeSpace(const CellType* L, const CellType* R) const {
        return edgetypes.getEdgeSpace(L->edgetypeR, R->edgetypeL);
    }
    inline int getCellTypeSpace(const Cell* L, const Cell* R) const { return getCellTypeSpace(L->ctype(), R->ctype()); }
    int getContainedSites(
        const int lx, const int ly, const int hx, const int hy, int& slx, int& sly, int& shx, int& shy) const;
    int getOverlappedSites(
        const int lx, const int ly, const int hx, const int hy, int& slx, int& sly, int& shx, int& shy) const;

    long long getCellArea(Region* region = nullptr) const;
    long long getFreeArea(Region* region = nullptr) const;

    bool placed();
    bool globalRouted();
    bool detailedRouted();

    void errorCheck(bool autoFix = true);
    void setup(bool liteMode = false);  // call after read

    long long getHPWL();

public:
    void checkPlaceError();
    void checkDRCError();

    /* defined in io/file_lefdef_db.cpp */
public:
    bool readLEF(const std::string& file);
    bool readDEF(const std::string& file);
    bool readDEFPG(const string& file);
    bool writeDEF(const std::string& file);
    bool writeICCAD2017(const string& inputDef, const string& outputDef);
    bool writeICCAD2017(const string& outputDef);
    bool writeComponents(ofstream& ofs);
    bool writeBuffer(ofstream& ofs, const string& line);
    void writeBufferFlush(ofstream& ofs);

    bool readBSAux(const std::string& auxFile, const std::string& plFile);
    bool readBSNodes(const std::string& file);
    bool readBSNets(const std::string& file);
    bool readBSScl(const std::string& file);
    bool readBSRoute(const std::string& file);
    bool readBSShapes(const std::string& file);
    bool readBSWts(const std::string& file);
    bool readBSPl(const std::string& file);
    bool writeBSPl(const std::string& file);

    bool readVerilog(const std::string& file);
    bool readLiberty(const std::string& file);

    bool readConstraints(const std::string& file);
    bool readSize(const std::string& file);

    bool readRipple(const std::string& file);
    bool writeRipple(const std::string& file);

private:
    void SetupLayers();
    void SetupCellLibrary();
    void SetupFloorplan();
    void SetupRegions();
    void SetupSiteMap();
    void SetupRows();
    void SetupRowSegments();
    void SetupTargetDensity();
    void SetupNets();
    void SetupGRGrid();

public:
    void setParent(Drawable* parent);
    Drawable* parent() const;
    int globalX() const;
    int globalY() const;
    int localX() const;
    int localY() const;
    int boundL() const;
    int boundR() const;
    int boundB() const;
    int boundT() const;
    void draw(Visualizer* v) const;
};

class DBModule : public ripple::ShellModule {
private:
    static std::string _name;

public:
    static bool EdgeSpacing;
    static bool EnableFence;
    static bool EnablePG;
    static bool EnableIOPin;
    static std::string LogFolderName;

    void registerCommands() { /*ripple::Shell::addCommand(this, "xxx", DBModule::xxx);*/
    }
    void registerOptions();
    void showOptions() const;

public:
    static bool setup(bool liteMode = false);
    const std::string& name() const { return _name; }
};
}  // namespace db

extern db::Database database;

#endif
