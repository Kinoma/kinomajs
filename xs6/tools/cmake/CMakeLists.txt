include(CMakeParseArguments)

macro(BUILD_LIBRARY)
	set(oneValueArgs NAME)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	add_library(${LOCAL_NAME} STATIC ${${LOCAL_NAME}_SOURCES})
	target_include_directories(${LOCAL_NAME} PUBLIC ${${LOCAL_NAME}_C_INCLUDES} ${C_INCLUDES})
	target_compile_definitions(${LOCAL_NAME} PUBLIC ${${LOCAL_NAME}_C_DEFINITIONS} ${C_DEFINITIONS})
	target_compile_options(${LOCAL_NAME} PUBLIC ${${LOCAL_NAME}_C_OPTIONS} ${C_OPTIONS})
	add_dependencies(${LOCAL_NAME} FskManifest)
endmacro()

macro(COPY)
	set(oneValueArgs SOURCE DESTINATION)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(SOURCE_NAME ${LOCAL_SOURCE} NAME)
	get_filename_component(DESTIANTION_NAME ${LOCAL_DESTINATION} NAME)
	get_filename_component(DIRECTORY ${LOCAL_DESTINATION} DIRECTORY)

	file(COPY ${LOCAL_SOURCE} DESTINATION ${DIRECTORY})
	if (NOT ${SOURCE_NAME} STREQUAL ${DESTIANTION_NAME})
		file(RENAME ${DIRECTORY}/${SOURCE_NAME} ${DIRECTORY}/${DESTIANTION_NAME})
	endif ()

endmacro()

macro(JS2XSB)
	set(oneValueArgs SOURCE DESTINATION)
	set(multiValueArgs OPTIONS)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)

	add_custom_command(
		OUTPUT ${TMP_DIR}/${LOCAL_DESTINATION}/${NAME}.xsb
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TMP_DIR}/${LOCAL_DESTINATION}
		COMMAND ${XSC} ${LOCAL_SOURCE} ${LOCAL_OPTIONS} -o ${TMP_DIR}/${LOCAL_DESTINATION}
		DEPENDS ${LOCAL_SOURCE}
		)
	string(MAKE_C_IDENTIFIER ${LOCAL_DESTINATION}/${NAME}.xsb TARGET)
	# add_custom_target(${TARGET} DEPENDS ${TMP_DIR}/${LOCAL_DESTINATION}/${NAME}.xsb)
endmacro()

macro(LOCAL_FIND_LIBRARY libname)
	set(options OPTIONAL EXIT)
	cmake_parse_arguments(LOCAL_LIBRARY "${options}" "${onevalueArgs}" "${multiValueArgs}" ${ARGN})

	string(TOUPPER ${libname} LIB_NAME)
	find_library(${LIB_NAME} ${libname} ${ARGN})
	if (${${LIB_NAME}} STREQUAL ${LIB_NAME}-NOTFOUND)
		if (NOT ${LOCAL_LIBRARY_EXIT})
			LOCAL_FIND_LIBRARY(${libname} EXIT true NO_CMAKE_FIND_ROOT_PATH)
		else ()
			message(FATAL_ERROR ": ${libname} not found")
		endif ()
	else ()
		if (APPLE)
			get_filename_component(EXTENSION ${${LIB_NAME}} EXT)
			if (${EXTENSION} STREQUAL ".framework")
				mark_as_advanced(${LIB_NAME})
			endif ()
		endif ()
		list(APPEND LIBRARIES ${${LIB_NAME}})
	endif()

endmacro()

macro(STRING_APPEND)
	set(oneValueArgs NAME VALUE)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(${LOCAL_NAME} "${${LOCAL_NAME}} ${LOCAL_VALUE}")
endmacro()

macro(XML2XSB)
	set(oneValueArgs SOURCE DESTINATION)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)

	add_custom_command(
		OUTPUT ${TMP_DIR}/${LOCAL_DESTINATION}/${NAME}.js
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TMP_DIR}/${LOCAL_DESTINATION}
		COMMAND ${KPR2JS} ${LOCAL_SOURCE} -o ${TMP_DIR}/${LOCAL_DESTINATION}
		DEPENDS ${LOCAL_SOURCE}
		)
	JS2XSB(SOURCE ${TMP_DIR}/${LOCAL_DESTINATION}/${NAME}.js DESTINATION ${LOCAL_DESTINATION})
	string(MAKE_C_IDENTIFIER ${LOCAL_DESTINATION}/${NAME}.js TARGET)
endmacro()

macro(XS2XSB)
	set(oneValueArgs SOURCE)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)

	XS2JS(SOURCE ${LOCAL_SOURCE})
	JS2XSB(SOURCE ${TMP_DIR}/${NAME}.js)
endmacro()

macro(XS2JS)
	set(oneValueArgs SOURCE)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	get_filename_component(NAME ${LOCAL_SOURCE} NAME_WE)

	add_custom_command(
		OUTPUT ${TMP_DIR}/${NAME}.js
		COMMAND ${XS2JS} ${LOCAL_SOURCE} ${XSC_OPTIONS} -p -o ${TMP_DIR}
		DEPENDS ${LOCAL_SOURCE} ${XSC_PACKAGES}
		)
endmacro()

macro(XSC)
	set(oneValueArgs SOURCE)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
endmacro()

macro(XSL)
	set(oneValueArgs NAME)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	XS2JS(SOURCE ${TMP_DIR}/${LOCAL_NAME}.xs)
	JS2XSB(SOURCE ${TMP_DIR}/${LOCAL_NAME}.js OPTIONS -c -d -e -p)

	add_custom_command(
		OUTPUT ${APP_DIR}/${LOCAL_NAME}.xsa ${TMP_DIR}/${LOCAL_NAME}.xs.c ${TMP_DIR}/src/${LOCAL_NAME}.xs.h
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${TMP_DIR}/src
		COMMAND ${XSL} ${MODULES} -a FskManifest -b ${TMP_DIR} -o ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TMP_DIR}/${LOCAL_NAME}.xs.h ${TMP_DIR}/src
		COMMAND ${CMAKE_COMMAND} -E remove ${TMP_DIR}/${LOCAL_NAME}.xs.h
		DEPENDS ${MODULES}
		)
	add_custom_target(${LOCAL_NAME} DEPENDS ${APP_DIR}/${LOCAL_NAME}.xsa)
endmacro()
