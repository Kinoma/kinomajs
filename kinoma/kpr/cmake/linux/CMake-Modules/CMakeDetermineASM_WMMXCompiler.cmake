
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

SET(ASM_DIALECT "_WMMX")
IF (${TARGET_SUBPLATFORM} STREQUAL "/aspen")
	SET(CMAKE_ASM${ASM_DIALECT}_COMPILER $ENV{ARM_MARVELL_LINUX_GNUEABI}/bin/arm-marvell-linux-gnueabi-as)
ELSEIF (${TARGET_SUBPLATFORM} STREQUAL "/gtk")
	SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${CMAKE_ASM_COMPILER})
ELSE (${TARGET_SUBPLATFORM} STREQUAL "/aspen")
	SET(CMAKE_ASM${ASM_DIALECT}_COMPILER $ENV{BG3CDP_GNUEABI}/bin/arm-linux-gnueabihf-as)
ENDIF (${TARGET_SUBPLATFORM} STREQUAL "/aspen")
INCLUDE(CMakeDetermineASMCompiler)
SET(ASM_DIALECT)
