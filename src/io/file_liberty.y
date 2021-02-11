%{

#include "../global.h"
//  #include "../data.h"
#include "../sta/sta.h"

#include "file_liberty.h"

using namespace db;
using namespace sta;

#define yylex   file_libertylex
#define yyparse file_libertyparse
#define yyin    file_libertyin
#define yyerror file_libertyerror

extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

STALibrary *stalib;
string lutTemplate;
char pindir='x';
STALibraryTiming timing;
STALibraryIPin ipin;
STALibraryOPin opin;
STALibraryCell cell;
STALibraryLUT lut;
stack<string> svalues;
vector<double> dvalues;

//%define api.prefix {liberty}

void yyerror(const char *s);

%}

%define parse.error verbose
%define parse.lac full
%locations

%union{
    double  dval;
    char *  sval;
    int     token;
}

%token <dval> NUMBER
%token <sval> STRING

%%
liberty: 
        group { }
;
statements:
        /* empty */
    |   statements statement
;
statement:
        simple_attribute
    |   complex_attribute
    |   group
;
simple_attribute:
        STRING ':' STRING ';' {
            if (!strcmp($1, "date")) {
            } else if (!strcmp($1, "revision")) {
            } else if (!strcmp($1, "comment")) {
            } else if (!strcmp($1, "delay_model")) {
            } else if (!strcmp($1, "in_place_swap_mode")) {
            } else if (!strcmp($1, "time_unit")) {
            } else if (!strcmp($1, "leakage_power_unit")) {
            } else if (!strcmp($1, "voltage_unit")) {
            } else if (!strcmp($1, "current_unit")) {
            } else if (!strcmp($1, "pulling_resistance_unit")) {
            } else if (!strcmp($1, "process_corner")) {
            } else if (!strcmp($1, "tree_type")) {
            } else if (!strcmp($1, "default_operating_conditions")) {
            } else if (!strcmp($1, "default_wire_load")) {
            } else if (!strcmp($1, "variable_1")) {
            } else if (!strcmp($1, "variable_2")) {
            } else if (!strcmp($1, "variable_3")) {
            } else if (!strcmp($1, "voltage_name")) {
            } else if (!strcmp($1, "pg_type")) {
            } else if (!strcmp($1, "when")) {
            } else if (!strcmp($1, "direction")) {
                if (!strcmp($3, "output")) {
                    pindir = 'o';
                } else if (!strcmp($3, "input")) {
                    pindir = 'i';
                } else if (!strcmp($3, "internal")) {
                    pindir = 'n';
                } else {
                    cout<<"unknown direction: "<<$3<<endl;
                    getchar();
                }
            } else if (!strcmp($1, "related_power_pin")) {
            } else if (!strcmp($1, "related_ground_pin")) {
            } else if (!strcmp($1, "function")) {
            } else if (!strcmp($1, "related_pin")) {
                timing.relatedPinName = $3;
            } else if (!strcmp($1, "timing_sense")) {
                if(strcmp($3, "negative_unate")==0){
                    timing.timingSense = '-';
                }else if(strcmp($3, "positive_unate")==0){
                    timing.timingSense = '+';
                }else if(strcmp($3, "non_unate")==0){
                    timing.timingSense = 'x';
                }else{
                    cout<<"unknown timing sense: "<<$3<<endl;
                }
            } else if (!strcmp($1, "dont_touch")) {
            } else if (!strcmp($1, "dont_use")) {
            } else if (!strcmp($1, "sdf_cond")) {
            } else if (!strcmp($1, "table")) {
            } else if (!strcmp($1, "clock_gating_integrated_cell")) {
            } else if (!strcmp($1, "internal_node")) {
            } else if (!strcmp($1, "state_function")) {
            } else if (!strcmp($1, "clock_gate_clock_pin")) {
            } else if (!strcmp($1, "clock_gate_enable_pin")) {
            } else if (!strcmp($1, "clock_gate_test_pin")) {
            } else if (!strcmp($1, "clock_gate_out_pin")) {
            } else if (!strcmp($1, "timing_type")) {
            } else if (!strcmp($1, "clock")) {
            } else if (!strcmp($1, "next_state")) {
            } else if (!strcmp($1, "clocked_on")) {
            } else if (!strcmp($1, "preset")) {
            } else if (!strcmp($1, "clear")) {
            } else if (!strcmp($1, "clear_preset_var1")) {
            } else if (!strcmp($1, "clear_preset_var2")) {
            } else if (!strcmp($1, "data_in")) {
            } else if (!strcmp($1, "enable")) {
            } else if (!strcmp($1, "three_state")) {
            } else if (!strcmp($1, "driver_type")) {
            }else if(strcmp($1, "nextstate_type")==0){
            }else{
                cout<<$1<<" = "<<$3<<endl;
            }
        }
    |   STRING ':' NUMBER ';' {
            if (!strcmp($1, "nom_process")) {
            } else if (!strcmp($1, "nom_voltage")) {
            } else if (!strcmp($1, "nom_temperature")) {
            } else if (!strcmp($1, "process")) {
            } else if (!strcmp($1, "voltage")) {
            } else if (!strcmp($1, "temperature")) {
            } else if (!strcmp($1, "slew_lower_threshold_pct_rise")) {
            } else if (!strcmp($1, "slew_lower_threshold_pct_fall")) {
            } else if (!strcmp($1, "slew_upper_threshold_pct_rise")) {
            } else if (!strcmp($1, "slew_upper_threshold_pct_fall")) {
            } else if (!strcmp($1, "slew_derate_from_library")) {
            } else if (!strcmp($1, "input_threshold_pct_rise")) {
            } else if (!strcmp($1, "input_threshold_pct_fall")) {
            } else if (!strcmp($1, "output_threshold_pct_rise")) {
            } else if (!strcmp($1, "output_threshold_pct_fall")) {
            } else if (!strcmp($1, "default_leakage_power_density")) {
            } else if (!strcmp($1, "default_cell_leakage_power")) {
            } else if (!strcmp($1, "default_inout_pin_cap")) {
            } else if (!strcmp($1, "default_input_pin_cap")) {
            } else if (!strcmp($1, "default_output_pin_cap")) {
            } else if (!strcmp($1, "default_fanout_load")) {
            } else if (!strcmp($1, "default_max_transition")) {
            } else if (!strcmp($1, "capacitance")) {
                ipin.capacitance = $3;
            } else if (!strcmp($1, "resistance")) {
            } else if (!strcmp($1, "slope")) {
            } else if (!strcmp($1, "drive_strength")) {
            } else if (!strcmp($1, "area")) {
            } else if (!strcmp($1, "value")) {
            } else if (!strcmp($1, "cell_leakage_power")) {
            } else if (!strcmp($1, "fall_capacitance")) {
            } else if (!strcmp($1, "rise_capacitance")) {
            } else if (!strcmp($1, "max_capacitance")) {
                opin.max_capacitance = $3;
            } else if (!strcmp($1, "min_capacitance")) {
                opin.min_capacitance = $3;
            } else if (!strcmp($1, "function")) {
            } else if (!strcmp($1, "reference_time")) {
            } else {
                cout<<$1<<" = "<<$3<<endl;
            }
        }
    |   STRING ':' value_list ';' {
            if (!strcmp($1, "date")) {
            } else {
                cout << $1 << " = simple\n";
            }
        }
