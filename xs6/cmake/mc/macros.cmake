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
macro(BINARY)
	set(oneValueArgs SOURCE MODULE SUBDIR)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(SOURCE ${LOCAL_SOURCE})
	set(MODULE ${LOCAL_MODULE})
	set(SUBDIR ${LOCAL_SUBDIR})

	get_filename_component(NAME ${SOURCE} NAME_WE)
	get_filename_component(EXT ${SOURCE} EXT)
	set(XSB ${NAME}.xsb)

	if (${EXT} STREQUAL ".js")
		set(JS true)
	else ()
		set(JS false)
	endif ()

	if (NOT MODULE)
		set(MODULE ${NAME})
	endif ()

	if (NOT JS)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.js
			COMMAND ${XS6_TOOLS_BIN}/xsr6 -a ${XS6_MODULES_DIR}/tools.xsa xs2js ${SOURCE} -m ${MODULE} -o ${CMAKE_CURRENT_BINARY_DIR}
			DEPENDS tools ${SOURCE}
			)
		set(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.js)
		set(DEPEND ${SOURCE})
	endif ()

	add_custom_command(
		OUTPUT ${DEST_DIR}/${SUBDIR}/${MODULE}.xsb
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}
		COMMAND ${XSC} ${XSC_OPTIONS} -c -e -d -o ${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR} ${SOURCE}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}/${SUBDIR}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}/${XSB} ${DEST_DIR}/${SUBDIR}/${MODULE}.xsb
		DEPENDS tools ${SOURCE} ${DEPENDS}
		)

	list(APPEND MC_DEPS ${DEST_DIR}/${SUBDIR}/${MODULE}.xsb)
endmacro()
