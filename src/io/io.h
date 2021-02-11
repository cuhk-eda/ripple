#ifndef _IO_IO_H_
#define _IO_IO_H_

#include "../tcl/tcl.h"

#include <string>

namespace io {
class IOModule : public ripple::ShellModule {
private:
    static std::string _name;

    void registerCommands();
    void registerOptions();
    void showOptions() const;

public:
    static std::string Format;

    static std::string BookshelfAux;
    static std::string BookshelfPl;
    static std::string BookshelfVariety;
    static std::string BookshelfPlacement;

    static std::string LefTech;
    static std::string LefCell;
    static std::string DefFloorplan;
    static std::string DefCell;
    static std::string DefPlacement;

    static std::string Verilog;
    static std::string Liberty;
    static std::string Size;
    static std::string Constraints;

public:
    const std::string& name() const { return _name; }
    static bool load();
    static bool load(ripple::ShellOptions& args, ripple::ShellCmdReturn& ret);
    static bool save();
    static bool save(ripple::ShellOptions& args, ripple::ShellCmdReturn& ret);
};
}

#endif
