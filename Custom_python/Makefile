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

# 1. Provide absolute search contexts for both the parent directory AND sub-paths
EXTRA_INCDIRS += src/micropython \
                 src/micropython/py \
                 src/micropython/genhdr \
                 src/micropython/extmod

# 2. Hard-inject standard compiler include flags into the build engine
# This explicitly tells GCC that 'src/micropython' is a valid search route for "py/..."
EXTRA_CFLAGS += -Isrc/micropython \
                -Isrc/micropython/py \
                -Isrc/micropython/extmod

# 3. Explicitly scan and track deep source paths inside common.mk loops
EXTRA_SRCDIRS += src/micropython \
                 src/micropython/py \
                 src/micropython/shared/runtime \
                 src/micropython/modules

# 4. Handle files you want to manually omit from compiling
EXCLUDE_SRC_FROM_LIB += src/micropython/shared/runtime/pyexec.c \
                        src/micropython/py/builtinhelp.c \
                        src/micropython/py/modarray.c

# 5. Global tracker symbols
SRC_QSTR += src/modpros.c

################################################################################
########## Nothing below this line should be edited by typical users ###########
-include ./common.mk
