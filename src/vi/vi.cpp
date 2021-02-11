#include "../db/db.h"
using namespace db;
#include "../ut/utils.h"
#include "vi.h"
using namespace vi;

void ColorScheme::setDefault() {
    metalFill.resize(12);
    metalLine.resize(12);
    viaFill.resize(11);
    viaLine.resize(11);
    background = Color::BLACK;
    instanceFill = Color(0, 255, 255);
    instanceLine = Color(0, 128, 128);
    rowFill = Color(0, 0, 0);
    rowLine = Color(128, 128, 128);
    regionFill = Color(128, 0, 64);
    metalFill[0] = Color(0, 0, 128);
    metalLine[0] = Color(0, 0, 255);
    metalFill[1] = Color(128, 0, 0);
    metalLine[1] = Color(255, 0, 0);
    metalFill[2] = Color(0, 128, 0);
    metalLine[2] = Color(0, 255, 0);
    metalFill[3] = Color(128, 128, 0);
    metalLine[3] = Color(255, 255, 0);
    metalFill[4] = Color(128, 0, 64);
    metalLine[4] = Color(255, 0, 128);
    metalFill[5] = Color(0, 0, 128);
    metalLine[5] = Color(0, 0, 255);
    metalFill[6] = Color(0, 0, 128);
    metalLine[6] = Color(0, 0, 255);
    metalFill[7] = Color(0, 0, 128);
    metalLine[7] = Color(0, 0, 255);
    metalFill[8] = Color(0, 0, 128);
    metalLine[8] = Color(0, 0, 255);
    metalFill[9] = Color(0, 0, 128);
    metalLine[9] = Color(0, 0, 255);
    metalFill[10] = Color(0, 0, 128);
    metalLine[10] = Color(0, 0, 255);
    metalFill[11] = Color(0, 0, 128);
    metalLine[11] = Color(0, 0, 255);
}

void ColorScheme::setLight() {
    metalFill.resize(12);
    metalLine.resize(12);
    viaFill.resize(11);
    viaLine.resize(11);
    background = Color::WHITE;
    instanceFill = Color::VERY_LIGHT_GRAY;
    instanceLine = Color::VERY_DARK_GRAY;
    rowFill = Color::WHITE;
    rowLine = Color::VERY_DARK_GRAY;
    regionFill = Color(128, 0, 64);
    metalFill[0] = Color(0, 0, 128);
    metalLine[0] = Color(0, 0, 255);
    metalFill[1] = Color(128, 0, 0);
    metalLine[1] = Color(255, 0, 0);
    metalFill[2] = Color(0, 128, 0);
    metalLine[2] = Color(0, 255, 0);
    metalFill[3] = Color(128, 128, 0);
    metalLine[3] = Color(255, 255, 0);
    metalFill[4] = Color(128, 0, 64);
    metalLine[4] = Color(255, 0, 128);
    metalFill[5] = Color(0, 0, 128);
    metalLine[5] = Color(0, 0, 255);
    metalFill[6] = Color(0, 0, 128);
    metalLine[6] = Color(0, 0, 255);
    metalFill[7] = Color(0, 0, 128);
    metalLine[7] = Color(0, 0, 255);
    metalFill[8] = Color(0, 0, 128);
    metalLine[8] = Color(0, 0, 255);
    metalFill[9] = Color(0, 0, 128);
    metalLine[9] = Color(0, 0, 255);
    metalFill[10] = Color(0, 0, 128);
    metalLine[10] = Color(0, 0, 255);
    metalFill[11] = Color(0, 0, 128);
    metalLine[11] = Color(0, 0, 255);
}

void Visualizer::setWindow(int w, int h)
{
    this->w = w;
    this->h = h;
    this->ready = false;
}

void Visualizer::setViewport(int lx, int ly, int hx, int hy)
{
    this->lx = lx;
    this->ly = ly;
    this->hx = hx;
    this->hy = hy;
    this->ready = false;
}

void Visualizer::setup()
{
    if (!ready) {
        _canvas.SetViewport(lx, ly, hx, hy);
        _canvas.SetWindow(w, h);
        _canvas.Init();
        ready = true;
    }
}

void Visualizer::reset()
{
    setup();
    _canvas.Clear();
}

void Visualizer::reset(Color color)
{
    setup();
    _canvas.Clear(color);
}

void Visualizer::setLineColor(Color color)
{
    setup();
    _canvas.SetLineColor(color);
}

void Visualizer::setFillColor(Color color) {
    setup();

#ifdef LIBGD
    _canvas.SetFillColor(color);
#endif
}

void Visualizer::setLineWidth(int size)
{
    setup();
    _canvas.SetLineWidth(size);
}

void Visualizer::drawPoint(int x, int y, int size)
{
    _canvas.DrawPoint(x, y, size);
}

void Visualizer::drawLine(int x1, int y1, int x2, int y2)
{
    _canvas.DrawLine(x1, y1, x2, y2);
}

void Visualizer::drawRect(int x1, int y1, int x2, int y2, bool fill, bool line)
{
    _canvas.DrawRect(x1, y1, x2, y2, fill, line);
}

void Visualizer::drawText(int num, int x, int y)
{
    _canvas.DrawNumber(num, x, y);
}

void Visualizer::drawText(double num, int x, int y, int precision)
{
    _canvas.DrawNumber(num, x, y, precision);
}

void Visualizer::drawText(string str, int x, int y)
{
    _canvas.DrawString(str, x, y);
}
