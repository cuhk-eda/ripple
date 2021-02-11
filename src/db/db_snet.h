#ifndef _DB_SNET_H_
#define _DB_SNET_H_

namespace db {
class SNet : virtual public Drawable {
public:
    string name;
    vector<Geometry> shapes;
    vector<Via> vias;
    char type = 'x';

    SNet(const string& name) : name(name) {}

    template <class... Args>
    void addShape(Args&&... args) {
        shapes.emplace_back(args...);
    }

    template <class... Args>
    void addVia(Args&&... args) {
        vias.emplace_back(args...);
    }

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
    void draw(Visualizer* v, Layer& L) const;
};
}  // namespace db

#endif
