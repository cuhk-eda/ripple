#ifndef _TCL_H_
#define _TCL_H_

#include "../global.h"

#include <climits>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

/*
///////////////////////
// example of module
///////////////////////
class TestSetting{
    public:
        static int a;
        static int b;
};

int TestSetting::a;
int TestSetting::b;
class TestModule : public ShellModule{
    private:
        void registerCommands(){
            Shell::addCommand(this, "plus", TestModule::plus);
            Shell::addCommand(this, "minus", TestModule::minus);
            Shell::addCommand(this, "times", TestModule::times);
            Shell::addAlias("sum", "plus");
        }
        void registerOptions(){
            Shell::addOption(this, "a", &TestSetting::a, 0);
            Shell::addOption(this, "b", &TestSetting::b, 0);
            Shell::addArgument(this, "plus", "a", (int*)NULL, 0);
            Shell::addArgument(this, "plus", "b", (int*)NULL, 0);
        }
    public:
        void showOptions() const {
            printlog(LOG_INFO, "a   : %d", TestSetting::a);
            printlog(LOG_INFO, "b   : %d", TestSetting::b);
        }
        TestModule(){ }
        ~TestModule(){ }
        std::string name() const {
            return "test";
        }
        static bool plus(ShellOptions &args, ShellCmdReturn &ret){
            ret.value(TestSetting::a + TestSetting::b);
            return true;
        }
        static bool minus(ShellOptions &args, ShellCmdReturn &ret){
            ret.value(TestSetting::a - TestSetting::b);
            return true;
        }
        static bool times(ShellOptions &args, ShellCmdReturn &ret){
            ret.value(TestSetting::a * TestSetting::b);
            return true;
        }
};
*/

void startTCL(int argc, char* argv[]);
void startTclEval();

void initTclEval();
void execCommand(std::string cmd);
int execCommandwithResult(std::string cmd);
int execCommandwithResult(std::string cmd, std::string& rmsg);

namespace ripple {
class ShellCommand;
class ShellCmdReturn;
class ShellModule;
class Shell;

template <typename T>
class ShellOption {
private:
    bool _selfOwn;
    T* _value;
    T _defaultValue;

public:
    T* value() const
    {
        return _value;
    }
    void reset()
    {
        *_value = _defaultValue;
    }
    ShellOption(T* v, T d)
    {
        _value = (v == NULL) ? (new T) : v;
        _selfOwn = (v == NULL);
        *_value = d;
        _defaultValue = d;
    }
    ~ShellOption()
    {
        if (_selfOwn) {
            delete _value;
        }
    }
};

class ShellOptions {
    friend class ShellModule;

public:
    enum Type {
        Bool,
        Int,
        Float,
        String
    };

private:
    std::vector<ShellOption<bool>*> bOptions;
    std::vector<ShellOption<int>*> iOptions;
    std::vector<ShellOption<unsigned>*> uOptions;
    std::vector<ShellOption<double>*> fOptions;
    std::vector<ShellOption<std::string>*> sOptions;
    std::map<std::string, std::pair<char, int> > nameMap;
    static bool parse(const std::string& value, bool* option);
    static bool parse(const std::string& value, int* option);
    static bool parse(const std::string& value, unsigned* option);
    static bool parse(const std::string& value, double* option);
    static bool parse(const std::string& value, std::string* option);
    bool find(const std::string& name, std::map<std::string, std::pair<char, int> >::const_iterator& mi) const;

public:
    ~ShellOptions();
    void addOption(const std::string name, bool* value, bool defaultValue);
    void addOption(const std::string name, int* value, int defaultValue);
    void addOption(const std::string name, unsigned* value, unsigned defaultValue);
    void addOption(const std::string name, double* value, double defaultValue);
    void addOption(const std::string name, std::string* value, std::string defaultValue);

    static inline string toString(bool value) { return value ? string("true") : string("false"); }
    static string toString(int value);
    static string toString(unsigned value);
    static string toString(double value);
    static inline string toString(string value) { return value; }

    bool setOption(const std::string& name, const std::string& value);
    bool setOption(const std::string& name, bool value);
    bool setOption(const std::string& name, int value);
    bool setOption(const std::string& name, unsigned value);
    bool setOption(const std::string& name, double value);
    bool getOption(const std::string& name, std::string& value) const;
    bool getOption(const std::string& name, bool& value) const;
    bool getOption(const std::string& name, int& value) const;
    bool getOption(const std::string& name, unsigned& value) const;
    bool getOption(const std::string& name, double& value) const;

    inline map<string, pair<char, int> >* getNames() { return &nameMap; }

