#ifndef _DP_DATA_H_
#define _DP_DATA_H_

#include <atomic>
#include <future>
#include <mutex>
#include <shared_mutex>

#include "../concurrentqueue/concurrentqueue.h"
#include "../db/db.h"

using namespace moodycamel;

#define DEBUG_ID -1
//  histogram for max displacement
#define MAINTAIN_HIST

//  #define OPTIMAL_REGION_SIZE_LIMIT
//  #define OPTIMAL_REGION_SIZE_X_LIMIT 80
//  #define OPTIMAL_REGION_SIZE_Y_LIMIT 6
#define DISTANCE_THRESHOLD 0
//  #define OPTIMAL_REGION_COVER_FIX
//  #define LEGALIZE_REGION_SHIFT_TOWARDS_OPTIMAL_REGION

//  #define NEW_PLACEL_PLACER
#ifdef NEW_PLACEL_PLACER
//	#define NEW_PLACEL_PLACER_USE_CELL_LIST
#define NEW_PLACEL_PLACER_USE_SORTED_CELLS
#endif

#define MLL_CONSIDER_N_NEIGHBOR_CELLS
#define MLL_FIND_REAL_CRITICAL_POINTS
//  #define MLL_FIND_DISPLACEMENT_CURVE
//  #define MLL_VERIFY_ESTIMATION

#define ILP_TIMELIMIT 1

#define MLL_WEIGHTED_DISPLACEMENT
#define MLLCost double
constexpr MLLCost MLLCostInf =
    numeric_limits<MLLCost>::has_infinity ? numeric_limits<MLLCost>::infinity() : numeric_limits<MLLCost>::max();
//  Local region width = W * 2 + targetW
//  Local region height = H * 2 + targetH
#define MLLLocalRegionW 30
#define MLLLocalRegionH 2

namespace dp {
// typedef int MLLCost ;

class Region;
class Cell;
class Net;
class Pin;
class Segment;

class LocalCell;
class LocalSegment;
class LocalRegion;

template <class Score>
class PinAccess;
template <class Score>
class CellTypeMap;

class DPlacer;

class Region {
private:
    bool _isMax = false;

public:
    int lx = INT_MAX;
    int ly = INT_MAX;
    int hx = INT_MIN;
    int hy = INT_MIN;

    Region(int lx = INT_MAX, int ly = INT_MAX, int hx = INT_MIN, int hy = INT_MIN) : lx(lx), ly(ly), hx(hx), hy(hy) {}

    bool isMax() const { return _isMax; }
    int cx() const { return (hx + lx) / 2; }
    int cy() const { return (hy + ly) / 2; }
    void clear() {
        lx = ly = INT_MAX;
        hx = hy = INT_MIN;
    }
    bool empty() const { return lx == INT_MAX; }
    void expand(const int l, const int r, const int b, const int t, const db::Region* reg = nullptr);
    void shift(int dx, int dy);
    void cover(const int x, const int y);
    void constraint(const int lx, const int ly, const int hx, const int hy);
    void constraint(const Region& region) { constraint(region.lx, region.ly, region.hx, region.hy); }
    bool contains(const int x, const int y) const { return x >= lx && x <= hx && y >= ly && y <= hy; }
    bool contains(const int x, const int y, const int w, const int h) const {
        return x >= lx && x + w <= hx && y >= ly && y <= hy;
    }
    int area() const { return (hx - lx) * (hy - ly); }
    int width() const { return hx - lx; }
    int height() const { return hy - ly; }
    int distance(const int x, const int y);

    template <class Container>
    bool independent(const Container& c) const;

    friend ostream& operator<<(ostream& os, const Region& r) {
        return os << '(' << r.lx << ", " << r.ly << ") (" << r.hx << ", " << r.hy << ')';
    }
};

template <class Container>
bool Region::independent(const Container& c) const {
    for (const auto& [cell, r] : c) {
        if (!cell) {
            printlog(LOG_INFO, "No cell associated with region ( %d %d ) ( %d %d )", r.lx, r.ly, r.hx, r.hy);
        }
        if (r.lx <= hx && r.hx >= lx && r.ly <= hy && r.hy >= ly) {
            return false;
        }
    }
    return true;
}

/* data structure for global view */
class Cell {
private:
    static const char OddFlip = 1;
    static const char OddNoFlip = 2;
    static const char EvenFlip = 4;
    static const char EvenNoFlip = 8;
    atomic<int> _x = {-1};
    atomic<int> _y = {-1};
    //  horizontal flipping a.k.a. flipping by y axis
    atomic<char> _fx = {0};
    //  vertical flipping a.k.a. flipping by x axis
    char _fy = 0;
    // bit pattern showing if flip is required on odd/even rows
    char _row = 0;
    // input position
    float _ox = 0;
    float _oy = 0;
    // target position
    float _tx = 0;
    float _ty = 0;

    // data for premove
    int _pm_dy = 0;

    db::Cell* _dbCell = nullptr;

    static vector<vector<int>> _spaceLUT;
    static vector<unsigned> _spaceMax;

    static int getSpace(const int lType, const char lFx, const int rType, const char rFx);
    static int getSpaceMax(const int type);

public:
    // index in framework structure
    int id = -1;
    // index in DP structure
    int i = -1;
    // in unit of sites
    unsigned w = 0;
    unsigned h = 0;
    // fence id
    int fid = -1;
    // cell type
    unsigned type = 0;
    // 0 if no overlap
    char overlap = 0;
    // true if placed
    bool placed = false;

    // temp data for MLL
    int mll_ox;
    int mll_oy;
    int mll_x;
    int mll_y;
    bool mll_fixed;
    vector<LocalSegment*> mll_segments;
    /////////////////////

    vector<Pin*> pins;          // cell pins
    vector<Segment*> segments;  // overlapping segment for each row

    Cell(db::Cell* dbCell, int dbSiteW, int dbSiteH, int dbLX, int dbLY, char row)
        : _x((dbCell->lx() - dbLX + dbSiteW / 2) / dbSiteW),
          _y((dbCell->ly() - dbLY + dbSiteH / 2) / dbSiteH),
          _fx(dbCell->flipX() ? 1 : 0),
          _fy(dbCell->flipY() ? 1 : 0),
          _row(row),
          _ox((dbCell->lx() - dbLX) / static_cast<double>(dbSiteW)),
          _oy((dbCell->ly() - dbLY) / static_cast<double>(dbSiteH)),
          _tx((dbCell->lx() - dbLX) / static_cast<double>(dbSiteW)),
          _ty((dbCell->ly() - dbLY) / static_cast<double>(dbSiteH)),
          _dbCell(dbCell),
          w(dbCell->width() / dbSiteW),
          h(dbCell->height() / dbSiteH),
          fid((dbCell->region) ? ((int)dbCell->region->id) : (-1)),
          segments(h, nullptr) {}