;
complex_attribute:
        STRING '(' STRING ')' ';' {
            if (!strcmp($1, "technology")) {
            } else {
                cout << $1 << " = (\"" << $3 << "\")\n";
            }
        }
    |   STRING '(' NUMBER ')' ';' {
            if (!strcmp($1, "index_1")) {
                lut.indexX = dvalues;
                dvalues.clear();
            } else if (!strcmp($1, "index_2")) {
                lut.indexY = dvalues;
                dvalues.clear();
            } else if (!strcmp($1, "index_3")) {
                lut.indexY = dvalues;
                dvalues.clear();
            } else if (!strcmp($1, "values")) {
                lut.values.resize(1, vector<vector<double>>(1, vector<double>(1, $3)));
            } else {
                cout << $1 << " = (" << $3 << ")\n";
            }
        }
    |   STRING '(' value_list ')' ';' {
            if (!strcmp($1, "library_features")) {
            } else if (!strcmp($1, "capacitive_load_unit")) {
                dvalues.clear();
                svalues.pop();
            } else if (!strcmp($1, "voltage_map")) {
            } else if (!strcmp($1, "define")) {
            } else if (!strcmp($1, "fanout_length")) {
            } else if (!strcmp($1, "index_1")) {
                lut.indexX = dvalues;
                dvalues.clear();
            } else if (!strcmp($1, "index_2")) {
                lut.indexY = dvalues;
                dvalues.clear();
            } else if (!strcmp($1, "index_3")) {
                lut.indexY = dvalues;
                dvalues.clear();
            } else if (!strcmp($1, "values")) {
                unsigned nx = max(1UL, lut.indexX.size());
                unsigned ny = max(1UL, lut.indexY.size());
                unsigned nz = max(1UL, lut.indexZ.size());
                //  cout << dvalues.size() << '\t' << nx << '\t' << ny << endl;
                if (dvalues.size() != nx * ny * nz) {
                    yyerror("size not match");
                }
                unsigned i = 0;
                lut.values.resize(nx, vector<vector<double>>(ny, vector<double>(nz, 0.0)));
                for (vector<vector<double>>& value : lut.values) {
                    for (vector<double>& val : value) {
                        for (double& v : val) {
                            v = dvalues[i++];
                        }
                    }
                }
                dvalues.clear();
            }else{
                cout<<$1<<" = (complex)"<<endl;
            }
        }
