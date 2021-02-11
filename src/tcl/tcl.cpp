#include "tcl.h"

#include "register.h"

using namespace std;
using namespace ripple;

ShellOptions::~ShellOptions() {
    for (ShellOption<bool>* bOption : bOptions) {
        delete bOption;
    }
    for (ShellOption<int>* iOption : iOptions) {
        delete iOption;
    }
    for (ShellOption<unsigned>* uOption : uOptions) {
        delete uOption;
    }
    for (ShellOption<double>* fOption : fOptions) {
        delete fOption;
    }
    for (ShellOption<string>* sOption : sOptions) {
        delete sOption;
    }
}

bool ShellOptions::parse(const std::string& value, bool* option) {
    if (value == "true" || value == "1" || value == "yes") {
        *option = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        *option = false;
        return true;
    }
    return false;
}
bool ShellOptions::parse(const std::string& value, int* option) {
    int v = atoi(value.c_str());
    if (v == 0 && value != "0") {
        return false;
    }
    *option = v;
    return true;
}
bool ShellOptions::parse(const std::string& value, unsigned* option) {
    unsigned v = atoi(value.c_str());
    if (v == 0 && value != "0") {
        return false;
    }
    *option = v;
    return true;
}
bool ShellOptions::parse(const std::string& value, double* option) {
    double v = atof(value.c_str());
    if (v == 0 && value != "0" && value != "0.0") {
        return false;
    }
    *option = v;
    return true;
}
bool ShellOptions::parse(const std::string& value, std::string* option) {
    *option = value;
    return true;
}

void ShellOptions::addOption(const std::string name, bool* value, bool defaultValue) {
    nameMap[name] = make_pair('b', (int)bOptions.size());
    bOptions.push_back(new ShellOption<bool>(value, defaultValue));
}
void ShellOptions::addOption(const std::string name, int* value, int defaultValue) {
    nameMap[name] = make_pair('i', (int)iOptions.size());
    iOptions.push_back(new ShellOption<int>(value, defaultValue));
}
void ShellOptions::addOption(const std::string name, unsigned* value, unsigned defaultValue) {
    nameMap[name] = make_pair('i', (int)uOptions.size());
    uOptions.push_back(new ShellOption<unsigned>(value, defaultValue));
}
void ShellOptions::addOption(const std::string name, double* value, double defaultValue) {
    nameMap[name] = make_pair('f', (int)fOptions.size());
    fOptions.push_back(new ShellOption<double>(value, defaultValue));
}
void ShellOptions::addOption(const std::string name, std::string* value, std::string defaultValue) {
    nameMap[name] = make_pair('s', (int)sOptions.size());
    sOptions.push_back(new ShellOption<std::string>(value, defaultValue));
}

std::string ShellOptions::toString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string ShellOptions::toString(unsigned value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::string ShellOptions::toString(double value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

bool ShellOptions::find(const std::string& name,
                        std::map<std::string, std::pair<char, int> >::const_iterator& mi) const {
    mi = nameMap.find(name);
    if (mi == nameMap.end()) {
        printlog(LOG_INFO, "no such option name : %s", name.c_str());
        return false;
    }
    return true;
}

bool ShellOptions::setOption(const std::string& name, const std::string& value) {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    switch (type) {
        case 'b':
            parse(value, bOptions[index]->value());
            break;
        case 'i':
            parse(value, iOptions[index]->value());
            break;
        case 'u':
            parse(value, uOptions[index]->value());
            break;
        case 'f':
            parse(value, fOptions[index]->value());
            break;
        case 's':
            parse(value, sOptions[index]->value());
            break;
    }
    return true;
}

bool ShellOptions::setOption(const std::string& name, bool value) {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'b') {
        *(bOptions[index]->value()) = value;
        return true;
    }
    return false;
}

bool ShellOptions::setOption(const std::string& name, int value) {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'i') {
        *(iOptions[index]->value()) = value;
        return true;
    }
    return false;
}

bool ShellOptions::setOption(const std::string& name, unsigned value) {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'u') {
        *(uOptions[index]->value()) = value;
        return true;
    }
    return false;
}

bool ShellOptions::setOption(const std::string& name, double value) {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'f') {
        *(fOptions[index]->value()) = value;
        return true;
    }
    return false;
}