    float ox() const { return _ox; }
    float oy() const { return _oy; }
    long oxLong() const { return lround(_ox); }
    long oyLong() const { return lround(_oy); }
    float tx() const { return _tx; }
    float ty() const { return _ty; }
    long txLong() const { return lround(_tx); }
    long tyLong() const { return lround(_ty); }

    int cx() const { return _x.load(memory_order_acquire) + w / 2; }
    int cy() const { return _y.load(memory_order_acquire) + h / 2; }
    int lx() const { return _x.load(memory_order_acquire); }
    int ly() const { return _y.load(memory_order_acquire); }
    int hx() const { return _x.load(memory_order_acquire) + w; }
    int hy() const { return _y.load(memory_order_acquire) + h; }
    bool fx() const { return _fx.load(memory_order_acquire) != 0; }
    bool fy() const { return _fy; }
    int pmDy() const { return _pm_dy; }
    void cx(const int x) { _x.store(x - w / 2, memory_order_release); }
    void cy(const int y) { _y.store(y - h / 2, memory_order_release); }
    void lx(const int x) { _x.store(x, memory_order_release); }
    void ly(const int y) { _y.store(y, memory_order_release); }
    void hx(const int x) { _x.store(x - w, memory_order_release); }
    void hy(const int y) { _y.store(y - h, memory_order_release); }
    void fx(const bool f) { _fx.store(f ? 1 : 0, memory_order_release); }
    void fy(const bool f) { _fy = f ? 1 : 0; }

    void tx(const float x) { _tx = x; }
    void ty(const float y) { _ty = y; }

    void pmDy(int dy) { _pm_dy = dy; }
    // for y-th row: 'x' = no placement 'y' = need flipping 'n' = no flipping 'a' = both flipping
    char rowFlip(int y) {
        if (y % 2) {                            // odd row
            if ((_row & OddFlip) == 0) {        // not flip
                if ((_row & OddNoFlip) == 0) {  // not no flip
                    return 'x';
                } else {
                    return 'n';
                }
            } else {
                if ((_row & OddNoFlip) == 0) {  // not no flip
                    return 'y';
                } else {
                    return 'a';
                }
            }
        } else {
            if ((_row & EvenFlip) == 0) {
                if ((_row & EvenNoFlip) == 0) {
                    return 'x';
                } else {
                    return 'n';
                }
            } else {
                if ((_row & EvenNoFlip) == 0) {
                    return 'y';
                } else {
                    return 'a';
                }
            }
        }
    }
    char rowPlace() const {
        const bool oddOK = ((_row & OddFlip) || (_row & OddNoFlip));
        const bool evenOK = ((_row & EvenFlip) || (_row & EvenNoFlip));
        if (oddOK && evenOK) {
            return 'a';
        } else if (oddOK) {
            return 'o';
        } else if (evenOK) {
            return 'e';
        } else {
            return 'x';
        }
    }
    void setOddRowFlip() { _row = (_row | OddFlip); }
    void setOddRowNoFlip() { _row = (_row | OddNoFlip); }
    void setEvenRowFlip() { _row = (_row | EvenFlip); }
    void setEvenRowNoFlip() { _row = (_row | EvenNoFlip); }
    db::Cell* getDBCell() const { return _dbCell; }
    db::CellType* getDBCellType() const { return _dbCell->ctype(); }
    float displacementX() const { return abs(_x.load(memory_order_acquire) - _ox); }
    float displacementY() const { return abs(_y.load(memory_order_acquire) - _oy); }
    void getLocalRegion(Region& region, const int rangeX, const int rangeY) const;
    void getOriginalRegion(Region& region, const int rangeX, const int rangeY) const;
    void getOptimalRegion(Region& region) const;
    void getImproveRegion(Region& region, Region& optimalRegion) const;

    Region getLocalRegion(const int rangeX, const int rangeY) const;
    Region getOriginalRegion(const int rangeX, const int rangeY) const;
    Region getOptimalRegion() const;
    Region getImproveRegion() const;
    Region getImproveRegion(Region& optimalRegion) const;

    static int getSpace(const Cell* lCell, const Cell* rCell) {
        return getSpace(lCell->type, lCell->_fx, rCell->type, rCell->_fx);
    }
    int getSpaceMax() const { return getSpaceMax(type); }
    static void setSpace(const vector<db::CellType*>& dbCellTypes);

    friend ostream& operator<<(ostream& os, const Cell& cell) {
        return os << cell.id << "\t(" << cell._ox << ", " << cell._oy << ")\t(" << cell.lx() << ", " << cell.ly()
                  << ')';
    }
};

class Pin {
public:
    // cell pin: offset on cell, otherwise offset on core
    int x = 0;
    int y = 0;
    Cell* cell = nullptr;
    Net* net = nullptr;
    static int maxCellPinRIndex;

    Pin(const int x = 0, const int y = 0, Net* net = nullptr, Cell* cell = nullptr)
        : x(x), y(y), cell(cell), net(net) {}
    int pinX() const;
    int pinY() const;
};

class Net {
public:
    Region box;
    int i;  // index in DP structure
    vector<Pin*> pins;

    Net();
    void update();
    long long hpwl() const;
};

typedef vector<Cell*> CellListType;
typedef vector<Cell*>::iterator CellListIter;
class CellList {
private:
    CellListType cells;
    void insert(CellListIter iter, Cell* cell) { cells.insert(iter, cell); }
    void remove(Cell* cell) {
        CellListIter c = find(cells.begin(), cells.end(), cell);
        if (c != cells.end()) {
            cells.erase(c);
        }
    }

public:
    CellListIter begin() { return cells.begin(); }
    CellListIter end() { return cells.end(); }
    Cell* get(int i) const { return cells[i]; }
    Cell* getCell(CellListIter iter) const { return *iter; }

