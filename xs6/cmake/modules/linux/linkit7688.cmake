set(LINUX true)
set(LINKIT7688 true)
include (CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "mips24KEc")

set(TOOL_PREFIX mipsel-openwrt-linux-uclibc-)
set(LINKIT7688_SYSROOT $ENV{LINKIT7688_SYSROOT})

find_path(TOOLCHAIN_BIN ${TOOL_PREFIX}gcc $ENV{PATH} $ENV{LINKIT7688_GNUEABI}/bin)

set(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc CACHE PATH "assembler")
cmake_force_c_compiler(${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc GNU)
cmake_force_cxx_compiler(${TOOLCHAIN_BIN}/${TOOL_PREFIX}gcc GNU)
set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS gas7;gas;s)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -c -x assembler-with-cpp -o <OBJECT> <SOURCE>")

set(AS_NEON ${TOOL_PREFIX}as)
set(AS_NEON_OPTIONS -mfpu=neon)
set(AS_OPTIONS -c -x assembler-with-cpp)
set(AS_V7 ${TOOL_PREFIX}as)
set(AS_V7_OPTIONS )

set(FREETYPE_VERSION 2.6)
set(FREETYPE_DIR ${TMP_DIR}/freetype-${FREETYPE_VERSION})

macro(BUILD)
	set(oneValueArgs APPLICATION)
	cmake_parse_arguments(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	file(MAKE_DIRECTORY ${FREETYPE_DIR})

	add_subdirectory(${F_HOME}/libraries/freetype/xs6 FreeType)

	add_executable(${KPR_APPLICATION} ${SOURCES} ${FskPlatform_SOURCES} ${TARGET_OBJECTS})
	add_dependencies(${KPR_APPLICATION} FreeType)
	target_include_directories(${KPR_APPLICATION}  PRIVATE ${C_INCLUDES})
	target_compile_definitions(${KPR_APPLICATION} PRIVATE ${C_DEFINITIONS})
	target_compile_options(${KPR_APPLICATION} PRIVATE ${C_OPTIONS})
	target_link_libraries(${KPR_APPLICATION} -Wl,--whole-archive -Wl,-Map,${TMP_DIR}/${KPR_APPLICATION}.map ${OBJECTS} ${LIBRARIES})

	if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		add_custom_command(
			TARGET ${KPR_APPLICATION}
			POST_BUILD
			COMMAND ${TOOLCHAIN_BIN}/${TOOL_PREFIX}strip $<TARGET_FILE:${KPR_APPLICATION}>
			)
	endif()

	add_custom_target(
		Assemble
		ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR}/ ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${KPR_APPLICATION}> ${APP_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${TMP_DIR}/app ${APP_DIR}
		COMMAND tar czf ${APP_DIR}/../${LOCAL_APPLICATION}.tgz -C ${APP_DIR}/../ ${LOCAL_APPLICATION}
		DEPENDS ${KPR_APPLICATION} FskManifest.xsa
		)
endmacro()
# vim: ft=cmake