;
value_list:
        value ',' value
    |   value_list ',' value
;
value:
        STRING { svalues.push($1); }
    |   NUMBER { dvalues.push_back($1); }
;
group:
        STRING '(' ')' '{' statements '}' {
            if (!strcmp($1, "leakage_power")) {
            } else if (!strcmp($1, "timing")) {
                opin.timings.push_back(timing);
            } else if (!strcmp($1, "internal_power")) {
            } else if (!strcmp($1, "output_current_fall")) {
            } else if (!strcmp($1, "output_current_rise")) {
            } else if (!strcmp($1, "receiver_capacitance")) {
            } else {
                cout <<"unknown group : " << $1 << endl;
            }
        }
    |   STRING '(' STRING ')' '{' statements '}' {
            if (!strcmp($1, "library")) {
                stalib->name = $3;
            } else if (!strcmp($1, "operating_conditions")) {
            } else if (!strcmp($1, "wire_load")) {
            } else if (!strcmp($1, "power_lut_template")) {
                lut.clear();
            } else if (!strcmp($1, "lu_table_template")) {
                lutTemplate = $3;
                lut.clear();
            } else if(!strcmp($1, "cell")) {
                cell.name($3);
                unordered_map<string, CellType*>::iterator mi = database.name_celltypes.find(cell.name());
                if (mi != database.name_celltypes.end()) {
                    STALibraryCell& libcell = stalib->cells[mi->second->libcell()];
                    for (const STALibraryIPin& ipin : cell.ipins) {
                        bool ipFound = false;
                        for (STALibraryIPin& libipin : libcell.ipins) {
                            if(ipin.name() == libipin.name()) {
                                libipin = move(ipin);
                                ipFound = true;
                                break;
                            }
                        }
                        if (!ipFound) {
                            cerr << "input pin not found: " << ipin.name() << endl;
                        }
                    }
                    for (const STALibraryOPin& opin : cell.opins) {
                        //  cout << opin.name() << '\t' << opin.max_capacitance << endl;
                        bool opFound = false;
                        for (STALibraryOPin& libopin : libcell.opins) {
                            if (opin.name() == libopin.name()) {
                                libopin = move(opin);
                                opFound = true;
                                break;
                            }
                        }
                        if (!opFound) {
                            cerr << cell.name() << "\toutput pin not found: " << opin.name() << '\t' << opin.max_capacitance << endl;
                            assert(false);
                        }
                    }
                } else {
                    cout << "not found: " << $3 << endl;
                }
                cell.name("");
                cell.ipins.clear();
                cell.opins.clear();
            } else if (!strcmp($1, "pg_pin")) {
            } else if (!strcmp($1, "pin")) {
                switch (pindir) {
                case 'i':
                    ipin.name($3);
                    cell.ipins.push_back(ipin);
                    break;
                case 'n':
                    break;
                case 'o':
                    opin.name($3);
                    cell.opins.push_back(opin);
                    //  cout << pindir << ' ' << opin.name() << endl;
                    break;
                default:
                    cout << "unknown pin type: " << pindir << endl;
                    break;
                }

                ipin.name("");
                ipin.capacitance = 0.0;
                opin.name("");
                opin.min_capacitance = 0.0;
                opin.max_capacitance = 0.0;
                opin.timings.clear();
                pindir = 'x';
            } else if (!strcmp($1, "cell_fall")) {
                if(strcmp($3, lutTemplate.c_str())==0){
                    timing.delayFall = lut;
                } else if (!strcmp($3, "scalar")) {
                    timing.delayFall = STALibraryLUT(lut.values[0][0][0]);
                }
                lut.clear();
            } else if (!strcmp($1, "cell_rise")) {
                if(strcmp($3, lutTemplate.c_str())==0){
                    timing.delayRise = lut;
                } else if (!strcmp($3, "scalar")) {
                    timing.delayRise = STALibraryLUT(lut.values[0][0][0]);
                }
                lut.clear();
            } else if (!strcmp($1, "fall_transition")) {
                if(strcmp($3, lutTemplate.c_str())==0){
                    timing.slewFall = lut;
                } else if (!strcmp($3, "scalar")) {
                    timing.slewFall = STALibraryLUT(lut.values[0][0][0]);
                }
                lut.clear();
            } else if (!strcmp($1, "rise_transition")) {
                if(strcmp($3, lutTemplate.c_str())==0){
                    timing.slewRise = lut;
                } else if(!strcmp($3, "scalar")) {
                    timing.slewRise = STALibraryLUT(lut.values[0][0][0]);
                }
                lut.clear();
            } else if (!strcmp($1, "fall_power")) {
                lut.clear();
            } else if (!strcmp($1, "rise_power")) {
                lut.clear();
            } else if (!strcmp($1, "rise_constraint")) {
                lut.clear();
            } else if (!strcmp($1, "fall_constraint")) {
                lut.clear();
            } else if (!strcmp($1, "output_current_template")) {
            } else if (!strcmp($1, "receiver_capacitance1_fall")) {
                lut.clear();
            } else if (!strcmp($1, "receiver_capacitance1_rise")) {
                lut.clear();
            } else if (!strcmp($1, "receiver_capacitance2_fall")) {
                lut.clear();
            } else if (!strcmp($1, "receiver_capacitance2_rise")) {
                lut.clear();
            } else if (!strcmp($1, "vector")) {
                lut.clear();
            } else {
                cout << "read group " << $1 << "() finish" << endl;
            }
        }
    |   STRING '(' value_list ')' '{' statements '}' {
            /* ignore ff group */
        }
;

%%

void read_liberty(STALibrary *lib, const string& filename)
{
    FILE *fp = fopen(filename.c_str(), "r");
    if(!fp){
        cerr<<"open file fail: "<<filename<<" : "
            <<__FILE__<<":"<<__LINE__<<endl;
        return;
    }
    yyin = fp;
    stalib = lib;
    lib->preLoad();
    while(!feof(yyin)){
        yyparse();
    }
    fclose(fp);
    lib->postLoad();
}

bool Database::readLiberty(const string& filename)
{
    read_liberty(&(rtimer.lLib), filename);
    return true;
}

/*
void read_liberty_benchmark(STATimer *timer)
{
    read_liberty(&(timer->lLib), setting.benchmark_dir+setting.lib_late);
    read_liberty(&(timer->eLib), setting.benchmark_dir+setting.lib_early);
}
*/