    void resetOptions();
    bool parseOptions(int argc, char** argv);
    bool parseOptions(std::vector<std::string>& args);
};

class ShellCommand {
    friend class Shell;

private:
    ShellModule* _module;
    ShellOptions _arguments;
    std::string _name;
    std::string _help;
    bool (*callback)(ShellOptions&, ShellCmdReturn&);

public:
    ShellCommand(ShellModule* mod, std::string name, bool (*cb)(ShellOptions&, ShellCmdReturn&))
    {
        _module = mod;
        _name = name;
        callback = cb;
    }
    void addArgument(std::string name, bool* value, bool defaultValue)
    {
        _arguments.addOption(name, value, defaultValue);
    }
    void addArgument(std::string name, int* value, int defaultValue)
    {
        _arguments.addOption(name, value, defaultValue);
    }
    void addArgument(std::string name, unsigned* value, unsigned defaultValue)
    {
        _arguments.addOption(name, value, defaultValue);
    }
    void addArgument(std::string name, double* value, double defaultValue)
    {
        _arguments.addOption(name, value, defaultValue);
    }
    void addArgument(std::string name, std::string* value, std::string defaultValue)
    {
        _arguments.addOption(name, value, defaultValue);
    }
    std::string name() const { return _name; }
    std::string help() const { return _help; }
};
class ShellCmdReturn {
public:
    enum ErrorCode {
        NoError = 0,
        WrongNumArgs = 1,
        WrongArgType = 2,
        InvalidArgValue = 3
    };

private:
    ErrorCode _error;
    char* _value;

public:
    ShellCmdReturn()
    {
        _error = NoError;
        _value = NULL;
    }
    ~ShellCmdReturn()
    {
        if (_value) {
            delete _value;
        }
    }
    void value(int val)
    {
        std::string str = ShellOptions::toString(val);
        _value = new char[str.length() + 1];
        strncpy(_value, str.c_str(), str.length() + 1);
    }
    void value(double val)
    {
        std::string str = ShellOptions::toString(val);
        _value = new char[str.length() + 1];
        strncpy(_value, str.c_str(), str.length() + 1);
    }
    void value(bool val)
    {
        std::string str = ShellOptions::toString(val);
        _value = new char[str.length() + 1];
        strncpy(_value, str.c_str(), str.length() + 1);
    }
    void error(ErrorCode code)
    {
        _error = code;
    }
    char* value() const
    {
        return _value;
    }
    ErrorCode error() const
    {
        return _error;
    }
};

class ShellModule {
    friend class Shell;

private:
    std::vector<ShellCommand*> _commands;
    std::map<std::string, ShellCommand*> _commandMap;
    ShellOptions _options;

protected:
    virtual void registerCommands() = 0;
    virtual void registerOptions() = 0;

    ShellCommand* addCommand(std::string cmd, bool (*cb)(ShellOptions&, ShellCmdReturn&))
    {
        ShellCommand* sCmd = new ShellCommand(this, cmd, cb);
        _commands.push_back(sCmd);
        _commandMap[cmd] = sCmd;
        return sCmd;
    }
    ShellCommand* getCommand(std::string cmd)
    {
        std::map<std::string, ShellCommand*>::iterator itr = _commandMap.find(cmd);
        if (itr == _commandMap.end()) {
            return NULL;
        }
        return itr->second;
    }
    void addOption(std::string name, bool* value, bool defaultValue)
    {
        _options.addOption(name, value, defaultValue);
    }
    void addOption(std::string name, int* value, int defaultValue)
    {
        _options.addOption(name, value, defaultValue);
    }
    void addOption(std::string name, unsigned* value, unsigned defaultValue)
    {
        _options.addOption(name, value, defaultValue);
    }
    void addOption(std::string name, double* value, double defaultValue)
    {
        _options.addOption(name, value, defaultValue);
    }
    void addOption(std::string name, std::string* value, std::string defaultValue)
    {
        _options.addOption(name, value, defaultValue);
    }

    void addArgument(std::string cmd, std::string name, bool* value, bool defaultValue);
    void addArgument(std::string cmd, std::string name, int* value, int defaultValue);
    void addArgument(std::string cmd, std::string name, unsigned* value, unsigned defaultValue);
    void addArgument(std::string cmd, std::string name, double* value, double defaultValue);
    void addArgument(std::string cmd, std::string name, std::string* value, std::string defaultValue);

public:
    virtual const std::string& name() const = 0;
    virtual void showOptions() const = 0;
    ShellOptions* getOptions()
    {
        return &_options;
    }
    std::vector<ShellCommand*>* getCommands()
    {
        return &_commands;
    }
};

class Shell {
private:
    static Shell* _instance;
    std::map<std::string, ShellCommand*> commandMap;
    std::map<std::string, ShellModule*> moduleMap;

