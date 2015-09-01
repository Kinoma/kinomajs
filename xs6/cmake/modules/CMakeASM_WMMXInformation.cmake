
# derived from
#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# support for AT&T syntax assemblers, e.g. GNU as

SET(ASM_DIALECT "_WMMX")
SET(CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS wmmx)
SET(CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "<CMAKE_ASM${ASM_DIALECT}_COMPILER> ${AS_WMMX_OPTIONS} -o <OBJECT> <SOURCE>")
INCLUDE(CMakeASMInformation)
SET(ASM_DIALECT)
