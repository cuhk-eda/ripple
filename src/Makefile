ifeq ($(origin gd), undefined)
	GD_LIBS =
	GD_OPT =
else
	GD_LIBS = -lgd -lpng16 -lz
	GD_OPT = -DLIBGD
endif

ifeq ($(origin gp), undefined)
	GR_BFILE =
	GP_OBJS =
	GR_OBJS =
	GP_OPT =
else
	GR_BFILE = cugr/bin/iccad19gr gr/visualizer/congest_visualizer.py
	GP_OBJS = gp/gp.o gp/gp_cg.o gp/gp_congest.o gp/gp_data.o gp/gp_draw.o gp/gp_inflate.o gp/gp_qsolve.o gp/gp_spread.o gp/gp_main.o gp/gp_region.o gp/gp_setting.o
	GR_OBJS = gr/gr.o gr/fastroute.o gr/ripplegr.o gr/cugr.o
	GP_OPT = -D__GP__
endif

ifeq ($(origin tcl),undefined)
	TCL_LIBS =
	TCL_PATH =
	TCL_INC =
	TCL_OBJS = tcl/tcl.o
else
	TCL_LIBS = -ltcl
	TCL_PATH=/usr/include/tcl8.6
	TCL_INC = -I$(TCL_PATH)
	TCL_OBJS = tcl/tcl.o tcl/register.o
endif

LEMON_PATH=./lemon
LEMON_INC = -I$(LEMON_PATH)/include
LEMON_LIBS = $(LEMON_PATH)/lib/libemon.a

ifeq ($(mode),asan)
	OPT = -fsanitize=address -O0 -g $(EXP_OPT) $(GP_OPT)
	LIBS = -pthread -ldl $(TCL_LIBS)
	CXXFLAG = -std=c++17
else
ifeq ($(mode),debug)
	OPT= -O0 -ggdb -DDEBUG $(EXP_OPT) $(GD_OPT) $(GP_OPT)
	LIBS = -pthread -ldl $(GD_LIBS) $(TCL_LIBS)
	CXXFLAG = -std=c++17
else
ifeq ($(mode),profile)
	OPT= -O3 -pg $(EXP_OPT) $(GP_OPT)
	LIBS = -pthread -ldl $(TCL_LIBS)
	CXXFLAG = -std=c++17
else
ifeq ($(mode),release)
	OPT= -O3 -DNDEBUG $(GD_OPT) $(GP_OPT)
	LIBS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -ldl $(GD_LIBS) $(TCL_LIBS)
	CXXFLAG = -std=c++17 -static
else
ifeq ($(mode),tsan)
	OPT = -fsanitize=thread -O0 -g $(EXP_OPT) $(GP_OPT)
	LIBS = -pthread -ldl  $(TCL_LIBS)
	CXXFLAG = -std=c++17
else
	OPT= -O3 $(GD_OPT) $(EXP_OPT) $(GP_OPT)
	LIBS = -pthread -ldl $(GD_LIBS) $(TCL_LIBS)
	CXXFLAG = -std=c++17
endif
endif
endif
endif
endif

WFLAG    = -Wall -Werror
INC      = $(TCL_INC) $(LEMON_INC) -I.

ifeq ($(origin target),undefined)
	target = bin
endif

ifeq ($(BISON),)
	ifeq ($(shell which bison),)
		BISON_ERROR = $(error "No bison in $(PATH), consider doing `apt install bison`")
	else
		BISON = bison
	endif
endif

ifeq ($(FLEX),)
	ifeq ($(shell which flex),)
		FLEX_ERROR = $(error "No flex in $(PATH), consider doing `apt install flex`")
	else
		FLEX = flex
	endif
endif

AR          = ar rcs
CXXPROC     = $(CXX) $(OPT) $(WFLAG) $(CXXFLAG) $(INC)
TAG         = ctags

CC_OBJS = main.o
DB_OBJS = db/db.o db/db_cell.o db/db_layer.o db/db_net.o db/db_pin.o db/db_region.o db/db_drc.o db/db_geom.o db/db_route.o db/db_get.o db/db_set.o db/db_add.o db/db_map.o db/db_check.o db/db_place.o
DP_OBJS = dp/dp.o dp/dp_data.o dp/dp_datalocal.o dp/dp_dataglobal.o dp/dp_draw.o dp/dp_move.o dp/dp_movemll.o dp/dp_movesr.o dp/dp_form.o dp/dp_place.o
IO_OBJS = io/file_bkshf_db.o io/file_constraints.o io/file_lefdef_db.o io/file_liberty.tab.o io/file_liberty.yy.o io/utils.o io/io.o io/file_verilog.o
UT_OBJS = ut/log.o ut/timer.o ut/utils.o
STA_OBJS = sta/lib.o sta/sta.o
VI_OBJS = vi/vi.o vi/draw.o vi/item_pos.o vi/vi_db.o
RIPPLE_OBJS = ripple/ripple.o 
LIB_OBJS = def58/lib/libdef.a lef58/lib/liblef.a $(LEMON_LIBS)

OBJS = $(CC_OBJS) $(DB_OBJS) $(DP_OBJS) $(GP_OBJS) $(GR_OBJS) $(IO_OBJS) $(RIPPLE_OBJS) $(STA_OBJS) $(TCL_OBJS) $(UT_OBJS) $(VI_OBJS)

SRCS = ${OBJS:%.o=%.cpp}
DEPS = ${OBJS:%.o=%.d}
BFILE = placer
AFILE = ripple.a

define copy_build
	mkdir -p ../$(target)
	cp -u $(BFILE) $(GR_BFILE) ../$(target)/
endef

.PHONY: all
all: err $(BFILE)

.PHONY: install
install: $(BFILE)
	$(call copy_build)

.PHONY: db
db: $(DB_OBJS)
	$(CXXPROC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)
.PHONY: dp
dp: $(DP_OBJS)
	$(CXXPROC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)
.PHONY: gp
gp: $(GP_OBJS)
	$(CXXPROC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)
.PHONY: io
io: $(IO_OBJS)
	$(CXXPROC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)
.PHONY: tcl
tcl: $(TCL_OBJS)
	$(CXXPROC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)

err:
	$(BISON_ERROR)

$(BFILE): $(OBJS)
	$(CXXPROC) -o $(BFILE) $(OBJS) $(LIB_OBJS) $(LIBS)
	$(call copy_build)

$(AFILE): $(AR_OBJS) $(DP_OBJS) $(UT_OBJS)
	$(AR) $(AFILE) $(AR_OBJS) $(DP_OBJS) $(UT_OBJS)
	cp $(AFILE) ../$(target)/$(AFILE)

io/%.tab.o: io/%.y
	$(BISON) -v -p$* --defines=io/$*.tab.h --output=io/$*.tab.cpp io/$*.y
	$(CXXPROC) -o io/$*.tab.o -c io/$*.tab.cpp

io/%.yy.o: io/%.l io/%.tab.o
	$(FLEX) -P$* -oio/$*.yy.cpp io/$*.l
	$(CXXPROC) -o io/$*.yy.o -c io/$*.yy.cpp

%.o : %.cpp
	$(CXXPROC) -o $*.o -c $*.cpp

.PHONY: clean
clean:
	rm -f */*.o *.o
	rm -f */*.d *.d
	rm -f io/*.tab.cpp io/*.tab.h io/*.yy.cpp io/*.output
	rm -f *.dat $(BFILE) $(AFILE) core

.PHONY: tags
tags:
	cscope -Rbq
	ctags -R

