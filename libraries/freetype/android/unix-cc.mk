#
# FreeType 2 template for Unix-specific compiler definitions
#

# Copyright 1996-2000, 2002, 2003, 2005, 2006 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

AGCC_INC	:= -I$(NDK_DIR)platforms/android-9/arch-arm/usr/include/

AGCC_STUFF	:= $(AGCC_INC) -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -DANDROID -DSK_RELEASE -DNDEBUG -UDEBUG -march=armv5te -mtune=xscale -msoft-float -mthumb-interwork -fpic -fno-exceptions -ffunction-sections -funwind-tables -fstack-protector -fmessage-length=0 -Wall -Wno-unused -Wno-multichar -Wstrict-aliasing=2 -O2 -finline-functions -finline-limit=300 -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -fno-short-enums

CC			:= arm-linux-androideabi-gcc $(AGCC_STUFF)
#CC			:= agcc
COMPILER_SEP := $(SEP)

#LIBTOOL ?= $(BUILD_DIR)/libtool


# The object file extension (for standard and static libraries).  This can be
# .o, .tco, .obj, etc., depending on the platform.
#
O  := lo
SO := o


# The library file extension (for standard and static libraries).  This can
# be .a, .lib, etc., depending on the platform.
#
A  := la
SA := a


# The name of the final library file.  Note that the DOS-specific Makefile
# uses a shorter (8.3) name.
#
LIBRARY := lib$(PROJECT)


# Path inclusion flag.  Some compilers use a different flag than `-I' to
# specify an additional include path.  Examples are `/i=' or `-J'.
#
I := -I


# C flag used to define a macro before the compilation of a given source
# object.  Usually it is `-D' like in `-DDEBUG'.
#
D := -D


# The link flag used to specify a given library file on link.  Note that
# this is only used to compile the demo programs, not the library itself.
#
L := -l


# Target flag.
#
T := -o$(space)


# C flags
#
#   These should concern: debug output, optimization & warnings.
#
#   Use the ANSIFLAGS variable to define the compiler flags used to enfore
#   ANSI compliance.
#
#   We use our own FreeType configuration file.
#
CPPFLAGS :=
# CFLAGS   := $(C_FLAGS) -c  -g -O2 -DFT_CONFIG_CONFIG_H="<ftconfig.h>"
#CFLAGS   := $(C_FLAGS) -c  -g  -DFT_CONFIG_CONFIG_H="<ftconfig.h>"
CFLAGS   := $(C_FLAGS) -c  -O2  -DFT_CONFIG_CONFIG_H="<freetype/config/ftconfig.h>"

# ANSIFLAGS: Put there the flags used to make your compiler ANSI-compliant.
#
ANSIFLAGS :=

# C compiler to use -- we use libtool!
#
#
CCraw := $(CC)
CC    := $(LIBTOOL) --mode=compile $(CCraw)

# Linker flags.
#
LDFLAGS :=


# export symbols (XXX: HOW TO DEAL WITH CROSS COMPILATION ?)
#
EXPORTS_LIST := $(OBJ_DIR)/ftexport.sym
CCexe        := $(CCraw)   # used to compile "apinames" only


# Library linking
#
LINK_LIBRARY = $(LIBTOOL) --mode=link $(CCraw) -o $@ $(OBJECTS_LIST) \
                          -rpath $(libdir) -version-info $(version_info) \
                          $(LDFLAGS) \
                          # -export-symbols $(EXPORTS_LIST) -no-undefined

# EOF