    size_t size() const { return cells.size(); }
    void clear() { cells.clear(); }
    void getCellAt(int x, int y, Cell*& cellL, Cell*& cellR, CellListIter& iterL, CellListIter& iterR) {
        unsigned nCells = size();
        if (nCells == 0) {
            // no cell
            cellL = nullptr;
            cellR = nullptr;
            iterL = cells.end();
            iterR = cells.end();
            return;
        }

        if (cells[0]->cx() > x) {
            // all cells at the right
            cellL = nullptr;
            cellR = cells[0];
            iterL = cells.end();
            iterR = cells.begin();
            return;
        }

        if (cells[nCells - 1]->cx() <= x) {
            // all cells at the left
            cellL = cells[nCells - 1];
            cellR = nullptr;
            iterL = cells.end() - 1;
            iterR = cells.end();
            return;
        }

        // binary search
        /*
                iterL = upper_bound(cells.begin(), cells.end(), x, Cell::CompareXCell);
                iterR = iterL--;
                cellL = *iterL;
                cellR = *iterR;
                */
        int lo = 0;
        int hi = nCells - 1;
        while (lo != hi) {
            int i = (hi + lo) / 2;
            cellL = cells[i];
            cellR = cells[i + 1];

            if (cellL->cx() > x) {
                hi = i;
            } else if (cellR->cx() <= x) {
                lo = i;
            } else {
                assert(cellL->cx() <= x && cellR->cx() > x);
                iterL = cells.begin() + i;
                iterR = cells.begin() + (i + 1);
                return;
            }
        }
    }

    void addCell(Cell* cell) {
        Cell* cellL = nullptr;
        Cell* cellR = nullptr;
        CellListIter iterL;
        CellListIter iterR;
        getCellAt(cell->lx(), cell->ly(), cellL, cellR, iterL, iterR);
        insert(iterR, cell);
    }
    void removeCell(Cell* cell) { remove(cell); }
};

class Segment {
public:
    int i;  // index in a row
    int x, y;
    unsigned w;
    // int space;
    int fid;
    bool flip;
    CellList cells;
    mutable shared_mutex mtx;

    Segment(int i = 0, int x = 0, int y = 0, unsigned w = 0);
    Segment(const Segment& s);
    int boundL() const { return x; }
    int boundR() const { return x + w; }
    void addCell(Cell* cell);
    void removeCell(Cell* cell);
    void getCellAt(int x, int y, Cell*& cellL, Cell*& cellR, CellListIter& iterL, CellListIter& iterR) {
        cells.getCellAt(x, y, cellL, cellR, iterL, iterR);
    }
    unsigned setLocalRegion(LocalRegion& localRegion, unsigned& nLocalCells, const int fid);
};

/* data structure for MLL */
class LegalMoveIntervalEndpoint;
class LegalMoveInterval;
class DisplacementSubcurve;
class DisplacementCurve;
class LegalMoveIntervalEndpoint {
public:
    char side;
    int pos;
    const LocalCell* cell;
    LegalMoveInterval* interval;

    LegalMoveIntervalEndpoint(const char s) : side(s) {}
};

class LegalMoveInterval {
private:
    float _optPointL;  // right-boundary of the left cell
    float _optPointR;  // left-boundary of the right cell

public:
    int r = 0;
    int i = 0;
    LegalMoveIntervalEndpoint L{'L'};
    LegalMoveIntervalEndpoint R{'R'};

    LegalMoveInterval(const int row = 0) : r(row) {}

    float optPointL() const { return _optPointL; }
    float optPointR() const { return _optPointR; }

    void optPointL(const float l) { _optPointL = l; }
    void optPointR(const float r) { _optPointR = r; }

    static bool isInvalid(const LegalMoveInterval& interval) { return interval.L.pos > interval.R.pos; }
};

class DisplacementSubcurve {
public:
    unsigned index;
    int critical;
    int optimal;
    char direction;
    DisplacementSubcurve() {
        index = INT_MAX;
        critical = INT_MAX;
        optimal = INT_MAX;
        direction = 'R';
    }
    DisplacementSubcurve(unsigned i, int c, int o, char d) {
        index = i;
        critical = c;
        optimal = o;
        direction = d;
    }
};
class DisplacementCurve {
public:
    vector<DisplacementSubcurve> curves;
    void addCurve(unsigned i, int c, int o, char d) { curves.push_back(DisplacementSubcurve(i, c, o, d)); }
    void print() {
        for (unsigned i = 0; i != curves.size(); i++) {
            const DisplacementSubcurve& curve = curves[i];
            printlog(
                LOG_INFO, "%d : i=%u c=%d o=%d d=%c", i, curve.index, curve.critical, curve.optimal, curve.direction);
        }
    }
};

/* data structure for local region*/
class LocalCell {
private:
    float _ox = -1;
    float _oy = -1;

public:
    Cell* parent = nullptr;
    int i = -1;
    int x = -1;
    int y = -1;
    int w = 0;
    unsigned h = 0;
    char row = 'a';
    bool fixed = false;
    double weight = 1;
    vector<unsigned> edgespaceL;
    vector<unsigned> edgespaceR;

    LocalCell() = default;
    LocalCell(Cell* cell);
    LocalCell(const int ix,
              const int iy,
              const int iw,
              const unsigned ih,
              const db::CellType* type = nullptr,
              const unsigned l = 0,
              const unsigned r = 0);

    float ox() const { return _ox; }
    float oy() const { return _oy; }
    long oxLong() const { return lround(_ox); }
    long oyLong() const { return lround(_oy); }

    void ox(const float x) { _ox = x; }
    void oy(const float y) { _oy = y; }

    friend ostream& operator<<(ostream& os, const LocalCell& lc) {
        return os << lc.i << "\t(" << lc.x << ", " << lc.y << ')';
    }
};

class LocalSegment {
public:
    int x, y, w;
    Segment* parent = nullptr;
    vector<int> localCells;
    unsigned nCells = 0;
    LocalSegment() {}
    LocalSegment(Segment* segment, int lx, int hx);
};

class LocalRegion {
private:
    bool _isMax = false;

public:
    int useCount = 0;
    int lx = -1;
    int ly = -1;
    int hx = -1;
    int hy = -1;
    vector<LocalSegment> localSegments;
    vector<LocalCell> localCells;
    vector<int> localCellMap;
    vector<int> localCellList;
    unsigned nSegments;
    unsigned nCells;

    LocalRegion(size_t localCellMapSize) : localCellMap(localCellMapSize, -1) {}

    void isMax(bool b) { _isMax = b; }

