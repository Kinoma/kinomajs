
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

set(ASM_DIALECT "_WMMX")
if(ANDROID)
	set(CMAKE_ASM${ASM_DIALECT}_COMPILER ${F_HOME}/tools/wmmx/arm-marvell-eabi-as)
else()
	if(ASPEN OR POKY)
		set(CMAKE_ASM${ASM_DIALECT}_COMPILER ${TOOLCHAIN_BIN}/${TOOL_PREFIX}as)
	elseif(GTK)
		SET(CMAKE_ASM${ASM_DIALECT}_COMPILER ${CMAKE_ASM_COMPILER})
	else()
		set(CMAKE_ASM${ASM_DIALECT}_COMPILER ${TOOL_PREFIX}as)
	endif()
endif()
include(CMakeDetermineASMCompiler)
set(ASM_DIALECT)
