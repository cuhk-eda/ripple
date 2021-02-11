#include "io.h"

#include "../db/db.h"
#include "../global.h"
#include "../tcl/register.h"

using namespace io;

std::string IOModule::_name = "io";

std::string IOModule::Format = "lefdef";
std::string IOModule::BookshelfAux = "";
std::string IOModule::BookshelfPl = "";
std::string IOModule::BookshelfVariety = "";
std::string IOModule::BookshelfPlacement = "";

std::string IOModule::LefTech = "";
std::string IOModule::LefCell = "";
std::string IOModule::DefFloorplan = "";
std::string IOModule::DefCell = "";
std::string IOModule::DefPlacement = "";

std::string io::IOModule::Verilog = "";
std::string io::IOModule::Liberty = "";
std::string io::IOModule::Size = "";
std::string io::IOModule::Constraints = "";

void io::IOModule::registerCommands() {
    ripple::Shell::addCommand(this, "load", IOModule::load);
    ripple::Shell::addCommand(this, "save", IOModule::save);

    ripple::Shell::addArgument(this, "load", "format", (std::string*)nullptr, "");
    ripple::Shell::addArgument(this, "load", "1", (std::string*)nullptr, "");
    ripple::Shell::addArgument(this, "load", "2", (std::string*)nullptr, "");
}

void io::IOModule::registerOptions() {
    ripple::Shell::addOption(this, "format", &IOModule::Format);

    ripple::Shell::addOption(this, "bookshelfAux", &IOModule::BookshelfAux);
    ripple::Shell::addOption(this, "bookshelfPl", &IOModule::BookshelfPl);
    ripple::Shell::addOption(this, "bookshelfVariety", &IOModule::BookshelfVariety);
    ripple::Shell::addOption(this, "bookshelfPlacement", &IOModule::BookshelfPlacement);

    ripple::Shell::addOption(this, "lefTech", &IOModule::LefTech);
    ripple::Shell::addOption(this, "lefCell", &IOModule::LefCell);
    ripple::Shell::addOption(this, "defFloorplan", &IOModule::DefFloorplan);
    ripple::Shell::addOption(this, "defCell", &IOModule::DefCell);
    ripple::Shell::addOption(this, "defPlacement", &IOModule::DefPlacement);

    ripple::Shell::addOption(this, "verilog", &IOModule::Verilog);
    ripple::Shell::addOption(this, "liberty", &IOModule::Liberty);
    ripple::Shell::addOption(this, "size", &IOModule::Size);
    ripple::Shell::addOption(this, "constraints", &IOModule::Constraints);
}

void IOModule::showOptions() const {
    printlog(LOG_INFO, "format              : %s", IOModule::Format.c_str());
    printlog(LOG_INFO, "bookshelfAux        : %s", IOModule::BookshelfAux.c_str());
    printlog(LOG_INFO, "bookshelfPl         : %s", IOModule::BookshelfPl.c_str());
    printlog(LOG_INFO, "bookshelfVariety    : %s", IOModule::BookshelfVariety.c_str());
    printlog(LOG_INFO, "lefTech             : %s", IOModule::LefTech.c_str());
    printlog(LOG_INFO, "lefCell             : %s", IOModule::LefCell.c_str());
    printlog(LOG_INFO, "defFloorplan        : %s", IOModule::DefFloorplan.c_str());
    printlog(LOG_INFO, "defCell             : %s", IOModule::DefCell.c_str());
    printlog(LOG_INFO, "defPlacement        : %s", IOModule::DefPlacement.c_str());
    printlog(LOG_INFO, "verilog             : %s", IOModule::Verilog.c_str());
    printlog(LOG_INFO, "liberty             : %s", IOModule::Liberty.c_str());
    printlog(LOG_INFO, "size                : %s", IOModule::Size.c_str());
    printlog(LOG_INFO, "constraints         : %s", IOModule::Constraints.c_str());
}

bool io::IOModule::load() {
    if (BookshelfAux.length() > 0 && BookshelfPl.length()) {
        Format = "bookshelf";
        database.readBSAux(BookshelfAux, BookshelfPl);
    }
    if (LefTech.length() > 0) {
        Format = "lefdef";
        database.readLEF(LefTech);
    }
    if (LefCell.length() > 0) {
        Format = "lefdef";
        database.readLEF(LefCell);
    }
    if (DefCell.length()) {
        Format = "lefdef";
        database.readDEF(DefCell);
        //  database.readDEFPG(DefCell);
    } else if (DefFloorplan.length()) {
        database.readDEF(DefFloorplan);
        //  database.readDEFPG(DefFloorplan);
    }
    if (Verilog.length() > 0) {
        // database.readVerilog(Verilog);
    }
    if (Liberty.length() > 0) {
        database.readLiberty(Liberty);
    }
    if (Size.length() > 0) {
        database.readSize(Size);
    }
    if (Constraints.length() > 0) {
        database.readConstraints(Constraints);
    }
    return true;
}

bool io::IOModule::load(ripple::ShellOptions& options, ripple::ShellCmdReturn& ret) {
    std::string format, file1, file2;
    options.getOption("format", format);
    options.getOption("1", file1);
    options.getOption("2", file2);

    if (format == "lef") {
        database.readLEF(file1);
    } else if (format == "def") {
        database.readDEF(file1);
        //  database.readDEFPG(file1);
    } else if (format == "bookshelf") {
        database.readBSAux(file1, file2);
    } else if (format == "verilog") {
        database.readVerilog(file1);
    } else if (format == "liberty") {
        database.readLiberty(file1);
    } else if (format == "size") {
        database.readSize(file1);
    } else if (format == "constraints") {
        database.readConstraints(file1);
    } else if (format == "") {
        load();
    }
    return true;
}

bool io::IOModule::save() {
    if (Format == "bookshelf" && BookshelfPlacement.size()) {
        database.writeBSPl(BookshelfPlacement);
    } else if (Format == "lefdef" && DefPlacement.size()) {
        if (DefCell.size()) {
            database.writeICCAD2017(DefCell, DefPlacement);
        } else if (DefFloorplan.size()) {
            database.writeICCAD2017(DefFloorplan, DefPlacement);
        }
    }
    return true;
}

bool io::IOModule::save(ripple::ShellOptions& options, ripple::ShellCmdReturn& ret) { return save(); }
