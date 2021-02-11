#include "lib.h"
using namespace sta;

#include "../db/db.h"
using namespace db;

#include <experimental/iterator>

STALibraryLUT::STALibraryLUT(double scalar)
    : isScalar(true)
    , value(scalar)
{
}

void STALibraryLUT::clear()
{
    indexX.clear();
    indexY.clear();
    indexZ.clear();
    values.clear();
}

void STALibraryLUT::print() {
    cout << "index_1 : ";
    //  for (double i : indexX) {
    //      cout << i << ' ';
    //  }
    copy(indexX.begin(), indexX.end(), experimental::make_ostream_joiner(cout, " "));
    cout << endl;

    cout << "index_2 : ";
    //  for (double i : indexY) {
    //      cout << i << ' ';
    //  }
    copy(indexY.begin(), indexY.end(), experimental::make_ostream_joiner(cout, " "));
    cout << endl;
    
    cout << "index_3 : ";
    //  for (double i : indexZ) {
    //      cout << i << ' ';
    //  }
    copy(indexZ.begin(), indexZ.end(), experimental::make_ostream_joiner(cout, " "));
    cout << endl;

    for (const vector<vector<double>>& value : values) {
        for (const vector<double>& val : value) {
            //  for (double v : val) {
            //      cout << v << ' ';
            //  }
            copy(val.begin(), val.end(), experimental::make_ostream_joiner(cout, " "));
            cout << endl;
        }
        cout << endl;
    }
}

STALibraryOPin::STALibraryOPin(const STALibraryOPin& opin)
    : _name(opin._name)
    , min_capacitance(opin.min_capacitance)
    , max_capacitance(opin.max_capacitance)
    , timings(opin.timings)
{ }

STALibraryIPin::STALibraryIPin(const string& name)
    : _name(name)
{
}

STALibraryIPin::STALibraryIPin(const STALibraryIPin& ipin)
    : _name(ipin._name)
    , capacitance(ipin.capacitance)
{
}

STALibraryCell::STALibraryCell(const string& name)
    : _name(name)
{
}

void STALibrary::preLoad()
{
    /*
    cells.clear();
    int nCellTypes = database.getNumCellTypes();
    cells.resize(nCellTypes, STALibraryCell());
    for(int i=0; i<nCellTypes; i++){
        STALibraryCell &libcell = cells[i];
        libcell.name = database.celltypes[i]->name;
        int nPins = circuit.celltypes[i].pins.size();
        libcell.pins.resize(nPins);
        for(int p=0; p<nPins; p++){
            FPinType &pin = circuit.celltypes[i].pins[p];
            if(pin.dir=='I'){
                STALibraryIPin ipin;
                ipin.name = pin.name;
                ipin.pin = p;
                libcell.pins[p] = libcell.ipins.size();
                libcell.ipins.push_back(ipin);
            }else if(pin.dir=='O'){
                STALibraryOPin opin;
                opin.name = pin.name;
                opin.pin = p;
                libcell.pins[p] = libcell.opins.size();
                libcell.opins.push_back(opin);
            }else{
                cout<<"unknown dir="<<pin.dir<<endl;
            }
        }
        libcell.timingArcs.resize(libcell.ipins.size(), vector<int>(libcell.opins.size(), -1));
    }
    */
}

void STALibrary::postLoad()
{
    for (STALibraryCell& libcell : cells) {
        unsigned nOPins = libcell.opins.size();
        unsigned nIPins = libcell.ipins.size();
        libcell.timingArcs.resize(nIPins, vector<int>(nOPins, -1));
        for (unsigned op = 0; op != nOPins; ++op) {
            unsigned nTimings = libcell.opins[op].timings.size();
            for (unsigned t = 0; t != nTimings; ++t) {
                STALibraryTiming &timing = libcell.opins[op].timings[t];
                bool ipFound = false;
                for (unsigned ip = 0; ip != nIPins; ++ip) {
                    if(libcell.ipins[ip].name() == timing.relatedPinName){
                        timing.relatedPin = ip;
                        if (libcell.timingArcs[ip][op] != -1) {
                            //  cerr << "timing arc duplicated!" << endl;
                        }
                        libcell.timingArcs[ip][op] = t;
                        ipFound = true;
                        break;
                    }
                }
                if(!ipFound){
                    cout << "related pin (" << timing.relatedPinName << ") not found for pin " << libcell.opins[op].name() << endl;
                }
            }
        }
    }
}

unsigned STALibrary::addCell(const string& name)
{
    cells.emplace_back(name);
    return cells.size() - 1;
}