    void reset();
    void clear();
    void placeL(vector<int>& posL, const vector<CellTypeMap<MLLCost>>& cellTypeMaps);
    void placeR(vector<int>& posR, const vector<CellTypeMap<MLLCost>>& cellTypeMaps);
    void placeSpreadL(int targetCell, vector<int>& posL, const vector<CellTypeMap<MLLCost>>& cellTypeMaps);
    void placeSpreadR(int targetCell, vector<int>& posR, const vector<CellTypeMap<MLLCost>>& cellTypeMaps);

    void estimate(vector<int>& posL,
                  vector<int>& posR,
                  const vector<vector<LegalMoveInterval>>& intervals,
                  const vector<int>& insertionPoint,
                  int targetX,
                  Cell* targetCell,
                  const vector<CellTypeMap<MLLCost>>& cellTypeMaps);
    void estimateL(vector<int>& posL,
                   const vector<vector<LegalMoveInterval>>& intervals,
                   const vector<int>& insertionPoint,
                   int targetX,
                   Cell* targetCell,
                   const vector<CellTypeMap<MLLCost>>& cellTypeMaps);
    void estimateR(vector<int>& posR,
                   const vector<vector<LegalMoveInterval>>& intervals,
                   const vector<int>& insertionPoint,
                   int targetX,
                   Cell* targetCell,
                   const vector<CellTypeMap<MLLCost>>& cellTypeMaps);

    void draw(string filename, int targetCell = -1);
    bool isValid(int x, int y, int w, int h);
    bool verify(std::string title);
    bool checkOverlap();
    void report();
};

class CriticalPoint {
public:
    int x;
    int lSlope;
    int rSlope;
    CriticalPoint(int xx, int l, int r) : x(xx), lSlope(l), rSlope(r) {}
    CriticalPoint() {}
};

class CellMove {
private:
    float _x = 0;
    float _y = 0;
    int _orient = -1;

public:
    Cell* cell = nullptr;

    CellMove() {}
    CellMove(Cell* cell, const float x, const float y, const int orient = -1)
        : _x(x), _y(y), _orient(orient), cell(cell) {}

    float x() const { return _x; }
    float y() const { return _y; }
    long xLong() const { return lround(_x); }
    long yLong() const { return lround(_y); }
    bool hasOrient() const { return _orient >= 0; }
    bool isFlipX() const;
    bool isFlipY() const;

    void x(const float f) { _x = f; }
    void y(const float f) { _y = f; }

    friend ostream& operator<<(ostream& os, const CellMove& cm) {
        return os << cm.cell->id << "\t(" << cm._x << ", " << cm._y << ")";
    }
};

class Move {
public:
    CellMove target;
    vector<CellMove> legals;

    Move() {}
    Move(Cell* cell, const float x, const float y, const int orient = -1);
    Move(const CellMove& cellMove) : target(cellMove) {}
    long long displacementFromInput(int targetWeight = 1);
    long long displacement(int targetWeight = 1);
    Move inverse();

    friend ostream& operator<<(ostream& os, const Move& move) {
        os << move.target;
        for (const CellMove& cm : move.legals) {
            os << endl << cm;
        }
        return os;
    }
};
class Density {
private:
    int lx, ly, hx, hy;
    int nBinsX, nBinsY;
    int nRegions;
    int binW, binH;
    vector<vector<vector<int>>> binCellArea;
    vector<vector<vector<int>>> binFreeArea;

public:
    vector<vector<double>> targetDensity;
    void setup(int coreLX, int coreLY, int coreHX, int coreHY, int w, int h, int nRegs) {
        lx = coreLX;
        ly = coreLY;
        hx = coreHX;
        hy = coreHY;
        binW = w;
        binH = h;
        nRegions = nRegs;
        nBinsX = (hx - lx - 1) / w + 1;
        nBinsY = (hy - ly - 1) / h + 1;

        binCellArea.resize(nRegions, vector<vector<int>>(nBinsX, vector<int>(nBinsY, 0)));
        binFreeArea.resize(nRegions, vector<vector<int>>(nBinsX, vector<int>(nBinsY, 0)));
        targetDensity.resize(nBinsX, vector<double>(nBinsY, 1.0));

#ifndef NDEBUG
        printlog(LOG_INFO, "density map = %d x %d", nBinsX, nBinsY);
#endif
    }
    int getBinBoundL(const int bx) const { return bx * binW + lx; }
    int getBinBoundR(const int bx) const { return (bx + 1) * binW + lx; }
    int getBinBoundB(const int by) const { return by * binH + ly; }
    int getBinBoundT(const int by) const { return (by + 1) * binH + ly; }
    int getBinX(const int x) const {
        assert(x >= lx && x < hx);
        return (x - lx) / binW;
    }
    int getBinY(const int y) const {
        assert(y >= ly && y < hy);
        return (y - ly) / binH;
    }
    int getPosX(const int bx) const { return getBinBoundL(bx) + binW / 2; }
    int getPosY(const int by) const { return getBinBoundB(by) + binH / 2; }
    void addSpace(int fid, int x, int y, int w, int h) {
        for (int sy = y; sy < y + h; sy++) {
            int by = getBinY(sy);
            for (int sx = x; sx < x + w; sx++) {
                int bx = getBinX(sx);
                binFreeArea[fid][bx][by]++;
            }
        }
    }
    vector<int> spiral_x;
    vector<int> spiral_y;
    void initSpiralSequence(int level) {
        spiral_x.clear();
        spiral_y.clear();
        int x = 0, y = 0;
        for (int L = 0; L < level; L++) {
            for (; x < 0; x++, y++) {
                spiral_x.push_back(x);
                spiral_y.push_back(y);
            }
            for (; y > 0; x++, y--) {
                spiral_x.push_back(x);
                spiral_y.push_back(y);
            }
            for (; x > 0; x--, y--) {
                spiral_x.push_back(x);
                spiral_y.push_back(y);
            }
            for (; y < 0; x--, y++) {
                spiral_x.push_back(x);
                spiral_y.push_back(y);
            }
            spiral_x.push_back(x);
            spiral_y.push_back(y);
            y++;
        }
    }
    void getVicinityLowDensityLocation(int& x, int& y, int ox, int oy, int distance, int fid = -1) {
        if (spiral_x.size() == 0) {
            initSpiralSequence(30);
        }
        int obx = getBinX(ox);
        int oby = getBinY(oy);
        int nSpiralEntries = spiral_x.size();
        double minDensity = 1.1;
        for (int i = 0; i < nSpiralEntries; i++) {
            int ibx = obx + spiral_x[i];
            int iby = oby + spiral_y[i];
            if (ibx < 0 || ibx >= nBinsX || iby < 0 || iby >= nBinsY) {
                continue;
            }
            if (abs(spiral_x[i]) + abs(spiral_y[i]) > distance) {
                break;
            }
            double density = getBinDensity(ibx, iby, fid);
            if (density < minDensity) {
                minDensity = density;
                x = getPosX(ibx);
                y = getPosY(iby);
            }
        }
    }
    void addCell(Cell* cell) { addCell(cell->fid, cell->lx(), cell->ly(), cell->w, cell->h); }
    void addCell(int fid, int x, int y, int w, int h) {
        for (int sy = y; sy < y + h; sy++) {
            int by = getBinY(sy);
            for (int sx = x; sx < x + w; sx++) {
                int bx = getBinX(sx);
                binCellArea[fid][bx][by]++;
            }
        }
    }
    void removeCell(Cell* cell) { removeCell(cell->fid, cell->lx(), cell->ly(), cell->w, cell->h); }
    void removeCell(int fid, int x, int y, int w, int h) {
        for (int sy = y; sy < y + h; sy++) {
            int by = getBinY(sy);
            for (int sx = x; sx < x + w; sx++) {
                int bx = getBinX(sx);
                binCellArea[fid][bx][by]--;
            }
        }
    }
    double getBinDensity(int bx, int by, int fid = -1) {
        if (nRegions == 1) {
            fid = 0;
        }
        int cellArea = 0;
        int freeArea = 0;
        if (fid < 0) {
            for (int i = 0; i < nRegions; i++) {
                cellArea += binCellArea[i][bx][by];
                freeArea += binFreeArea[i][bx][by];
            }
        } else {
            cellArea = binCellArea[fid][bx][by];
            freeArea = binFreeArea[fid][bx][by];
        }
        // printlog(LOG_INFO, "density (%d,%d) : %d / %d = %lf", bx, by, cellArea, freeArea, (double)cellArea /
        // (double)freeArea);
        if (freeArea == 0) {
            return 1.0;
        }
        return (double)cellArea / (double)freeArea;
    }
    double getDensity(int x, int y, int fid = -1) {
        int bx = getBinX(x);
        int by = getBinY(y);
        return getBinDensity(bx, by, fid);
    }

