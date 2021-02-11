#include <cstring>
#include "tcl.h"
#include "register.h"

std::list<_VARIABLE> user_vars;

char* tcl_var_trace(ClientData data, Tcl_Interp *interp, char *name1, char *name2, int flags) {
    USER_DEFINED_VARIABLE *var=(USER_DEFINED_VARIABLE *)data;
    Tcl_Obj *v_obj=Tcl_GetVar2Ex(interp, name1, name2, TCL_GLOBAL_ONLY);

    if (flags&TCL_TRACE_READS) {
        if (var->vtype==TYPE_INT) {
            int v_cur;
            Tcl_GetIntFromObj(interp, v_obj, &v_cur);
            if (v_cur!=*((int *)var->target)) {
                //Tcl_AppendPrintfToObj(v_obj, "%d", *((int *)var->target));
                Tcl_SetIntObj(v_obj, *((int *)var->target));
            }
        }
        else if (var->vtype==TYPE_DOUBLE) {
            double v_cur;
            Tcl_GetDoubleFromObj(interp, v_obj, &v_cur);
            if (v_cur!=*((double *)var->target)) {
                //Tcl_AppendPrintfToObj(v_obj, "%lf", *((double *)var->target));
                Tcl_SetDoubleObj(v_obj, *((double *)var->target));
            }
        }
        else if (var->vtype==TYPE_BOOLEAN) {
            bool v_cur;
            Tcl_GetBooleanFromObj(interp, v_obj, (int *)&v_cur);
            if (v_cur!=*((bool *)var->target)) {
                //Tcl_AppendPrintfToObj(v_obj, "%d", *((bool *)var->target));
                Tcl_SetBooleanObj(v_obj, *((bool *)var->target));
            }
        }
        else if (var->vtype==TYPE_STD_STRING) {
            char *v_cur=Tcl_GetString(v_obj);
            if (((std::string *)var->target)->compare(v_cur)!=0) {
                Tcl_AppendPrintfToObj(v_obj, "%s", ((std::string *)var->target)->c_str());
            }
        }
        else if (var->vtype==TYPE_CSTRING) {
            char *v_cur=Tcl_GetString(v_obj);
            if (strcmp((char *)var->target, v_cur)!=0) {
                Tcl_AppendPrintfToObj(v_obj, "%s", ((char *)var->target));
            }
        }
    }
    else if (flags&TCL_TRACE_WRITES) {
        if (var->vtype==TYPE_INT) {
            //Tcl_GetIntFromObj(interp, v_obj, (int *)var->target);
            if (Tcl_GetIntFromObj(interp, v_obj, (int *)var->target)==TCL_ERROR) {
                printf("Write error: %s\n", Tcl_GetStringResult(interp));
                if (Tcl_IsShared(v_obj)) {
                    Tcl_DecrRefCount(v_obj);
                    Tcl_SetIntObj(v_obj, *((int *)var->target));
                    Tcl_IncrRefCount(v_obj);
                }
            }
        }
        else if (var->vtype==TYPE_DOUBLE) {
            //Tcl_GetDoubleFromObj(interp, v_obj, (double *)var->target);
            if (Tcl_GetDoubleFromObj(interp, v_obj, (double *)var->target)==TCL_ERROR) {
                printf("Write error: %s\n", Tcl_GetStringResult(interp));
                if (Tcl_IsShared(v_obj)) {
                    Tcl_DecrRefCount(v_obj);
                    Tcl_SetDoubleObj(v_obj, *((double *)var->target));
                    Tcl_IncrRefCount(v_obj);
                }
            }
        }
        else if (var->vtype==TYPE_BOOLEAN) {
            //Tcl_GetBooleanFromObj(interp, v_obj, (int *)var->target);
            if (Tcl_GetBooleanFromObj(interp, v_obj, (int *)var->target)==TCL_ERROR) {
                printf("Write error: %s\n", Tcl_GetStringResult(interp));
                if (Tcl_IsShared(v_obj)) {
                    Tcl_DecrRefCount(v_obj);
                    Tcl_SetBooleanObj(v_obj, *((int *)var->target));
                    Tcl_IncrRefCount(v_obj);
                }
            }
        }
        else if (var->vtype==TYPE_STD_STRING) {
            ((std::string *)var->target)->assign(Tcl_GetString(v_obj));
        }
        else if (var->vtype==TYPE_CSTRING) {
            int vlen;
            char *vstr=Tcl_GetStringFromObj(v_obj, &vlen);
            if (vlen>(int)strlen((char *)var->target)) {
                if (var->target!=NULL) free(var->target);
                var->target=malloc((vlen+1)*sizeof(char));
            }
            strcpy((char *)var->target, vstr);
        }
    }

    return NULL;
}

