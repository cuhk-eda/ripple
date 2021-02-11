#include "db.h"
using namespace db;

Placement& Database::placement(){
    return _placements[_activePlacement];
}
Placement& Database::placement(unsigned i){
    return _placements[i];
}

void Database::setActivePlacement(unsigned i){
    _activePlacement = i;
    if(_activePlacement >= _placements.size()){
        _placements.resize(_activePlacement+1);
    }
}