    double getOverFlow(const vector<unsigned>& cellArea, const double densityLimit, int fid = -1) const;
    void draw(string filename);
};

class DPStage {
public:
    enum Technique {
        None = 0,
        Init,
        Eval,
        Premove,
        Legalize,
        ChainMove,
        GlobalMove,
        GlobalMoveForPinAccess,
        LocalMove,
        LocalMoveForPinAccess,
        GlobalNF
    };

private:
    Technique _technique = Technique::None;
    string _name;
    int _maxIter = 0;
    int _runIter = 0;
    vector<long long> _iterHPWL;
    vector<long long> _iterDBDisp;
    vector<vector<double>> _iterDBMean;
    vector<unsigned long long> _iterDBMax;
    vector<const Cell*> _iterDBMaxCell;
    vector<int> _overlaps;
    vector<int> _unplaced;
    vector<int> _overlapSum;
    vector<int> _pinAccess;

public:
    DPStage() {}
    DPStage(std::string name, Technique technique, int maxIter)
        : _technique(technique),
          _name(name),
          _maxIter(maxIter),
          _iterHPWL(maxIter, -1),
          _iterDBDisp(maxIter, -1),
          _iterDBMean(maxIter, vector<double>(5, -1)),
          _iterDBMax(maxIter, -1),
          _iterDBMaxCell(maxIter, nullptr),
          _overlaps(maxIter, -1),
          _unplaced(maxIter, -1),
          _overlapSum(maxIter, -1),
          _pinAccess(maxIter, -1) {}

    const std::string& name() const { return _name; }
    Technique technique() const { return _technique; }
    unsigned maxIter() const { return _maxIter; }
    unsigned runIter() const { return _runIter; }
    long long hpwl(int iter = -1) const { return _iterHPWL[iter < 0 ? _runIter : iter]; }
    long long dbDisp(int iter = -1) const { return _iterDBDisp[iter < 0 ? _runIter : iter]; }
    const vector<double>& dbMean() const { return _iterDBMean[_runIter]; }
    unsigned long long dbMax(int iter = -1) const { return _iterDBMax[iter < 0 ? _runIter : iter]; }
    const Cell* dbMaxCell(int iter = -1) const { return _iterDBMaxCell[iter < 0 ? _runIter : iter]; }
    int overlap(int iter = -1) const { return _overlaps[iter < 0 ? _runIter : iter]; }
    int unplaced(int iter = -1) const { return _unplaced[iter < 0 ? _runIter : iter]; }
    int overlapSum(int iter = -1) const { return _overlapSum[iter < 0 ? _runIter : iter]; }
    int pinAccess(int iter = -1) const { return _pinAccess[iter < 0 ? _runIter : iter]; }
    void update(int iter = -1);
};

class DPFlow {
public:
    std::vector<DPStage> _stages;

    unsigned _stage = 0;
    unsigned _iter = 0;

    DPFlow() { _stages.emplace_back("  ", DPStage::Technique::Init, 1); }
    void addStage(string name, DPStage::Technique technique, int maxIter) {
        _stages.push_back(DPStage(name, technique, maxIter));
    }
    unsigned stage() { return _stage; }
    unsigned iter() { return _iter; }
    bool nextStage() {
        if (_stage + 1 >= _stages.size()) {
            return false;
        }
        _stage++;
        _iter = 0;
        return true;
    }
    bool nextIter() {
        if (_iter + 1 >= _stages[_stage].maxIter()) {
            return false;
        }
        _iter++;
        return true;
    }
    bool next() {
        if (nextIter()) {
            return true;
        }
        if (nextStage()) {
            return true;
        }
        return false;
    }
    DPStage::Technique technique() { return _stages[_stage].technique(); }
    long long hpwl(int stage = -1, int iter = -1) const {
        stage = (stage < 0 ? _stage : stage);
        return _stages[stage].hpwl(iter);
    }

    long long dbDisp(int stage = -1, int iter = -1) const {
        stage = (stage < 0 ? _stage : stage);
        return _stages[stage].dbDisp(iter);
    }

