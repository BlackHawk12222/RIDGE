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
# EXCLUDE_COLD_LIBRARIES:= $(FWDIR)/your_library.a
EXCLUDE_COLD_LIBRARIES:= 

# Set this to 1 to add additional rules to compile your project as a PROS library template
IS_LIBRARY:=0
# TODO: CHANGE THIS! 
# Be sure that your header files are in the include directory inside of a folder with the
# same name as what you set LIBNAME to below.
LIBNAME:=libbest
VERSION:=1.0.0
# EXCLUDE_SRC_FROM_LIB= $(SRCDIR)/unpublishedfile.c
# this line excludes opcontrol.c and similar files
EXCLUDE_SRC_FROM_LIB+=$(foreach file, $(SRCDIR)/main,$(foreach cext,$(CEXTS),$(file).$(cext)) $(foreach cxxext,$(CXXEXTS),$(file).$(cxxext)))

# files that get distributed to every user (beyond your source archive) - add
# whatever files you want here. This line is configured to add all header files
# that are in the directory include/LIBNAME
TEMPLATE_FILES=$(INCDIR)/$(LIBNAME)/*.h $(INCDIR)/$(LIBNAME)/*.hpp

.DEFAULT_GOAL=quick

# Force include paths to omit the unneeded REPL execution module
EXCLUDE_SRC_FROM_LIB += src/micropython/shared/runtime/pyexec.c

# Add this line right below your other MicroPython QSTR source definitions
SRC_QSTR += src/modpros.c

# Add your custom MicroPython module code to the PROS compiler track
SRC_C += $(wildcard modules/*.c)

# Ensure the compiler can see MicroPython's header files and your module headers
EXTRA_INCDIRS += modules/umatrix.c

# Ensure the bottom of your root Makefile matches this block:
EXTRA_SRCDIRS += src/micropython \
                 src/micropython/py \
                 src/micropython/shared

EXTRA_CFLAGS += -I$(ROOT)/src/micropython \
                -I$(ROOT)/src/micropython/genhdr \
                -I$(ROOT)/src/genhdr \
                -I$(ROOT)/src/micropython/extmod

# Exclude help, arrays, and native modules we don't need from the C compilation list
TO_REMOVE := src/micropython/py/builtinhelp.c \
             src/micropython/py/modarray.c

SRC_C := $(filter-out $(TO_REMOVE),$(SRC_C))

# USERMOD_DIR := $(USER_C_MODULES)/custom_math

# # Add source files
# SRC_USERMOD += $(USERMOD_DIR)/custom_math.c

# Include the directory in the compiler path
CFLAGS_USERMOD += -I$(USERMOD_DIR)

################################################################################
################################################################################
########## Nothing below this line should be edited by typical users ###########
-include ./common.mk
