#include "../db/db.h"
using namespace db;

Drawable* Cell::parent() const { return &database; }
int Cell::globalX() const { return parent()->globalX() + localX(); }
int Cell::globalY() const { return parent()->globalY() + localY(); }
int Cell::localX() const { return lx(); }
int Cell::localY() const { return ly(); }
int Cell::boundL() const { return 0; }
int Cell::boundR() const { return width(); }
int Cell::boundB() const { return 0; }
int Cell::boundT() const { return height(); }

Drawable* Net::parent() const { return &database; }
int Net::globalX() const { return parent()->globalX() + localX(); }
int Net::globalY() const { return parent()->globalY() + localY(); }
int Net::localX() const { return 0; }
int Net::localY() const { return 0; }
int Net::boundL() const { return 0; }
int Net::boundR() const { return 0; }
int Net::boundB() const { return 0; }
int Net::boundT() const { return 0; }

Drawable* SNet::parent() const { return &database; }
int SNet::globalX() const { return parent()->globalX() + localX(); }
int SNet::globalY() const { return parent()->globalY() + localY(); }
int SNet::localX() const { return 0; }
int SNet::localY() const { return 0; }
int SNet::boundL() const { return 0; }
int SNet::boundR() const { return 0; }
int SNet::boundB() const { return 0; }
int SNet::boundT() const { return 0; }

Drawable* IOPin::parent() const { return &database; }
int IOPin::globalX() const { return parent()->globalX() + localX(); }
int IOPin::globalY() const { return parent()->globalY() + localY(); }
int IOPin::localX() const { return x; }
int IOPin::localY() const { return y; }
int IOPin::boundL() const { return 0; }
int IOPin::boundR() const { return type->shapes[0].hx - type->shapes[0].lx; }
int IOPin::boundB() const { return 0; }
int IOPin::boundT() const { return type->shapes[0].hy - type->shapes[0].ly; }

Drawable* Pin::parent() const { return (cell != NULL) ? (Drawable*)cell : (Drawable*)iopin; }
int Pin::globalX() const { return parent()->globalX() + localX(); }
int Pin::globalY() const { return parent()->globalY() + localY(); }
int Pin::localX() const { return type->shapes[0].lx; }
int Pin::localY() const { return type->shapes[0].ly; }
int Pin::boundL() const { return 0; }
int Pin::boundR() const { return type->shapes[0].hx - type->shapes[0].lx; }
int Pin::boundB() const { return 0; }
int Pin::boundT() const { return type->shapes[0].hy - type->shapes[0].ly; }

Drawable* Region::parent() const { return &database; }
int Region::globalX() const { return parent()->globalX() + localX(); }
int Region::globalY() const { return parent()->globalY() + localY(); }
int Region::localX() const { return 0; }
int Region::localY() const { return 0; }
int Region::boundL() const { return 0; }
int Region::boundR() const { return database.coreHX - database.coreLX; }
int Region::boundB() const { return 0; }
int Region::boundT() const { return database.coreHY - database.coreLY; }

Drawable* Geometry::parent() const
{
    if (_parent == NULL) {
        return &database;
    }
    return _parent;
}
int Geometry::globalX() const { return parent()->globalX() + localX(); }
int Geometry::globalY() const { return parent()->globalY() + localY(); }
int Geometry::localX() const { return lx; }
int Geometry::localY() const { return ly; }
int Geometry::boundL() const { return 0; }
int Geometry::boundR() const { return hx - lx; }
int Geometry::boundB() const { return 0; }
int Geometry::boundT() const { return hy - ly; }

Drawable* Row::parent() const { return &database; }
int Row::globalX() const { return parent()->globalX() + localX(); }
int Row::globalY() const { return parent()->globalY() + localY(); }
int Row::localX() const { return _x; }
int Row::localY() const { return _y; }
int Row::boundL() const { return 0; }
int Row::boundR() const { return _xStep * _xNum; }
int Row::boundB() const { return 0; }
int Row::boundT() const { return _yStep * _yNum; }

Drawable* Via::parent() const
{
    if (_parent == nullptr) {
        return &database;
    }
    return _parent;
}
int Via::globalX() const { return parent()->globalX() + localX(); }
int Via::globalY() const { return parent()->globalY() + localY(); }
int Via::localX() const { return x; }
int Via::localY() const { return y; }
int Via::boundL() const { return 0; }
int Via::boundR() const { return type->rects.begin()->hx - type->rects.begin()->lx; }
int Via::boundB() const { return 0; }
int Via::boundT() const { return type->rects.begin()->hy - type->rects.begin()->ly; }

Drawable* SiteMap::parent() const { return &database; }
int SiteMap::globalX() const { return parent()->globalX() + localX(); }
int SiteMap::globalY() const { return parent()->globalY() + localY(); }
int SiteMap::localX() const { return 0; }
int SiteMap::localY() const { return 0; }
int SiteMap::boundL() const { return 0; }
int SiteMap::boundR() const { return database.coreHX - database.coreLX; }
int SiteMap::boundB() const { return 0; }
int SiteMap::boundT() const { return database.coreHY - database.coreLY; }

Drawable* Database::parent() const { return nullptr; }
int Database::globalX() const { return dieLX; }
int Database::globalY() const { return dieLY; }
int Database::localX() const { return 0; }
int Database::localY() const { return 0; }
int Database::boundL() const { return 0; }
int Database::boundR() const { return dieHX - dieLX; }
int Database::boundB() const { return 0; }
int Database::boundT() const { return dieHY - dieLY; }