    const vector<double>& dbMean() const { return _stages[_stage].dbMean(); }

    unsigned long long dbMax(int stage = -1, int iter = -1) const {
        stage = (stage < 0 ? _stage : stage);
        return _stages[stage].dbMax(iter);
    }

    const Cell* dbMaxCell(int stage = -1, int iter = -1) const {
        stage = (stage < 0 ? _stage : stage);
        return _stages[stage].dbMaxCell(iter);
    }

    void getLastIter(int& lastStage, int& lastIter, int stage = -1, int iter = -1);
    double hpwlDiff(int cmpStage = -1, int cmpIter = -1, int stage = -1, int iter = -1);
    double dispDiff(int cmpStage = -1, int cmpIter = -1, int stage = -1, int iter = -1);
    int overlap(int iter = -1) { return _stages[_stage].overlap(iter); }
    int unplaced(int iter = -1) { return _stages[_stage].unplaced(iter); }
    int overlapSum(int iter = -1) { return _stages[_stage].overlapSum(iter); }
    int pinAccess(int iter = -1) { return _stages[_stage].pinAccess(iter); }
    const DPStage& stage(int stage = -1) const { return _stages[stage]; }
    void update(int stage = -1, int iter = -1);
    void report(int stage = -1, int iter = -1);
};

template <class Score>
class PinAccess {
private:
    unsigned char _size;
    unsigned char _fx;
    Score _score;
#ifndef NDEBUG
    vector<Score> _n;
#endif

public:
    PinAccess(unsigned char size = 0);

    bool fx() const { return _fx; }
    Score score() const { return _score; }

#ifndef NDEBUG
    //
    const vector<Score>& n() const { return _n; }
#endif
    //  const vector<unsigned char>& fs() const { return _fs; }

    //  void n(unsigned i, unsigned char c) { _n[i] = c; }
    //  void fs(unsigned i, unsigned char c) { _fs[i] = c; }

    void fx(const unsigned char f) { _fx = f ? 1 : 0; }
    void score(const Score s) { _score = s; }
    void accN(const unsigned i, const Score c);

    const PinAccess& operator+=(const PinAccess<Score>& r);
    const PinAccess operator+(const PinAccess<Score>& r) const;
};

template <class Score>
PinAccess<Score>::PinAccess(unsigned char size) : _size(size), _fx(0), _score(0) {
#ifndef NDEBUG
    _n.resize(size, 0);
#endif
}

template <class Score>
void PinAccess<Score>::accN(const unsigned i, const Score c) {
    _score += c;
#ifndef NDEBUG
    _n[i] += c;
#endif
}

template <class Score>
const PinAccess<Score>& PinAccess<Score>::operator+=(const PinAccess& r) {
    _score += r._score;
#ifndef NDEBUG
    for (unsigned i = 0; i != _size; ++i) {
        _n[i] += r._n[i];
    }
#endif
    return *this;
}

template <class Score>
const PinAccess<Score> PinAccess<Score>::operator+(const PinAccess& r) const {
    PinAccess pa(_size);
    pa._score = _score + r._score;
#ifndef NDEBUG
    for (unsigned i = 0; i != _size; ++i) {
        pa._n[i] = _n[i] + r._n[i];
    }
#endif
    return pa;
}

template <class Score>
class CellTypeMap {
private:
    unsigned _y = 0;
    unsigned _x = 0;
    unsigned _nLayers = 0;
    bool _lite = false;
    PinAccess<Score> _hSum;
    vector<PinAccess<Score>> _hRail;
    vector<PinAccess<Score>> _vRail;
    unordered_map<unsigned, PinAccess<Score>> _iopinMap;
    vector<vector<PinAccess<Score>>> _accessLUT;
    vector<vector<pair<unsigned, unsigned>>> _segments;

public:
    CellTypeMap(const unsigned y = 0, const unsigned x = 0, const unsigned nLayers = 0, const bool lite = false);
    void clear(const unsigned y = 0, const unsigned x = 0, const unsigned nLayers = 0, const bool lite = false);
    unsigned nLayers() const { return _nLayers; }
    bool lite() const { return _lite; }
    Score hSum() const { return _hSum.score(); }
    Score hScore(const unsigned y) const { return _hRail[y].score(); }
    Score vScore(const unsigned x) const { return _vRail[x].score(); }
    Score ioScore(const unsigned y, const unsigned x) const;
    bool fx(const unsigned y, const unsigned x) const;
    Score score(const unsigned y, const unsigned x) const;
    const pair<unsigned, unsigned>* getSegmentAt(const unsigned y, const unsigned x) const;

#ifndef NDEBUG
    const vector<Score> n(const unsigned y, const unsigned x) const;
#endif

