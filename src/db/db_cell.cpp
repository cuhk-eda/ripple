#include "db.h"
using namespace db;

#include "../ut/utils.h"

/***** Cell *****/

Cell::~Cell() {
    for (Pin* pin : _pins) {
        delete pin;
    }
}

Pin* Cell::pin(const string& name) const {
    for (Pin* pin : _pins) {
        if (pin->type->name() == name) {
            return pin;
        }
    }
    return nullptr;
}

void Cell::ctype(CellType* t) {
    if (!t) {
        return;
    }
    if (_type) {
        printlog(LOG_ERROR, "type of cell %s already set", _name.c_str());
        return;
    }
    _type = t;
    ++(_type->usedCount);
    _pins.resize(_type->pins.size(), nullptr);
    for (unsigned i = 0; i != _pins.size(); ++i) {
        _pins[i] = new Pin(this, i);
    }
}

int Cell::lx() const { return database.placement().x(const_cast<Cell*>(this)); }
int Cell::ly() const { return database.placement().y(const_cast<Cell*>(this)); }
bool Cell::flipX() const { return database.placement().flipX(const_cast<Cell*>(this)); }
bool Cell::flipY() const { return database.placement().flipY(const_cast<Cell*>(this)); }
int Cell::siteWidth() const { return width() / database.siteW; }
int Cell::siteHeight() const { return height() / database.siteH; }
bool Cell::placed() const { return database.placement().placed(const_cast<Cell*>(this)); }

void Cell::place(int x, int y) {
    if (_fixed) {
        printlog(LOG_WARN, "moving fixed cell %s to (%d,%d)", _name.c_str(), x, y);
    }
    database.placement().place(this, x, y);
}

void Cell::place(int x, int y, bool flipX, bool flipY) {
    if (_fixed) {
        printlog(LOG_WARN, "moving fixed cell %s to (%d,%d)", _name.c_str(), x, y);
    }
    database.placement().place(this, x, y, flipX, flipY);
}

void Cell::unplace() {
    if (_fixed) {
        printlog(LOG_WARN, "unplace fixed cell %s", _name.c_str());
    }
    database.placement().place(this);
}

/***** Cell Type *****/

CellType::~CellType() {
    for (PinType* pin : pins) {
        delete pin;
    }
}

PinType* CellType::addPin(const string& name, const char direction, const char type) {
    PinType* newpintype = new PinType(name, direction, type);
    pins.push_back(newpintype);
    return newpintype;
}

PinType* CellType::getPin(string& name) {
    for (int i = 0; i < (int)pins.size(); i++) {
        if (pins[i]->name() == name) {
            return pins[i];
        }
    }
    return nullptr;
}

void CellType::setOrigin(int x, int y) {
    _originX = x;
    _originY = y;
}

bool CellType::operator==(const CellType& r) const {
    if (width != r.width || height != r.height) {
        return false;
    } else if (_originX != r.originX() || _originY != r.originY() || _symmetry != r.symmetry() ||
               pins.size() != r.pins.size()) {
        return false;
    } else if (edgetypeL != r.edgetypeL || edgetypeR != r.edgetypeR) {
        return false;
    } else {
        //  return PinType::comparePin(pins, r.pins);
        for (unsigned i = 0; i != pins.size(); ++i) {
            if (*pins[i] != *r.pins[i]) {
                return false;
            }
        }
    }
    return true;
}
