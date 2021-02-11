#ifndef _DB_VIA_H_
#define _DB_VIA_H_

#include <set>

namespace db {
class Via : virtual public Drawable {
public:
    int x;
    int y;
    ViaType* type;
    Via(ViaType* type, int x, int y) : x(x), y(y), type(type) {}

    int globalX() const;
    int globalY() const;
    int localX() const;
    int localY() const;
    int boundL() const;
    int boundR() const;
    int boundB() const;
    int boundT() const;

    Drawable* parent() const;
    void draw(Visualizer* v, Layer& L) const;
};

class ViaType {
private:
    bool isDef_ = false;

public:
    string name = "";
    set<Geometry> rects;

    ViaType(const string& name = "", const bool isDef = false) : isDef_(isDef), name(name) {}

    template <class... Args>
    void addRect(Args&&... args) {
        rects.emplace(args...);
    }
    void isDef(bool b) { isDef_ = b; }
    bool isDef() const { return isDef_; }
};
}  // namespace db

#endif
