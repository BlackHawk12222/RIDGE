################################################################################
######################### User configurable parameters #########################

PYTHON=python3

PY_SOURCES := $(wildcard py/*.py)
FROZEN_PY_HEADER := include/frozen_py.h

$(FROZEN_PY_HEADER): $(PY_SOURCES) tools/pack_py.py
	$(PYTHON) tools/pack_py.py


# filename extensions
CEXTS:=c
ASMEXTS:=s S
CXXEXTS:=cpp c++ cc

# probably shouldn't modify these, but you may need them below
ROOT=.
FWDIR:=$(ROOT)/firmware
BINDIR=$(ROOT)/bin
SRCDIR=$(ROOT)/src
INCDIR=$(ROOT)/include

WARNFLAGS+=
EXTRA_CFLAGS=
EXTRA_CXXFLAGS=

# Set to 1 to enable hot/cold linking
USE_PACKAGE:=0  # Disabled to work wioth the .a src file.

# Add libraries you do not wish to include in the cold image here
EXCLUDE_COLD_LIBRARIES:= 

# Set this to 1 to add additional rules to compile your project as a PROS library template
IS_LIBRARY:=0
LIBNAME:=libbest
VERSION:=1.0.0

EXCLUDE_SRC_FROM_LIB+=$(foreach file, $(SRCDIR)/main,$(foreach cext,$(CEXTS),$(file).$(cext)) $(foreach cxxext,$(CXXEXTS),$(file).$(cxxext)))

TEMPLATE_FILES=$(INCDIR)/$(LIBNAME)/*.h $(INCDIR)/$(LIBNAME)/*.hpp

.DEFAULT_GOAL=quick

################################################################################
# MicroPython Header Paths
################################################################################

MPY_ROOT := src/micropython

EXTRA_CFLAGS += -I$(BINDIR)/micropython \
                -I$(MPY_ROOT) \
                -I$(MPY_ROOT)/py \
                -I$(MPY_ROOT)/extmod \
                -I$(MPY_ROOT)/modules \
                -Isrc/vex \
                -O3 -funroll-loops -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=hard

EXTRA_CXXFLAGS += -I$(BINDIR)/micropython \
                  -I$(MPY_ROOT) \
                  -I$(MPY_ROOT)/py \
                  -I$(MPY_ROOT)/extmod \
                  -I$(MPY_ROOT)/modules \
                  -Isrc/vex \
                  -O3 -funroll-loops -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=hard

################################################################################
# Full MicroPython Source & QSTR Build (Only active when COMPILE_ALL=1)
################################################################################
ifeq ($(COMPILE_ALL),1)
EXTRA_SRCDIRS += $(MPY_ROOT) \
                 $(MPY_ROOT)/py \
                 $(MPY_ROOT)/shared/runtime \
                 $(MPY_ROOT)/modules

EXCLUDE_SRC_FROM_LIB += $(MPY_ROOT)/shared/runtime/pyexec.c \
                        $(MPY_ROOT)/py/builtinhelp.c
endif

# Normal Mode: Compile ONLY src/main.cpp and link firmware/libmpy.a
ifneq ($(COMPILE_ALL),1)
CSRC   :=
ASMSRC :=
CXXSRC := $(SRCDIR)/main.cpp

COBJ   :=
ASMOBJ :=
CXXOBJ := $(BINDIR)/main.cpp.o
endif

# Ensure frozen_py.h is generated before PROS compiles C/C++ objects
$(CXXOBJ) $(COBJ): $(FROZEN_PY_HEADER)


################################################################################
# Normal Mode: Compile ONLY src/main.cpp and link firmware/libmpy.a
################################################################################
ifneq ($(COMPILE_ALL),1)
CSRC   :=
ASMSRC :=
CXXSRC := $(SRCDIR)/main.cpp

COBJ   :=
ASMOBJ :=
CXXOBJ := $(BINDIR)/main.cpp.o
endif

################################################################################
# Full Rebuild Mode: Ignore old libmpy.a and generate QSTRs for everything
################################################################################
ifeq ($(COMPILE_ALL),1)
WL_LIBRARIES := $(filter-out %/libmpy.a,$(WL_LIBRARIES))
LIBAR := $(filter-out %/libmpy.a,$(LIBAR))

# Explicit prerequisite on all object variables
$(COBJ) $(CXXOBJ) $(ASMOBJ): $(MP_QSTR_HEADER) $(MP_MODULE_HEADER) $(MP_ROOT_POINTER_HEADER)

# Pattern rule order-only prerequisite so PROS's pattern recipes MUST wait for QSTR headers
$(BINDIR)/%.o: | $(MP_QSTR_HEADER) $(MP_MODULE_HEADER) $(MP_ROOT_POINTER_HEADER)

MP_QSTR_SOURCES = $(sort $(call rwildcard,$(MPY_ROOT)/,*.c) src/mp_port.c src/modpros.c src/micropython/modules/vex_module.c)
MP_QSTR_GLOBAL_DEPS := $(MPY_ROOT)/mpconfigport.h \
                       $(MPY_ROOT)/py/mpconfig.h \
                       $(MPY_ROOT)/py/qstrdefs.h
MP_QSTR_BUILD := $(BINDIR)/micropython/qstrgen
MP_QSTR_FILES := $(MP_QSTR_BUILD)/qstr
MP_MODULE_FILES := $(MP_QSTR_BUILD)/module
MP_ROOT_POINTER_FILES := $(MP_QSTR_BUILD)/root_pointer
MP_QSTR_CPP := $(MP_QSTR_BUILD)/qstr.i.last
MP_QSTR_SPLIT := $(MP_QSTR_BUILD)/qstr.split
MP_QSTR_COLLECTED := $(MP_QSTR_BUILD)/qstrdefs.collected.h
MP_QSTR_PREPROCESSED := $(MP_QSTR_BUILD)/qstrdefs.preprocessed.h
MP_QSTR_HEADER := $(BINDIR)/micropython/genhdr/qstrdefs.generated.h
MP_MODULE_SPLIT := $(MP_QSTR_BUILD)/moduledefs.split
MP_MODULE_COLLECTED := $(MP_QSTR_BUILD)/moduledefs.collected
MP_MODULE_HEADER := $(BINDIR)/micropython/genhdr/moduledefs.h
MP_ROOT_POINTER_SPLIT := $(MP_QSTR_BUILD)/root_pointers.split
MP_ROOT_POINTER_COLLECTED := $(MP_QSTR_BUILD)/root_pointers.collected
MP_ROOT_POINTER_HEADER := $(BINDIR)/micropython/genhdr/root_pointers.h
MP_QSTR_OBJECTS := $(filter $(BINDIR)/micropython/%,$(COBJ)) \
                   $(BINDIR)/mp_port.c.o \
                   $(BINDIR)/modpros.c.o

$(CXXOBJ): $(MP_QSTR_HEADER) $(MP_MODULE_HEADER) $(MP_ROOT_POINTER_HEADER)
quick: $(MP_QSTR_HEADER) $(MP_MODULE_HEADER) $(MP_ROOT_POINTER_HEADER)
$(MP_QSTR_OBJECTS): $(MP_QSTR_HEADER) $(MP_MODULE_HEADER) $(MP_ROOT_POINTER_HEADER)

$(MP_QSTR_CPP): $(MP_QSTR_SOURCES) $(MP_QSTR_GLOBAL_DEPS) Makefile common.mk
	mkdir -p $(MP_QSTR_BUILD)
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py pp $(CC) -E output $@ \
		cflags -DNO_QSTR $(INCLUDE) $(CFLAGS) $(EXTRA_CFLAGS) \
		cxxflags -DNO_QSTR $(INCLUDE) $(CXXFLAGS) $(EXTRA_CXXFLAGS) \
		sources $(MP_QSTR_SOURCES) \
		changed_sources $(MP_QSTR_SOURCES) $(MP_QSTR_GLOBAL_DEPS) \
		dependencies $(MP_QSTR_GLOBAL_DEPS)

$(MP_QSTR_SPLIT): $(MP_QSTR_CPP)
	mkdir -p $(MP_QSTR_FILES)
	rm -f $(MP_QSTR_FILES)/*.qstr
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py split qstr $< $(MP_QSTR_FILES) _
	touch $@

$(MP_QSTR_COLLECTED): $(MP_QSTR_SPLIT)
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py cat qstr _ $(MP_QSTR_FILES) $@

$(MP_QSTR_PREPROCESSED): $(MPY_ROOT)/py/qstrdefs.h $(MP_QSTR_COLLECTED) \
                         $(MP_QSTR_GLOBAL_DEPS) Makefile common.mk
	mkdir -p $(MP_QSTR_BUILD)
	cat $(MPY_ROOT)/py/qstrdefs.h $(MP_QSTR_COLLECTED) | \
		sed 's/^Q(.*)/"&"/' | \
		$(CC) -E -x c $(INCLUDE) $(CFLAGS) $(EXTRA_CFLAGS) - | \
		sed 's/^"\(Q(.*)\)"/\1/' > $@

$(MP_QSTR_HEADER): $(MP_QSTR_PREPROCESSED) $(MPY_ROOT)/py/makeqstrdata.py
	mkdir -p $(@D)
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdata.py $< > $@.tmp
	mv $@.tmp $@

$(MP_MODULE_SPLIT): $(MP_QSTR_CPP)
	mkdir -p $(MP_MODULE_FILES)
	rm -f $(MP_MODULE_FILES)/*.module
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py split module $< $(MP_MODULE_FILES) _
	touch $@

$(MP_MODULE_COLLECTED): $(MP_MODULE_SPLIT)
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py cat module _ $(MP_MODULE_FILES) $@

$(MP_MODULE_HEADER): $(MP_MODULE_COLLECTED) $(MPY_ROOT)/py/makemoduledefs.py
	mkdir -p $(@D)
	$(PYTHON) $(MPY_ROOT)/py/makemoduledefs.py $< > $@.tmp
	mv $@.tmp $@

$(MP_ROOT_POINTER_SPLIT): $(MP_QSTR_CPP)
	mkdir -p $(MP_ROOT_POINTER_FILES)
	rm -f $(MP_ROOT_POINTER_FILES)/*.root_pointer
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py split root_pointer $< $(MP_ROOT_POINTER_FILES) _
	touch $@

$(MP_ROOT_POINTER_COLLECTED): $(MP_ROOT_POINTER_SPLIT)
	$(PYTHON) $(MPY_ROOT)/py/makeqstrdefs.py cat root_pointer _ $(MP_ROOT_POINTER_FILES) $@

$(MP_ROOT_POINTER_HEADER): $(MP_ROOT_POINTER_COLLECTED) $(MPY_ROOT)/py/make_root_pointers.py
	mkdir -p $(@D)
	$(PYTHON) $(MPY_ROOT)/py/make_root_pointers.py $< > $@.tmp
	mv $@.tmp $@

.PHONY: qstr_headers
qstr_headers: $(MP_QSTR_HEADER) $(MP_MODULE_HEADER) $(MP_ROOT_POINTER_HEADER)

endif

################################################################################
# Standalone Static Library Builder (Always visible to Make)
################################################################################
AR = $(patsubst %gcc,%ar,$(CC))

.PHONY: build_mpy
build_mpy:
	$(MAKE) clean
	rm -rf $(MPY_ROOT)/genhdr/*.h
	mkdir -p $(BINDIR)/micropython/genhdr
	printf '#ifndef MICROPY_MPVERSION_H\n#define MICROPY_MPVERSION_H\n#define MICROPY_GIT_TAG "v1.20.0-custom"\n#define MICROPY_GIT_HASH "custom-vex-port"\n#define MICROPY_BUILD_DATE __DATE__\n#endif\n' > $(BINDIR)/micropython/genhdr/mpversion.h
	$(MAKE) qstr_headers COMPILE_ALL=1 -j36
	$(MAKE) quick COMPILE_ALL=1 -j36
	$(MAKE) archive_mpy COMPILE_ALL=1 -j36

.PHONY: archive_mpy
archive_mpy:
	mkdir -p $(FWDIR) $(MPY_ROOT)/genhdr
	rm -f $(FWDIR)/libmpy.a
	$(AR) rcs $(FWDIR)/libmpy.a $(filter-out $(BINDIR)/main.cpp.o $(BINDIR)/_pros_ld_timestamp.o,$(COBJ) $(CXXOBJ) $(ASMOBJ))
	cp -f $(BINDIR)/micropython/genhdr/*.h $(MPY_ROOT)/genhdr/
	@echo "[SUCCESS] Built $(FWDIR)/libmpy.a"

################################################################################
########## Nothing below this line should be edited by typical users ###########
-include ./common.mk