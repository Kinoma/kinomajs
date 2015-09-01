
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

# This file is used by EnableLanguage in cmGlobalGenerator to
# determine that the selected ASM-ATT "compiler" works.
# For assembler this can only check whether the compiler has been found,
# because otherwise there would have to be a separate assembler source file
# for each assembler on every architecture.

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
INCLUDE(CMakeTestASMCompiler)
SET(ASM_DIALECT)
