
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
IF(${PLATFORM} STREQUAL "android")
	SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${F_HOME}/tools/wmmx/arm-marvell-eabi-as)
ELSE()
	IF(${SUBPLATFORM} STREQUAL "aspen")
		SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${TOOLCHAIN_BIN}/${TOOL_PREFIX}as)
	ELSEIF(${SUBPLATFORM} STREQUAL "gtk")
		SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${CMAKE_ASM_COMPILER})
	ELSE()
		SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${TOOL_PREFIX}as)
	ENDIF()
ENDIF()
INCLUDE(CMakeDetermineASMCompiler)
SET(ASM_DIALECT)
