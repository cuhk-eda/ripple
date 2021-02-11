#ifndef _TCL_R_H_
#define _TCL_R_H_

#include <iostream>
#include <list>
#include <string>

typedef int (*cmdProc)(int, const char**);

enum TCL_LINK_TYPE {
    TYPE_INT = 1,
    TYPE_DOUBLE = 2,
    TYPE_BOOLEAN = 3,
    TYPE_CSTRING = 4,
    TYPE_STD_STRING = 100
};

int addTclVariable(const char* varName, void* target, TCL_LINK_TYPE vtype);
int addTclVariable(std::string varName, void* target, TCL_LINK_TYPE vtype);
int addTclCommand(const char* cmdName, cmdProc* cProc);
int addTclCommand(std::string cmdName, cmdProc* cProc);

void clearUserVariable();

typedef struct _VARIABLE {
    char* name;
    void* target;
    int vtype;
} USER_DEFINED_VARIABLE;

typedef struct _COMMAND {
    char* name;
    cmdProc* proc;
    char* info;
} USER_DEFINED_COMMAND;

extern std::list<_COMMAND> user_cmds;
extern std::list<_VARIABLE> user_vars;

#endif
