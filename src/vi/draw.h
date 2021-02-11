#ifndef _VI_DRAW_H_
#define _VI_DRAW_H_

#ifdef LIBGD

#include <gd.h>
#include <gdfontg.h>
#include <gdfontl.h>
#include <gdfontmb.h>
#include <gdfonts.h>
#include <gdfontt.h>

#endif

#include <map>
using namespace std;

namespace vi {

class Color {
public:
    unsigned char a = 0;
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;

    //  #00000000
    static const Color BLACK;
    //  #00404040
    static const Color VERY_DARK_GRAY;
    //  #00808080
    static const Color DARK_GRAY;
    //  #00DADADA
    static const Color VERY_LIGHT_GRAY;
    //  #00FFFFFF
    static const Color WHITE;
    //  #0068372B
    static const Color VERY_DARK_DESATURATED_RED;
    //  #00B1001C
    static const Color OMNI_LIGHT_CAYENNE;
    //  #00D16E57
    static const Color MODERATE_RED;
    //  #00F7788A
    static const Color OMNI_SALMON;
    static const Color OMNI_SALMON5;
    //  #00FF0000
    static const Color PURE_RED;
    //  #0068BC36
    static const Color OMNI_FERN;
    static const Color OMNI_FERN5;
    //  #0037682B
    static const Color VERY_DARK_DESATURATED_LIME_GREEN;
    //  #006ED157
    static const Color MODERATE_LIME_GREEN;
    //  #0000BFC0
    static const Color OMNI_LIGHT_TEAL;
    static const Color OMNI_LIGHT_TEAL5;
    //  #003A8EED
    static const Color OMNI_DEEP_SKY_BLUE;
    static const Color OMNI_DEEP_SKY_BLUE5;
    //  #000000FF
    static const Color PURE_BLUE;
    static const Color PURE_BLUE5;

    Color();
    Color(int a, int r, int g, int b);
    Color(int r, int g, int b);
    Color(unsigned int color);
};

class ColorScale {
private:
    map<double, Color> keys;

public:
    ColorScale() {}

    void addColor(double value, Color color) { keys[value] = color; }
    Color getColor(double value);

    void setTransition(Color color0, Color color1);

    void setDefaultRainbow();
    void setDefaultBlackYellowRed();
};

class Canvas {
private:

#ifdef LIBGD
    gdImagePtr _image;
    gdFontPtr _font;
#endif

    int _lineColor;
    int _fillColor;
    int _lineSize;

protected:
    double _lx, _ly;
    double _hx, _hy;
    int _imgw;
    int _imgh;
    bool _fx;
    bool _fy;

private:
    int getImgX(double x);
    int getImgY(double y);

public:
    enum Font {
        FontTiny,
        FontSmall,
        FontMediumBold,
        FontLarge,
        FontGiant
    };

    Canvas();
    Canvas(int w, int h, bool fx = false, bool fy = false);
    Canvas(int w, int h, double lx, double ly, double hx, double hy, bool fx = false, bool fy = true);
    ~Canvas();

    void SetViewport(double lx, double ly, double hx, double hy);
    void SetWindow(int w, int h);
    void Init();
    void Free();

    void Clear();
    void Clear(Color color);

    void DrawLine(double x1, double y1, double x2, double y2);
    void DrawRect(double x1, double y1, double x2, double y2, bool fill = true, bool line = true);
    void DrawPoint(double x, double y, int size = 1);
    void DrawString(string str, double x, double y);
    void DrawNumber(int num, double x, double y);
    void DrawNumber(double num, double x, double y, int precision = 2);

    void SetLineColor(Color color);

#ifdef LIBGD
    void SetFillColor(Color color) { _fillColor = gdImageColorAllocateAlpha(_image, color.r, color.g, color.b, color.a); }
#endif

    void SetLineWidth(int size);
    void SetFont(Font size);

    void Save(string file);
};
}

#endif
