#ifndef _DB_CELL_H_
#define _DB_CELL_H_

#include "../sta/sta.h"

namespace db {
class CellType {
    friend class Database;

private:
    int _originX = 0;
    int _originY = 0;
    //  X=1  Y=2  R90=4  (can be combined)
    char _symmetry = 0;
    string _siteName = "";
    char _botPower = 'x';
    char _topPower = 'x';
    vector<Geometry> _obs;

    int _libcell = -1;

public:
    std::string name = "";
    char cls = 'x';
    bool stdcell = false;
    int width = 0;
    int height = 0;
    vector<PinType*> pins;
    int edgetypeL = 0;
    int edgetypeR = 0;
    int usedCount = 0;

    CellType(const string& name, int libcell) : _libcell(libcell), name(name) {}
    ~CellType();

    PinType* addPin(const string& name, const char direction, const char type);
    void addPin(PinType& pintype);

    template <class... Args>
    void addObs(Args&&... args) {
        _obs.emplace_back(args...);
    }

    PinType* getPin(string& name);

    int originX() const { return _originX; }
    int originY() const { return _originY; }
    char symmetry() const { return _symmetry; }
    char botPower() const { return _botPower; }
    char topPower() const { return _topPower; }
    const std::vector<Geometry>& obs() const { return _obs; }
    int libcell() const { return _libcell; }

    void setOrigin(int x, int y);
    void setXSymmetry() { _symmetry &= 1; }
    void setYSymmetry() { _symmetry &= 2; }
    void set90Symmetry() { _symmetry &= 4; }
    void siteName(const std::string& name) { _siteName = name; }

    bool operator==(const CellType& r) const;
    bool operator!=(const CellType& r) const { return !(*this == r); }
};

class Cell : virtual public Drawable {
private:
    string _name = "";
    int _spaceL = 0;
    int _spaceR = 0;
    int _spaceB = 0;
    int _spaceT = 0;
    bool _fixed = false;
    CellType* _type = nullptr;
    std::vector<Pin*> _pins;

public:
    bool highlighted = false;
    Region* region = nullptr;

    Cell(const string& name = "", CellType* t = nullptr) : _name(name) { ctype(t); }
    ~Cell();

    const std::string& name() const { return _name; }
    Pin* pin(const std::string& name) const;
    Pin* pin(unsigned i) const { return _pins[i]; }
    CellType* ctype() const { return _type; }
    void ctype(CellType* t);
    int lx() const;
    int ly() const;
    int hx() const { return lx() + width(); }
    int hy() const { return ly() + height(); }
    int cx() const { return lx() + width() / 2; }
    int cy() const { return ly() + height() / 2; }
    bool flipX() const;
    bool flipY() const;
    int width() const { return _type->width + _spaceL + _spaceR; }
    int height() const { return _type->height + _spaceB + _spaceT; }
    int siteWidth() const;
    int siteHeight() const;
    bool fixed() const { return _fixed; }
    void fixed(bool fix) { _fixed = fix; }
    bool placed() const;
    void place(int x, int y);
    void place(int x, int y, bool flipX, bool flipY);
    void unplace();
    unsigned numPins() const { return _pins.size(); }

    /*Drawable*/
    int globalX() const;
    int globalY() const;
    int localX() const;
    int localY() const;
    int boundL() const;
    int boundR() const;
    int boundB() const;
    int boundT() const;

    Drawable* parent() const;
    void draw(Visualizer* v) const;

    friend ostream& operator<<(ostream& os, const Cell& c) {
        return os << c._name << "\t(" << c.lx() << ", " << c.ly() << ')';
    }
};
}  // namespace db

#endif
