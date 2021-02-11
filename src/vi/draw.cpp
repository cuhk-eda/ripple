#include "../db/db.h"
#include "../global.h"
using namespace db;

#include "draw.h"
using namespace vi;

#define IMAGE_AREA_LIMIT 16000000

/***** Color *****/
const Color Color::BLACK(0x00000000);
const Color Color::VERY_DARK_GRAY(0x00404040);
const Color Color::DARK_GRAY(0x00808080);
const Color Color::VERY_LIGHT_GRAY(0x00DADADA);
const Color Color::WHITE(0x00FFFFFF);

const Color Color::VERY_DARK_DESATURATED_RED(0x0068372B);
const Color Color::OMNI_LIGHT_CAYENNE(0x00B1001C);
const Color Color::MODERATE_RED(0x00D16E57);
const Color Color::OMNI_SALMON(0x00F7788A);
const Color Color::OMNI_SALMON5(0x55F7788A);
const Color Color::PURE_RED(0x00FF0000);

const Color Color::OMNI_FERN(0x0068BC36);
const Color Color::OMNI_FERN5(0x5568BC36);

const Color Color::VERY_DARK_DESATURATED_LIME_GREEN(0x0037682B);
const Color Color::MODERATE_LIME_GREEN(0x006ED157);

const Color Color::OMNI_LIGHT_TEAL(0x0000BFC0);
const Color Color::OMNI_LIGHT_TEAL5(0x5500BFC0);

const Color Color::OMNI_DEEP_SKY_BLUE(0x003A8EED);
const Color Color::OMNI_DEEP_SKY_BLUE5(0x553A8EED);
const Color Color::PURE_BLUE(0x000000FF);
const Color Color::PURE_BLUE5(0x550000FF);

Color::Color()
    : Color(0, 0, 0, 0)
{
}

Color::Color(int a, int r, int g, int b)
    : a(a)
    , r(r)
    , g(g)
    , b(b)
{
}

Color::Color(int r, int g, int b)
    : Color(255, r, g, b)
{
}

Color::Color(unsigned int color)
{
    a = (color & 0xff000000) >> 24;
    r = (color & 0x00ff0000) >> 16;
    g = (color & 0x0000ff00) >> 8;
    b = (color & 0x000000ff);
}

/***** ColorScale *****/

Color ColorScale::getColor(double value)
{
    Color color;
    value = max(0.0, min(1.0, value));
    if (value == 0.0) {
        return keys[0.0];
    }
    map<double, Color>::iterator lo = keys.lower_bound(value);
    map<double, Color>::iterator hi = keys.upper_bound(value);
    if (lo == hi) {
        --lo;
    }
    double hiR = (value - lo->first) / (hi->first - lo->first);
    double loR = 1.0 - hiR;
    color.a = round((double)lo->second.a * loR + (double)hi->second.a * hiR);
    color.r = round((double)lo->second.r * loR + (double)hi->second.r * hiR);
    color.g = round((double)lo->second.g * loR + (double)hi->second.g * hiR);
    color.b = round((double)lo->second.b * loR + (double)hi->second.b * hiR);
    return color;
}

void ColorScale::setTransition(Color color0, Color color1)
{
    keys.clear();
    addColor(0.000, color0);
    addColor(1.000, color1);
}

void ColorScale::setDefaultRainbow()
{
    keys.clear();
    addColor(0.000, Color(0, 0, 128));
    addColor(0.125, Color(0, 0, 255));
    addColor(0.250, Color(0, 128, 255));
    addColor(0.375, Color(0, 255, 255));
    addColor(0.500, Color(128, 255, 128));
    addColor(0.625, Color(255, 255, 0));
    addColor(0.750, Color(255, 128, 0));
    addColor(0.875, Color(255, 0, 0));
    addColor(1.000, Color(128, 0, 0));
}
void ColorScale::setDefaultBlackYellowRed()
{
    keys.clear();
    addColor(0.000, Color(0, 0, 0));
    addColor(0.499, Color(128, 128, 128));
    addColor(0.500, Color(255, 255, 0));
    addColor(0.501, Color(255, 255, 0));
    addColor(0.502, Color(128, 0, 0));
    addColor(1.000, Color(255, 0, 0));
}

#ifdef LIBGD
/***** Canvas *****/
Canvas::Canvas()
{
    _lx = _ly = 0.0;
    _hx = _hy = 0.0;
    _imgw = 0;
    _imgh = 0;
    _fx = false;
    _fy = true;

    _image = NULL;
    _font = NULL;
}
Canvas::Canvas(int w, int h, bool fx, bool fy)
{
    SetWindow(w, h);
    SetViewport(0, w, 0, h);
    _fx = fx;
    _fy = fy;
    Init();
}
Canvas::Canvas(int w, int h, double lx, double ly, double hx, double hy, bool fx, bool fy)
{
    SetWindow(w, h);
    SetViewport(lx, ly, hx, hy);
    _fx = fx;
    _fy = fy;
    if ((_imgw * _imgh) > IMAGE_AREA_LIMIT) {
        printlog(LOG_WARN, "Image size too large: (%d,%d)", w, h);
    } else if ((_hx - _lx) <= 0 || (_hy - _ly) <= 0) {
        printlog(LOG_WARN, "Viewport width and height cannot be zero");
    } else {
        _image = NULL;
        _font = NULL;
        Init();
    }
}
Canvas::~Canvas()
{
    Free();
}
void Canvas::SetViewport(double lx, double ly, double hx, double hy)
{
    _lx = min(lx, hx);
    _ly = min(ly, hy);
    _hx = max(lx, hx);
    _hy = max(ly, hy);
    if (_hx == _lx) {
        _hx = _lx + 1e-12;
    }
    if (_hy == _ly) {
        _hy = _ly + 1e-12;
    }
}

