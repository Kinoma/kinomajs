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
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

FILE(TO_CMAKE_PATH $ENV{F_HOME} F_HOME)

IF(${CMAKE_MAJOR_VERSION} GREATER 2)
	CMAKE_POLICY(SET CMP0037 OLD)
ENDIF()

PROJECT(XS)

SET(CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR}/modules)

IF(NOT CMAKE_BUILD_TYPE)
  MESSAGE(STATUS "Setting build type to 'Release' as none was specified.")
  SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
ENDIF()

IF(CMAKE_CONFIGURATION_TYPES)
	SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "Reset the configurations to what we need" FORCE)
ENDIF()

STRING(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

SET(XS_BUILD true)

INCLUDE(Kinoma)
INCLUDE(XS6)

GET_PLATFORM(BUILD_PLATFORM)

SET(XS6 ${F_HOME}/xs6)
SET(XS6_TOOLS_BIN ${XS6}/bin/${BUILD_PLATFORM}/${BUILD_TYPE})
SET(XS6_TOOLS_TMP ${CMAKE_BINARY_DIR})
SET(XS6_MODULES_DIR ${XS6_TOOLS_BIN}/modules)
SET(XSR ${XS6_TOOLS_BIN}/xsr6 -a ${XS6_MODULES_DIR}/tools.xsa)
SET(KPR2JS ${XSR} ${XS6_MODULES} kpr2js)
SET(KPRCONFIG ${XSR} ${XS6_MODULES} kprconfig)
SET(XS2JS ${XSR} ${XS6_MODULES} xs2js)
SET(XSC ${XS6_TOOLS_BIN}/xsc6)
SET(XSL ${XS6_TOOLS_BIN}/xsl6)

GET_PLATFORM(BUILD_PLATFORM)

OPTION(XSBUG "Build XSBug" ON)

IF(NOT WIN32)
ADD_SUBDIRECTORY(${CMAKE_DIR}/kct)
ENDIF()
ADD_SUBDIRECTORY(${CMAKE_DIR}/tools)
ADD_SUBDIRECTORY(${CMAKE_DIR}/xsc)
ADD_SUBDIRECTORY(${CMAKE_DIR}/xsl)
ADD_SUBDIRECTORY(${CMAKE_DIR}/xsr)

IF(XSBUG)
	ADD_SUBDIRECTORY(${CMAKE_DIR}/xsbug)
ENDIF()
