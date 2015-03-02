
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

# determine the compiler to use for ASM using AT&T syntax, e.g. GNU as

SET(ASM_DIALECT "-gas7")
set(CMAKE_ASM-gas7_COMPILER "${CMAKE_C_COMPILER}"     CACHE PATH "Assembler (gas7)" )
INCLUDE(CMakeDetermineASMCompiler)
SET(ASM_DIALECT)
