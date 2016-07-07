#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
cmake_minimum_required(VERSION 2.8)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "Please do not use the same source directory and tmp directory.
Create a new directory after deleting ${CMAKE_SOURCE_DIR}/CMakeCache.txt and ${CMAKE_SOURCE_DIR}/CMakeFiles and try again.")
endif ()

file(TO_CMAKE_PATH $ENV{F_HOME} F_HOME)

if(${CMAKE_MAJOR_VERSION} GREATER 2)
	cmake_policy(SET CMP0037 OLD)
endif()

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Release" "Debug" CACHE STRING "Reset the configurations to what we need" FORCE)
else()
	if(NOT CMAKE_BUILD_TYPE)
		message(STATUS "Setting build type to 'Release' as none was specified.")
		set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
	endif()
endif()

project(XS)

set(XS_BUILD TRUE)

set(CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR}/modules)

include(Kinoma)
include(XS6)

get_platform(BUILD_PLATFORM)

include(${BUILD_PLATFORM} optional)

set(XS6 ${F_HOME}/xs6)
if(NOT XS6_TOOLS_BIN)
	set(XS6_TOOLS_BIN ${XS6}/bin/${BUILD_PLATFORM}/$<CONFIGURATION>)
endif()
set(XS6_TOOLS_TMP ${CMAKE_BINARY_DIR})
set(XS6_MODULES_DIR ${XS6_TOOLS_BIN}/modules)
set(XSR \"${XS6_TOOLS_BIN}/xsr6\" -a \"${XS6_MODULES_DIR}/tools.xsa\")
set(KPR2JS ${XSR} ${XS6_MODULES} kpr2js)
set(KPRCONFIG ${XSR} ${XS6_MODULES} kprconfig)
set(XS2JS ${XSR} ${XS6_MODULES} xs2js)
set(XSC ${XS6_TOOLS_BIN}/xsc6)
set(XSL ${XS6_TOOLS_BIN}/xsl6)

option(MC "Build MC" OFF)
if(MC)
	unset(MC_FLAG)
else()
	set(MC_FLAG EXCLUDE_FROM_ALL)
endif()

option(XSBUG "Build XSBug" OFF)
if(NOT XSBUG)
	set(XSBUG_FLAG EXCLUDE_FROM_ALL)
endif()

if(NOT WIN32)
	add_subdirectory(${CMAKE_DIR}/kct)
endif()

add_subdirectory(${CMAKE_DIR}/tools)
add_subdirectory(${CMAKE_DIR}/xsc)
add_subdirectory(${CMAKE_DIR}/xsl)
add_subdirectory(${CMAKE_DIR}/xsr)

if (EXISTS ${CMAKE_DIR}/mc)
	add_subdirectory(${CMAKE_DIR}/mc ${MC_FLAG})
endif ()

add_subdirectory(${CMAKE_DIR}/xsbug ${XSBUG_FLAG})
