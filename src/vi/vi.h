#ifndef _VI_H_
#define _VI_H_

#include "draw.h"

namespace vi {
class Visualizer;

class Drawable {
protected:
    Drawable* _parent = nullptr;
    bool _selected = false;

public:
    Drawable() {}
    virtual ~Drawable() {}

    void setParent(Drawable* parent) { _parent = parent; }
    void select(bool s = true) { _selected = s; }
    bool isSelected();

    virtual Drawable* parent() const = 0;
    virtual int globalX() const = 0;
    virtual int globalY() const = 0;
    virtual int localX() const = 0;
    virtual int localY() const = 0;
    virtual int boundL() const = 0;
    virtual int boundR() const = 0;
    virtual int boundB() const = 0;
    virtual int boundT() const = 0;
};

class ColorScheme {
public:
    Color background;
    Color instanceFill;
    Color instanceLine;
    Color rowFill;
    Color rowLine;
    Color regionFill;
    vector<Color> metalFill;
    vector<Color> metalLine;
    vector<Color> viaFill;
    vector<Color> viaLine;

    ColorScheme() { setLight(); }
    void setDefault();
    void setLight();
};

class Visualizer {
private:
    bool ready = false;
    Canvas _canvas;
    int w = 0;
    int h = 0;
    int lx = 0;
    int ly = 0;
    int hx = 0;
    int hy = 0;

public:
    ColorScheme scheme;

    void setWindow(int w, int h);
    void setViewport(int lx, int ly, int hx, int hy);
    void setup();

    void save(string file) { _canvas.Save(file); }
    void reset();
    void reset(Color color);

    void setLineColor(Color color);
    void setFillColor(Color color);
    void setLineWidth(int size);

    void drawPoint(int x, int y, int size);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawRect(int x1, int y1, int x2, int y2, bool fill, bool line);

    void drawText(int num, int x, int y);
    void drawText(double num, int x, int y, int precision);
    void drawText(string str, int x, int y);
};
}

#endif