    std::vector<std::string> commands;

    bool alive;

    Shell() { alive = true; }
    ~Shell() {}
    ShellModule* _addModule(ShellModule* module);
    ShellCommand* _addCommand(ShellModule* module, std::string cmd, bool (*cb)(ShellOptions&, ShellCmdReturn&));
    ShellCommand* _addAlias(std::string alias, std::string cmd);

    ShellModule* _getModule(std::string mod);
    ShellCommand* _getCommand(std::string cmd);

    void _init();

    inline void _kill() { alive = false; }

    bool _proc(std::string cmd, std::vector<std::string>& args);
    bool _proc(std::string cmd);
    bool _setOption(std::string module, std::string name, std::string value);
    bool _getOption(std::string module, std::string name, std::string& value);
    bool _showOptions(std::string module);

public:
    static Shell* get()
    {
        if (Shell::_instance == NULL) {
            Shell::_instance = new Shell;
        }
        return Shell::_instance;
    }

    static std::vector<std::string>& getCommands()
    {
        return get()->commands;
    }
    static void addOption(ShellModule* mod, std::string name, bool* value)
    {
        mod->addOption(name, value, *value);
    }
    static void addOption(ShellModule* mod, std::string name, int* value)
    {
        mod->addOption(name, value, *value);
    }
    static void addOption(ShellModule* mod, std::string name, unsigned* value)
    {
        mod->addOption(name, value, *value);
    }
    static void addOption(ShellModule* mod, std::string name, double* value)
    {
        mod->addOption(name, value, *value);
    }
    static void addOption(ShellModule* mod, std::string name, std::string* value)
    {
        mod->addOption(name, value, *value);
    }

    static void addOption(ShellModule* mod, std::string name, bool* value, bool defaultValue)
    {
        mod->addOption(name, value, defaultValue);
    }
    static void addOption(ShellModule* mod, std::string name, int* value, int defaultValue)
    {
        mod->addOption(name, value, defaultValue);
    }
    static void addOption(ShellModule* mod, std::string name, unsigned* value, int defaultValue)
    {
        mod->addOption(name, value, defaultValue);
    }
    static void addOption(ShellModule* mod, std::string name, double* value, double defaultValue)
    {
        mod->addOption(name, value, defaultValue);
    }
    static void addOption(ShellModule* mod, std::string name, std::string* value, std::string defaultValue)
    {
        mod->addOption(name, value, defaultValue);
    }

    static void addArgument(ShellModule* mod, std::string cmd, std::string name, bool* value, bool defaultValue)
    {
        mod->addArgument(cmd, name, value, defaultValue);
    }
    static void addArgument(ShellModule* mod, std::string cmd, std::string name, int* value, int defaultValue)
    {
        mod->addArgument(cmd, name, value, defaultValue);
    }
    static void addArgument(ShellModule* mod, std::string cmd, std::string name, unsigned* value, unsigned defaultValue)
    {
        mod->addArgument(cmd, name, value, defaultValue);
    }
    static void addArgument(ShellModule* mod, std::string cmd, std::string name, double* value, double defaultValue)
    {
        mod->addArgument(cmd, name, value, defaultValue);
    }
    static void addArgument(ShellModule* mod, std::string cmd, std::string name, std::string* value, std::string defaultValue)
    {
        mod->addArgument(cmd, name, value, defaultValue);
    }

    static ShellModule* addModule(ShellModule* module) { return get()->_addModule(module); }

    static ShellCommand* addCommand(ShellModule* module, std::string cmd, bool (*cb)(ShellOptions&, ShellCmdReturn&))
    {
        return get()->_addCommand(module, cmd, cb);
    }

    static ShellCommand* addAlias(std::string alias, std::string cmd) { return get()->_addAlias(alias, cmd); }

    static inline ShellModule* getModule(std::string mod) { return get()->_getModule(mod); }
    static inline ShellCommand* getCommand(std::string cmd) { return get()->_getCommand(cmd); }
    static inline void init() { get()->_init(); }

    static inline void kill() { get()->_kill(); }

    static bool proc(std::string cmd, std::vector<std::string>& args) { return get()->_proc(cmd, args); }

    static bool proc(std::string cmd)
    {
        return get()->_proc(cmd);
    }
    static bool setOption(std::string module, std::string name, std::string value)
    {
        return get()->_setOption(module, name, value);
    }

    static inline bool getOption(std::string module, std::string name, std::string& value) { return get()->_getOption(module, name, value); }
    static inline bool showOptions(std::string module) { return get()->_showOptions(module); }
};
};

#endif