    //
    void accH(const unsigned y, const unsigned i, const Score c) { _hRail[y].accN(i, c); }
    void accV(const unsigned x, const unsigned i, const Score c) { _vRail[x].accN(i, c); }
    void accIO(const unsigned y, const unsigned x, const unsigned i, const Score c);
    void update(CellTypeMap* fx = nullptr);
};

template <class Score>
CellTypeMap<Score>::CellTypeMap(const unsigned y, const unsigned x, const unsigned nLayers, const bool lite)
    : _y(y),
      _x(x),
      _nLayers(nLayers),
      _lite(lite),
      _hSum(nLayers),
      _hRail(y, PinAccess<Score>(nLayers)),
      _vRail(x, PinAccess<Score>(nLayers)),
      _iopinMap(),
      _segments(y) {
    if (!_lite) {
        _accessLUT.resize(y, vector<PinAccess<Score>>(x, PinAccess<Score>(nLayers)));
    }
}

template <class Score>
void CellTypeMap<Score>::clear(const unsigned y, const unsigned x, const unsigned nLayers, const bool lite) {
    _y = y;
    _x = x;
    _nLayers = nLayers;
    _lite = lite;
    _hSum = PinAccess<Score>(nLayers);
    _hRail.resize(y, PinAccess<Score>(nLayers));
    _vRail.resize(x, PinAccess<Score>(nLayers));
    _iopinMap.clear();
    if (!_lite) {
        _accessLUT.resize(y, vector<PinAccess<Score>>(x, PinAccess<Score>(nLayers)));
    }
    _segments.resize(y, vector<pair<unsigned, unsigned>>());
}

template <class Score>
bool CellTypeMap<Score>::fx(const unsigned y, const unsigned x) const {
    if (_lite) {
        return _vRail[x].fx();
    } else {
        return _accessLUT[y][x].fx();
    }
}

template <class Score>
Score CellTypeMap<Score>::ioScore(const unsigned y, const unsigned x) const {
    unsigned key = (y << 16) + x;
    typename unordered_map<unsigned, PinAccess<Score>>::const_iterator got = _iopinMap.find(key);
    if (got == _iopinMap.end()) {
        return 0;
    } else {
        return got->second.score();
    }
}

template <class Score>
Score CellTypeMap<Score>::score(const unsigned y, const unsigned x) const {
    if (_lite) {
        return _hRail[y].score() + _vRail[x].score() + ioScore(y, x);
    }
    return _accessLUT[y][x].score();
}

template <class Score>
const pair<unsigned, unsigned>* CellTypeMap<Score>::getSegmentAt(const unsigned y, const unsigned x) const {
    if (y >= _y) {
        return nullptr;
    }
    for (const pair<unsigned, unsigned>& seg : _segments[y]) {
        if (x < seg.first) {
            return nullptr;
        }
        if (x < seg.second) {
            return &seg;
        }
    }
    return nullptr;
}

#ifndef NDEBUG
template <class Score>
const vector<Score> CellTypeMap<Score>::n(const unsigned y, const unsigned x) const {
    if (!_lite) {
        return _accessLUT[y][x].n();
    }
    unsigned key = (y << 16) + x;
    typename unordered_map<unsigned, PinAccess<Score>>::const_iterator got = _iopinMap.find(key);
    if (got == _iopinMap.end()) {
        return (_hRail[y] + _vRail[x]).n();
    } else {
        return (_hRail[y] + _vRail[x] + got->second).n();
    }
}
#endif

template <class Score>
void CellTypeMap<Score>::accIO(const unsigned y, const unsigned x, const unsigned i, const Score c) {
    unsigned key = (y << 16) + x;
    typename unordered_map<unsigned, PinAccess<Score>>::iterator got = _iopinMap.find(key);
    if (got == _iopinMap.end()) {
        _iopinMap.emplace(key, _nLayers);
        _iopinMap[key].accN(i, c);
    } else {
        got->second.accN(i, c);
    }
}

template <class Score>
void CellTypeMap<Score>::update(CellTypeMap<Score>* fx) {
    if (!_lite) {
        for (const pair<unsigned, PinAccess<Score>>& p : _iopinMap) {
            unsigned j = (p.first & 0xFFFF0000) >> 16;
            unsigned k = p.first & 0xFFFF;
            _accessLUT[j][k] += p.second;
        }
        if (fx) {
            for (const pair<unsigned, PinAccess<Score>>& p : fx->_iopinMap) {
                unsigned j = (p.first & 0xFFFF0000) >> 16;
                unsigned k = p.first & 0xFFFF;
                fx->_accessLUT[j][k] += p.second;
            }
        }
    }
    for (unsigned j = 0; j != _x; ++j) {
        if (!_lite) {
            for (unsigned k = 0; k != _y; ++k) {
                _accessLUT[k][j] += _vRail[j];
                if (fx) {
                    fx->_accessLUT[k][j] += fx->_vRail[j];
                    if (_accessLUT[k][j].score() > fx->_accessLUT[k][j].score()) {
                        _accessLUT[k][j] = fx->_accessLUT[k][j];
                        _accessLUT[k][j].fx(true);
                    }
                }
                _accessLUT[k][j] += _hRail[k];
            }
        }
        if (fx && _vRail[j].score() > fx->_vRail[j].score()) {
            _vRail[j] = fx->_vRail[j];
            _vRail[j].fx(true);
        }
    }
    for (unsigned i = 0; i != _y; ++i) {
        _hSum += _hRail[i];
        unsigned w = 0;
        for (unsigned j = 0; j != _x; ++j) {
            if (!score(i, j)) {
                ++w;
                continue;
            }
            if (w) {
                _segments[i].emplace_back(j - w, j);
                w = 0;
            }
        }
        if (w) {
            _segments[i].emplace_back(_x - w, _x);
        }
    }
}

class DPlacer {
    friend class DPStage;
    friend class DPFlow;

private:
    static constexpr char OddFlip = 1;
    static constexpr char OddNoFlip = 2;
    static constexpr char EvenFlip = 4;
    static constexpr char EvenNoFlip = 8;

    bool multiRowMode;
    unordered_map<db::Cell*, Cell*> dbCellMap;

    vector<LocalRegion> localRegions;

    vector<CellTypeMap<MLLCost>> cellTypeMaps;
    vector<vector<db::GeoMap>> snetGeoMaps;
    vector<char> _row;

public:
    ConcurrentQueue<pair<Cell*, Region>>* tQ = nullptr;
    ConcurrentQueue<pair<Cell*, bool>>* rQ = nullptr;
    ProducerToken* tPtok = nullptr;
    vector<ProducerToken> rPtoks;

#ifdef MAINTAIN_HIST
    vector<unsigned> hist;
    unsigned maxDisp;
    vector<unsigned> oHist;
    unsigned oMaxDisp;
    mutable shared_mutex histMtx;
#endif

    vector<Cell*> cells;
    vector<Net*> nets;
    vector<Pin*> pins;
    vector<vector<Segment>> segments;
    vector<unsigned> counts;
    vector<unsigned> totalCellArea;
    vector<unsigned> totalFreeArea;

    Density density;

    int binW, binH;
    int siteW, siteH;
    int coreLX, coreLY;
    int coreHX, coreHY;
    int dbLX, dbLY;
    int dbHX, dbHY;
    unsigned nRegions = 0;
    unsigned nDPTypes = 0;
    vector<db::CellType*> dbCellTypes;

    DPFlow flow;

    DPlacer();
    ~DPlacer();

    void input();
    void place(const string& name = "iccad2017");
    void output();

    void setupFlow(const string& name);

    bool checkGeoMap(const db::GeoMap& geoMap, const int lx, const int ly, const int hx, const int hy) const;
    void getCellTypeMapFromBound(CellTypeMap<MLLCost>* cellTypeMapFx, const db::CellType* dbType, const unsigned i, const int lx, const int ly, const int hx, const int hy, const int rIndex);
    int getCellTypeMap(const vector<db::CellType*>& dbCellTypes, unsigned& index, mutex& indexMtx);
    void getAvail(
        const vector<vector<bool>>& hNR, vector<bool>& isSeg, unsigned maxH, unsigned spaceL, unsigned spaceR);

#ifndef NDEBUG
    void evalPin(vector<unsigned>& pin);
#endif