void Canvas::SetWindow(int w, int h)
{
    if (w <= 0 && h <= 0) {
        w = _hx - _lx;
        h = _hy - _ly;
    } else if (w <= 0) {
        w = h / (_hy - _ly) * (_hx - _lx);
    } else if (h <= 0) {
        h = w / (_hx - _lx) * (_hy - _ly);
    }
    _imgw = w;
    _imgh = h;
}
void Canvas::Init()
{
    Free();
    _image = gdImageCreateTrueColor(_imgw, _imgh);
    _font = gdFontGetLarge();
    _lineSize = 1;
    Clear();
}
void Canvas::Free()
{
    if (_image) {
        gdImageDestroy(_image);
        _image = NULL;
    }
    _font = NULL;
}

int Canvas::getImgX(double x)
{
    if (_fx) {
        return _imgw * (_hx - x) / (_hx - _lx);
    } else {
        return _imgw * (x - _lx) / (_hx - _lx);
    }
}
int Canvas::getImgY(double y)
{
    if (_fy) {
        return _imgh * (_hy - y) / (_hy - _ly);
    } else {
        return _imgh * (y - _ly) / (_hy - _ly);
    }
}
void Canvas::Clear()
{
    Clear(Color(0, 1, 2));
}
void Canvas::Clear(Color color)
{
    if (_image) {
        gdImageSaveAlpha(_image, 0);
        gdImageColorTransparent(_image, gdImageColorAllocate(_image, 0, 1, 2));
        SetFillColor(color);
        gdImageFilledRectangle(_image, 0, 0, _imgw, _imgh, _fillColor);
    }
}
void Canvas::SetLineWidth(int size)
{
    if (size == _lineSize) {
        return;
    }
    _lineSize = size;
    gdImageSetThickness(_image, max(1, size));
}
void Canvas::DrawLine(double x1, double y1, double x2, double y2)
{
    gdImageLine(_image, getImgX(x1), getImgY(y1), getImgX(x2), getImgY(y2), _lineColor);
}
void Canvas::DrawRect(double x1, double y1, double x2, double y2, bool fill, bool line)
{
    int ix1 = getImgX(x1);
    int iy1 = getImgY(y1);
    int ix2 = getImgX(x2);
    int iy2 = getImgY(y2);
    if (fill) {
        gdImageFilledRectangle(_image, ix1, iy1, ix2, iy2, _fillColor);
    }
    if (line) {
        gdImageRectangle(_image, ix1, iy1, ix2, iy2, _lineColor);
    }
}
void Canvas::DrawPoint(double x, double y, int size)
{
    if (size <= 1) {
        gdImageSetPixel(_image, getImgX(x), getImgY(y), _fillColor);
    } else {
        int ix = getImgX(x) - (size - 1) / 2;
        int iy = getImgY(y) - (size - 1) / 2;
        gdImageFilledRectangle(_image, ix, iy, ix + size, iy + size, _fillColor);
    }
}
void Canvas::DrawString(string str, double x, double y)
{
    gdImageString(_image, _font, getImgX(x), getImgY(y), (unsigned char*)str.c_str(), _lineColor);
}
void Canvas::DrawNumber(int num, double x, double y)
{
    stringstream ss;
    ss << num;
    DrawString(ss.str(), x, y);
}
void Canvas::DrawNumber(double num, double x, double y, int precision)
{
    stringstream ss;
    ss.precision(precision);
    ss << num;
    DrawString(ss.str(), x, y);
}
void Canvas::SetLineColor(Color color)
{
    _lineColor = gdImageColorAllocate(_image, color.r, color.g, color.b);
}

void Canvas::Save(string file)
{
    FILE* fp;
    if (_image == NULL) {
        return;
    }
    if ((fp = fopen(file.c_str(), "wb")) == NULL) {
        return;
    }
    gdImagePng(_image, fp);
    fclose(fp);
}
#else
Canvas::Canvas()
{
}
Canvas::Canvas(int w, int h, bool fx, bool fy) {}
Canvas::Canvas(int w, int h, double lx, double ly, double hx, double hy, bool fx, bool fy) {}
Canvas::~Canvas() {}
void Canvas::SetViewport(double lx, double ly, double hx, double hy) {}
void Canvas::SetWindow(int w, int h) {}
void Canvas::Init() {}
void Canvas::Free() {}
int Canvas::getImgX(double x) { return 0; }
int Canvas::getImgY(double y) { return 0; }
void Canvas::Clear() {}
void Canvas::Clear(Color color) {}
void Canvas::SetLineWidth(int size) {}
void Canvas::DrawLine(double x1, double y1, double x2, double y2) {}
void Canvas::DrawRect(double x1, double y1, double x2, double y2, bool fill, bool line) {}
void Canvas::DrawPoint(double x, double y, int size) {}
void Canvas::DrawString(string str, double x, double y) {}
void Canvas::DrawNumber(int num, double x, double y) {}
void Canvas::DrawNumber(double num, double x, double y, int precision) {}
void Canvas::SetLineColor(Color color) {}
void Canvas::Save(string file) {}
#endif
