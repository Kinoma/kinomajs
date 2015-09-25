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
project(TOOLS)

include_directories(${F_HOME}/libraries/expat)
include_directories(${XS6}/includes)
include_directories(${XS6}/sources)
include_directories(${XS6}/sources/tool)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/modules)

if(NOT DEFINED CMAKE_MACOSX_RPATH)
	set(CMAKE_MACOSX_RPATH 0)
endif()

add_definitions(-DmxDebug=1)
add_definitions(-DHAVE_MEMMOVE=1)

if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /c /D _CONSOLE /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /D HAVE_MEMMOVE=1 /D XML_STATIC=1 /nologo /Zp1")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /D _DEBUG /D mxDebug /Od /Z7 /MTd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /D NDEBUG /O2 /MT")
	link_directories(${CMAKE_BINARY_DIR}/cmake/xsr/${CMAKE_CFG_INTDIR})
	set(LINK_OPTIONS ws2_32.lib comctl32.lib gdi32.lib xsr6.lib)
	set(SUFFIX ".dll")
elseif(UNIX)
	if(APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -mmacosx-version-min=10.7")
		set(LINK_OPTIONS "-dynamiclib -flat_namespace -undefined suppress")
	else()
		set(LINK_OPTIONS "-shared -Wl,-Bdynamic\\,-Bsymbolic")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
	set(SUFFIX ".so")
endif()

set(XSC_OPTIONS -c -d -e)

list(APPEND OBJECTS ${XS6}/tools/fs.c)
list(APPEND OBJECTS ${XS6}/tools/grammar.c)
list(APPEND OBJECTS ${XS6}/tools/infoset.c)
list(APPEND OBJECTS ${XS6}/tools/markup.c)
list(APPEND OBJECTS ${XS6}/tools/tool.c)

list(APPEND EXPAT_OBJECTS ${F_HOME}/libraries/expat/xmlparse.c)
list(APPEND EXPAT_OBJECTS ${F_HOME}/libraries/expat/xmlrole.c)
list(APPEND EXPAT_OBJECTS ${F_HOME}/libraries/expat/xmltok.c)

file(GLOB MODULES RELATIVE ${XS6}/tools ${XS6}/tools/*.js)
if (UNIX)
	if (APPLE)
		list(APPEND MODULES
			cmake/android.js
			cmake/ios.js
			cmake/mac.js
			make/android.js
			make/ios.js
			make/mac.js
			shared/android.js
			shared/ios.js
			)
	else ()
		file(GLOB_RECURSE LINUX_MODULES RELATIVE ${XS6}/tools ${XS6}/tools/cmake/linux/*.js ${XS6}/tools/make/linux/*.js)
		list(APPEND MODULES ${LINUX_MODULES})
	endif ()
else ()
	list(APPEND MODULES
		cmake/win.js
		make/win.js)
endif ()

foreach(MODULE ${MODULES})
	get_filename_component(MODULE_PATH ${MODULE} NAME_WE)
	get_filename_component(MODULE_DIR ${MODULE} PATH)
	if (MODULE_DIR)
		set(MODULE_PATH ${MODULE_DIR}/${MODULE_PATH})
	endif ()
	xsc(SOURCE_DIR ${XS6}/tools SOURCE ${MODULE_PATH} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/modules OPTIONS ${XSC_OPTIONS})
	list(APPEND XSB_PACKAGES ${CMAKE_CURRENT_BINARY_DIR}/modules/${MODULE_PATH}.xsb)
	list(APPEND SOURCES ${XS6}/tools/${MODULE})
endforeach()

xsl(NAME tools TMP ${CMAKE_CURRENT_BINARY_DIR}/modules SOURCES ${XSB_PACKAGES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/modules/tools.xs.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/modules/tools.xs.c PROPERTIES GENERATED TRUE)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/modules)

add_library(tools SHARED ${OBJECTS} ${EXPAT_OBJECTS} ${SOURCES})
add_dependencies(tools tools.xsa xsc xsl xsr)
if(LINK_OPTIONS)
	target_link_libraries(tools ${LINK_OPTIONS})
endif()
set_target_properties(tools PROPERTIES
	SUFFIX ${SUFFIX} PREFIX ""
	)
add_custom_command(
	TARGET tools
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${XS6_TOOLS_BIN}/modules
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:tools> ${XS6_TOOLS_BIN}/modules
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tools.xsa ${XS6_TOOLS_BIN}/modules
	)

set(SHEBANG "#!/usr/bin/env bash\n")
string(REPLACE ";" " " EXEC "${XSR}")

foreach(COMMAND kprconfig kpr2js xs2js)
	if(WIN32)
		add_custom_command(
			TARGET tools
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${XS6_TOOLS_BIN}
			COMMAND ${CMAKE_COMMAND} -E echo "@ECHO OFF" > ${COMMAND}6.bat
			COMMAND ${CMAKE_COMMAND} -E echo "${EXEC} ${COMMAND} %%*" >> ${COMMAND}6.bat
			COMMAND ${CMAKE_COMMAND} -E copy ${COMMAND}6.bat ${XS6_TOOLS_BIN}
			)
	else()
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${COMMAND}6 ${SHEBANG})
		file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${COMMAND}6
			DESTINATION ${CMAKE_BINARY_DIR}
			FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
		add_custom_command(
			TARGET tools
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory ${XS6_TOOLS_BIN}
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${COMMAND}6 ${XS6_TOOLS_BIN}
			COMMAND ${CMAKE_COMMAND} -E echo_append "${EXEC} ${COMMAND} $$" >> ${XS6_TOOLS_BIN}/${COMMAND}6
			COMMAND ${CMAKE_COMMAND} -E echo '*' >> ${XS6_TOOLS_BIN}/${COMMAND}6
			)
	endif()
endforeach()