bool ShellOptions::getOption(const std::string& name, std::string& value) const {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    switch (type) {
        case 'b':
            value = ShellOptions::toString(*(bOptions[index]->value()));
            return true;
        case 'i':
            value = ShellOptions::toString(*(iOptions[index]->value()));
            return true;
        case 'u':
            value = ShellOptions::toString(*(uOptions[index]->value()));
            return true;
        case 'f':
            value = ShellOptions::toString(*(fOptions[index]->value()));
            return true;
        case 's':
            value = *(sOptions[index]->value());
            return true;
        default:
            return false;
    }
    return false;
}

bool ShellOptions::getOption(const std::string& name, bool& value) const {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'b') {
        value = *(bOptions[index]->value());
        return true;
    }
    return false;
}

bool ShellOptions::getOption(const std::string& name, int& value) const {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'i') {
        value = *(iOptions[index]->value());
        return true;
    }
    return false;
}

bool ShellOptions::getOption(const std::string& name, unsigned& value) const {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'u') {
        value = *(uOptions[index]->value());
        return true;
    }
    return false;
}

bool ShellOptions::getOption(const std::string& name, double& value) const {
    std::map<std::string, std::pair<char, int> >::const_iterator mi;
    if (!find(name, mi)) {
        return false;
    }
    const auto& [type, index] = mi->second;
    if (type == 'f') {
        value = *(fOptions[index]->value());
        return true;
    }
    return false;
}

void ShellOptions::resetOptions() {
    for (auto option : bOptions) {
        option->reset();
    }
    for (auto option : iOptions) {
        option->reset();
    }
    for (auto option : uOptions) {
        option->reset();
    }
    for (auto option : fOptions) {
        option->reset();
    }
    for (auto option : sOptions) {
        option->reset();
    }
}

bool ShellOptions::parseOptions(int argc, char** argv) {
    char anonArg[2] = "0";
    setOption(std::string(anonArg), std::string(argv[0]));
    int anonArgCount = 0;
    for (int i = 1; i < argc; i++) {
        std::string name;
        std::string value;
        if (argv[i][0] == '-' && i < argc - 1) {
            // option
            name = std::string(argv[i] + 1);
            value = std::string(argv[i + 1]);
        } else if (anonArgCount < 9) {
            // anonymous argument
            anonArg[0] = '0' + anonArgCount + 1;
            name = std::string(anonArg);
            value = std::string(argv[i]);
        }
        setOption(name, value);
    }
    return true;
}

bool ShellOptions::parseOptions(std::vector<std::string>& args) {
    char anonArg[2] = "0";
    setOption(std::string(anonArg), args[0]);
    int anonArgCount = 0;
    for (unsigned i = 1; i < args.size(); i++) {
        std::string name;
        std::string value;
        if (args[i][0] == '-' && i < args.size() - 1) {
            // option
            name = args[i].substr(1);
            value = args[i + 1];
        } else if (anonArgCount < 9) {
            // anonymous argument
            anonArg[0] = '0' + anonArgCount + 1;
            name = std::string(anonArg);
            value = args[i];
        }
        setOption(name, value);
    }
    return true;
}

void ShellModule::addArgument(std::string cmd, std::string name, bool* value, bool defaultValue) {
    ShellCommand* sCmd = getCommand(cmd);
    assert(sCmd);
    sCmd->addArgument(name, value, defaultValue);
}

void ShellModule::addArgument(std::string cmd, std::string name, int* value, int defaultValue) {
    ShellCommand* sCmd = getCommand(cmd);
    assert(sCmd);
    sCmd->addArgument(name, value, defaultValue);
}

void ShellModule::addArgument(std::string cmd, std::string name, unsigned* value, unsigned defaultValue) {
    ShellCommand* sCmd = getCommand(cmd);
    assert(sCmd);
    sCmd->addArgument(name, value, defaultValue);
}

void ShellModule::addArgument(std::string cmd, std::string name, double* value, double defaultValue) {
    ShellCommand* sCmd = getCommand(cmd);
    assert(sCmd);
    sCmd->addArgument(name, value, defaultValue);
}

void ShellModule::addArgument(std::string cmd, std::string name, std::string* value, std::string defaultValue) {
    ShellCommand* sCmd = getCommand(cmd);
    assert(sCmd);
    sCmd->addArgument(name, value, defaultValue);
}

