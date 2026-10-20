################################################################################
######################### User configurable parameters #########################
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
USE_PACKAGE:=1

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
# Custom MicroPython Port Parameter Mapping (Fully Manual)
################################################################################

# 1. Revert INCDIR back to standard defaults to avoid common.mk layout conflicts
INCDIR=$(ROOT)/include

# 2. Append the base micropython folder directly into the compiler flag queues.
# This forces the inclusion of 'mpconfigport.h' via global GCC flags.
EXTRA_CFLAGS += -I$(BINDIR)/micropython \
                -Isrc/micropython \
                -Isrc/micropython/py \
                -Isrc/micropython/extmod

EXTRA_CXXFLAGS += -I$(BINDIR)/micropython \
                  -Isrc/micropython \
                  -Isrc/micropython/py \
                  -Isrc/micropython/extmod

# 3. Explicitly scan and track deep source paths inside common.mk loops
EXTRA_SRCDIRS += src/micropython \
                 src/micropython/py \
                 src/micropython/shared/runtime \
                 src/micropython/modules

# 4. Handle files you want to manually omit from compiling
EXCLUDE_SRC_FROM_LIB += src/micropython/shared/runtime/pyexec.c \
                        src/micropython/py/builtinhelp.c

# 5. Global tracker symbols
SRC_QSTR += src/mp_port.c src/modpros.c src/micropython/modules/vex_module.c

################################################################################
########## Nothing below this line should be edited by typical users ###########
-include ./common.mk

PYTHON ?= python

################################################################################
# Generate MicroPython QSTR data using the upstream 1.29.0 tools.
################################################################################

MPY_ROOT := src/micropython
MP_QSTR_SOURCES := $(sort $(call rwildcard,$(MPY_ROOT)/,*.c,) src/mp_port.c src/modpros.c src/micropython/modules/vex_module.c)
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