    Cell* addCell(db::Cell* cell, char row);
    Net* addNet();
    Pin* addPin();
    Segment* addSegment(const db::RowSegment& dbSegment, unsigned& s, unsigned r, int dpsx, unsigned dpsw);

    void drawLayout(string filename);
    void drawDisplacement(const string& filename, const Cell* targetCell = nullptr);
    void drawMove(string filename, Region region, vector<Region>& cellRegion);

private:
    bool getBinAt(int x, int y, int& bx, int& by);
    bool getCellAt(
        Cell* cell, int x, int y, Cell*& cellL, Cell*& cellR, CellListIter& iterL, CellListIter& iterR, Segment*& seg);
    void insertCell(Cell* cell);  // insert a new cell to the placement
    void removeCell(Cell* cell);  // remove a cell from the placement
    void moveCell(Cell* cell, int x, int y);
    void moveCell(Cell* cell, int x, int y, bool fx, bool fy);
    void shiftCell(Cell* cell, int x, int y);  // lite-weight version of moveCell

#ifdef MAINTAIN_HIST
    void updateHist(vector<unsigned>& hist, unsigned& maxDisp, Cell* cell, int x, int y);
#endif

    Segment* getSegmentAt(const Cell* cell, const int x, const int y);
    const Segment* getNearestSegment(Cell* cell, const int x, const int y);
    Segment* getNearestSegmentInRow(Cell* cell, int x, int y);

    double defineLocalRegion(LocalRegion& localRegion, Cell* cell, const Region& region);
    int getOptimalX(vector<CriticalPoint>& critiPtsInput,
                    const unsigned targetCellType,
                    const int rangeBoundL,
                    const int rangeBoundR) const;
    //  move exactly a cell, assume this cell is unplaced
    void doLegalMove(const Move& move, bool updateNet);
    bool isLegalCellMove(const CellMove& cellMove);
    bool isLegalMove(Move& move, Region region, int threshold = -1, double targetWeight = 1.0, unsigned iThread = 0);
    bool isLegalMoveSR(Move& move, Region& region, double targetWeight = 1.0);
    bool isLegalMoveMLL(Move& move, Region& region, double targetWeight = 1.0, unsigned iThread = 0);

    int eval();
    int premove();
    int legalize();
    unsigned legalizeCell(const int threshold, const unsigned iThread);
    unsigned chainMove();
    unsigned chainMoveCell(const unsigned fid, const unsigned type);
    unsigned localChainMove();
    unsigned localChainMoveCell(Cell* cell);
    unsigned globalMove();
    unsigned globalMoveCell(Cell* cell, int threshold, int rangeX, int rangeY);
    int globalMoveForPinAccess();
    int globalMoveForPinAccessCell(Cell* cell, int threshold, int rangeX, int rangeY);
    int localMove();
    int localVerticalMove(Cell* cell, int threshold, int rangeX, int rangeY);
    int localMoveForPinAccess();
    int localShiftForPinAccess(Cell* cell, int maxDisp);
    int localSwapForPinAccess(Segment& seg, int maxDisp);
    bool isBookshelfSymbol(unsigned char c);
    bool readBSLine(istream& is, vector<string>& tokens);
    bool readDefCell(string file, map<string, tuple<int, int, int>>& cell_list);

    unsigned minCostFlow();
    void localMinCostFlow(LocalRegion& localRegion);

    void updateNet(Move& move);
    long long getHPWLDiff(Move& move);
    double getScoreDiff(const Move& move);
    double getDensityDiff(const Move& move) const { return 0.0; }
    int checkOverlap(bool spaceAware = false);
    int getUnplaced() const;
    unsigned getOverlapSum() const;
    int getPinAccess() const;
    //  void getDisplacement(long long& sum, double& mean, long long& max, const Cell*& maxCell);
    void getDBDisplacement(long long& sum, vector<double>& mean, unsigned long long& max, const Cell*& maxCell);
    long long hpwl();
    MLLCost getPinAccessAndDispCost(Cell* cell, const int x, const int y) const;
    MLLCost getPinAccessAndDispCost(Cell* cell) const { return getPinAccessAndDispCost(cell, cell->lx(), cell->ly()); }
    MLLCost getPinAccessAndDispCost(const CellMove& cellMove) const {
        return getPinAccessAndDispCost(cellMove.cell, cellMove.xLong(), cellMove.yLong());
    }

    double convertUnit();

    // for y-th row: 'x' = no placement 'y' = need flipping 'n' = no flipping 'a' = both flipping
    char rowFlip(unsigned i, int y) const {
        if (y % 2) {                               // odd row
            if ((_row[i] & OddFlip) == 0) {        // not flip
                if ((_row[i] & OddNoFlip) == 0) {  // not no flip
                    return 'x';
                } else {
                    return 'n';
                }
            } else {
                if ((_row[i] & OddNoFlip) == 0) {  // not no flip
                    return 'y';
                } else {
                    return 'a';
                }
            }
        } else {
            if ((_row[i] & EvenFlip) == 0) {
                if ((_row[i] & EvenNoFlip) == 0) {
                    return 'x';
                } else {
                    return 'n';
                }
            } else {
                if ((_row[i] & EvenNoFlip) == 0) {
                    return 'y';
                } else {
                    return 'a';
                }
            }
        }
    }
    char rowPlace(unsigned i) {
        bool oddOK = ((_row[i] & OddFlip) || (_row[i] & OddNoFlip));
        bool evenOK = ((_row[i] & EvenFlip) || (_row[i] & EvenNoFlip));
        if (oddOK && evenOK) {
            return 'a';
        } else if (oddOK) {
            return 'o';
        } else if (evenOK) {
            return 'e';
        } else {
            return 'x';
        }
    }
    void setOddRowFlip(unsigned i) { _row[i] = (_row[i] | OddFlip); }
    void setOddRowNoFlip(unsigned i) { _row[i] = (_row[i] | OddNoFlip); }
    void setEvenRowFlip(unsigned i) { _row[i] = (_row[i] | EvenFlip); }
    void setEvenRowNoFlip(unsigned i) { _row[i] = (_row[i] | EvenNoFlip); }
};
};  // namespace dp

// extern dp::DPlacer dplacer;

#endif