int addTclVariable(const char *varName, void *target, TCL_LINK_TYPE vtype) {
    if (vtype!=TYPE_CSTRING && (vtype>0 && vtype<15)) {
        Tcl_LinkVar(winterp, varName, (char*)(target), vtype);
        //user_vars.emplace_back(varName,vtype);
        _VARIABLE var;
        var.name = (char*)malloc(sizeof(char)*(strlen(varName)+1));
        strcpy(var.name, varName);
        var.target = target;
        var.vtype = vtype;
        user_vars.push_back(var);
        return 0;
    }
    
    int vlen=strlen(varName);
    Tcl_Obj *newVal=Tcl_NewObj();
    if (Tcl_SetVar2Ex(winterp, varName, NULL, newVal, TCL_GLOBAL_ONLY)==NULL) {
        return -1;
    }
    
    USER_DEFINED_VARIABLE *var;
    var=(USER_DEFINED_VARIABLE *)Tcl_Alloc(sizeof(USER_DEFINED_VARIABLE));
    var->name=(char *)malloc((vlen+1)*sizeof(char));
    strcpy(var->name, varName);
    var->target=target;
    var->vtype=vtype;
    Tcl_TraceVar(winterp,
            varName, (TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_GLOBAL_ONLY),
            (Tcl_VarTraceProc *)tcl_var_trace, (ClientData)var);

    //user_vars.emplace_back(varName,vtype);
    return 0;
}

void UserVarFree(char *blockPtr) {
    USER_DEFINED_VARIABLE *var=(USER_DEFINED_VARIABLE *)blockPtr;

    if (var->name!=NULL) {
        free(var->name);
    }
    var->target=NULL;
    var->vtype=0;
}
void removeTclVariable(char *varName) {
    ClientData prev_data, data;

    data=Tcl_VarTraceInfo(winterp,
            varName, (TCL_GLOBAL_ONLY),
            (Tcl_VarTraceProc *)tcl_var_trace, NULL);
    while (data) {
        Tcl_UntraceVar(winterp,
                varName, (TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_GLOBAL_ONLY),
                (Tcl_VarTraceProc *)tcl_var_trace, data);

        prev_data=data;
        data=Tcl_VarTraceInfo(winterp,
                varName, (TCL_GLOBAL_ONLY),
                (Tcl_VarTraceProc *)tcl_var_trace, prev_data);
        
        UserVarFree((char *)prev_data);
        Tcl_Free((char *)prev_data);
    }
}
void clearUserVariable() {
    std::list<_VARIABLE>::iterator vari=user_vars.begin();
    std::list<_VARIABLE>::iterator vare=user_vars.end();
    for (; vari!=vare; ++vari) {
        char *name = vari->name;
        int vtype = vari->vtype;
        if (vtype!=TYPE_CSTRING && (vtype>0 && vtype<15)) {
            Tcl_UnlinkVar(winterp, name);
        }
        else {
            removeTclVariable(name);
        }
    }
}


std::list<_COMMAND> user_cmds;


void UserCmdFree(char *blockPtr) {
    USER_DEFINED_COMMAND *cmd=(USER_DEFINED_COMMAND *)blockPtr;

    if (cmd->name!=NULL) {
        free((char*)cmd->name);
    }
    cmd->proc=NULL;
    cmd->info=NULL;
}

int tcl_run_proc(ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    if (objc<1) return TCL_ERROR;
    
    cmdProc func=(cmdProc)data;
    if (func!=NULL) {
        char **arg=(char **)malloc(objc*sizeof(char*));
        for (int i=0; i<objc; ++i) {
            int slen=0;
            char *str=Tcl_GetStringFromObj(objv[i], &slen);
            arg[i]=(char *)malloc((slen+1)*sizeof(char));
            strcpy(arg[i], str);
        }

        int ret = (*func)(objc, (const char **)arg);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(ret));
        
        for (int i=0; i<objc; ++i) {
            free(arg[i]);
        }
        free(arg);
    }

    return TCL_OK;
}
int addTclCommand(const char *cmdName, cmdProc *cProc) {
    Tcl_CreateObjCommand(winterp,
            cmdName, (Tcl_ObjCmdProc *)tcl_run_proc,
            (ClientData)cProc, (Tcl_CmdDeleteProc *)NULL);
    
    _COMMAND cmd;
    cmd.name = (char*)malloc(sizeof(char)*(strlen(cmdName)+1));
    cmd.proc = cProc;
    cmd.info = NULL;
    user_cmds.push_back(cmd);
    
    return 0;
}

int addTclVariable(std::string varName, void *target, TCL_LINK_TYPE vtype) {
    return addTclVariable(varName.c_str(), target, vtype);
}
int addTclCommand(std::string cmdName, cmdProc *cProc) {
    return addTclCommand(cmdName.c_str(), cProc);
}