/* Shell */

Shell* Shell::_instance = nullptr;

ShellModule* Shell::_addModule(ShellModule* mod) {
    if (moduleMap.find(mod->name()) != moduleMap.end()) {
        return NULL;
    }
    moduleMap[mod->name()] = mod;
    return mod;
}
ShellCommand* Shell::_addCommand(ShellModule* mod, std::string cmd, bool (*cb)(ShellOptions&, ShellCmdReturn&)) {
    if (commandMap.find(cmd) != commandMap.end()) {
        return NULL;
    }
    ShellCommand* sCmd = mod->addCommand(cmd, cb);
    sCmd->addArgument("0", (std::string*)NULL, cmd);
    commandMap[cmd] = sCmd;
    return sCmd;
}

ShellCommand* Shell::_addAlias(std::string alias, std::string cmd) {
    if (commandMap.find(alias) != commandMap.end()) {
        printlog(LOG_ERROR, "alias name used : %s", alias.c_str());
    }
    if (commandMap.find(cmd) == commandMap.end()) {
        printlog(LOG_ERROR, "command not found : %s", cmd.c_str());
    }
    ShellCommand* sCmd = _getCommand(cmd);
    commandMap[alias] = sCmd;
    return sCmd;
}

ShellModule* Shell::_getModule(string mod) {
    map<string, ShellModule*>::iterator mi = moduleMap.find(mod);
    if (mi == moduleMap.end()) {
        return nullptr;
    }
    return mi->second;
}
ShellCommand* Shell::_getCommand(string cmd) {
    map<string, ShellCommand*>::iterator ci = commandMap.find(cmd);
    if (ci == commandMap.end()) {
        return nullptr;
    }
    return ci->second;
}

void Shell::_init() {
    for (pair<const string, ShellModule*>& mod : moduleMap) {
        mod.second->registerCommands();
        mod.second->registerOptions();
    }
}

bool Shell::_proc(std::string cmd, std::vector<std::string>& args) {
    ShellCommand* sCmd = Shell::getCommand(cmd);
    if (sCmd == NULL) {
        printlog(LOG_INFO, "invalid command : %s", cmd.c_str());
        return false;
    }
    sCmd->_arguments.parseOptions(args);
    if (sCmd->callback != NULL) {
        ShellCmdReturn ret;
        if (sCmd->callback(sCmd->_arguments, ret)) {
            return true;
        }
    }
    printlog(LOG_ERROR, "fail in executing %s...", cmd.c_str());
    return false;
}

bool Shell::_proc(std::string cmdargs) {
    std::string cmd;
    std::vector<std::string> args;

    std::string buffer;
    for (unsigned i = 0; i < cmdargs.length(); i++) {
        char c = cmdargs[i];
        bool space = true;
        if (c == '-' || std::isalnum(c)) {
            buffer.push_back(c);
            space = false;
        }
        if (!buffer.empty() && (space || i == cmdargs.length() - 1)) {
            args.push_back(buffer);
            buffer.clear();
        }
    }
    if (args.size() < 1) {
        return false;
    }

    cmd = args[0];
    return _proc(cmd, args);
}

bool Shell::_setOption(std::string mod, std::string name, std::string value) {
    std::map<std::string, ShellModule*>::iterator itr = moduleMap.find(mod);
    if (itr == moduleMap.end()) {
        printlog(
            LOG_INFO, "module [%s] not found for setting option %s = %s", mod.c_str(), name.c_str(), value.c_str());
        return false;
    }
    return itr->second->_options.setOption(name, value);
}

bool Shell::_getOption(std::string mod, std::string name, std::string& value) {
    std::map<std::string, ShellModule*>::iterator itr = moduleMap.find(mod);
    if (itr == moduleMap.end()) {
        printlog(
            LOG_INFO, "module [%s] not found for getting option %s = %s", mod.c_str(), name.c_str(), value.c_str());
        return false;
    }
    return itr->second->_options.getOption(name, value);
}

bool Shell::_showOptions(std::string mod) {
    std::map<std::string, ShellModule*>::iterator itr = moduleMap.find(mod);
    if (itr == moduleMap.end()) {
        printlog(LOG_INFO, "module [%s] not found for showing options", mod.c_str());
        return false;
    }
    itr->second->showOptions();
    return true;
}
