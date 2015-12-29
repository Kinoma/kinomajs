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
include_directories(${XS6}/includes)
include_directories(${XS6}/sources)
include_directories(${XS6}/sources/tool)
include_directories(${XS6}/sources/mc)
include_directories(${XS6}/sources/mc/extensions/crypt)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/modules)
include_directories(${MC_DIR}/extensions/crypt)
include_directories(${MC_DIR}/extensions/crypt/kcl)
include_directories(${MC_DIR}/extensions/crypt/c25519/src)
include_directories(${MC_DIR}/extensions/crypt/poly1305-donna)

if(NOT DEFINED CMAKE_MACOSX_RPATH)
	set(CMAKE_MACOSX_RPATH 0)
endif()

add_definitions(-DXS_ARCHIVE=1)
add_definitions(-D_DEBUG=1)
add_definitions(-DmxDebug=1)
add_definitions(-DmxDebug=1)
add_definitions(-DmxReport=1)
add_definitions(-DmxRun=1)

if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /c /D _CONSOLE /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /nologo /Zp1")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /D _DEBUG /Od /Z7 /MTd")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /D NDEBUG /O2 /MT")
	link_directories(${CMAKE_BINARY_DIR}/cmake/xsr/${CMAKE_CFG_INTDIR})
	set(LINK_OPTIONS ws2_32.lib comctl32.lib gdi32.lib xsr6.lib)
	set(SUFFIX ".dll")
elseif(UNIX)
	if(APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -mmacosx-version-min=10.7")
		set(LINK_OPTIONS "-dynamiclib -flat_namespace -undefined suppress")
		set(MOD_LIBRARIES "-framework CoreServices -lSystem")
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
	set(SUFFIX ".so")
endif()

set(MOD_LINK_OPTIONS "-nodefaultlibs -nostartfiles")


add_library(libmc STATIC
	${MC_DIR}/mc_file.c
	${MC_DIR}/mc_env.c
	${MC_DIR}/mc_event.c
	${MC_DIR}/xs_patch.c
	${MC_DIR}/mc_ipc.c
	${MC_DIR}/mc_misc.c
	${MC_DIR}/mc_stdio.c
	${MC_DIR}/mc_xs.c
	)
target_link_libraries(libmc ${LINK_OPTIONS} ${MOD_LINK_OPTIONS} ${MOD_LIBRARIES})
set_target_properties(libmc PROPERTIES OUTPUT_NAME "mc")

add_library(libmodule STATIC
	${MC_DIR}/extensions/crypt/arith_ec.c
	${MC_DIR}/extensions/crypt/arith_ecp.c
	${MC_DIR}/extensions/crypt/arith_ed.c
	${MC_DIR}/extensions/crypt/arith_int.c
	${MC_DIR}/extensions/crypt/arith_mod.c
	${MC_DIR}/extensions/crypt/arith_mont.c
	${MC_DIR}/extensions/crypt/arith_z.c
	${MC_DIR}/extensions/crypt/c25519/src/c25519.c
	${MC_DIR}/extensions/crypt/c25519/src/ed25519.c
	${MC_DIR}/extensions/crypt/c25519/src/edsign.c
	${MC_DIR}/extensions/crypt/c25519/src/f25519.c
	${MC_DIR}/extensions/crypt/c25519/src/fprime.c
	${MC_DIR}/extensions/crypt/c25519/src/morph25519.c
	${MC_DIR}/extensions/crypt/c25519/src/sha512.c
	${MC_DIR}/extensions/crypt/crypt_aes.c
	${MC_DIR}/extensions/crypt/crypt_cbc.c
	${MC_DIR}/extensions/crypt/crypt_chacha.c
	${MC_DIR}/extensions/crypt/crypt_cipher.c
	${MC_DIR}/extensions/crypt/crypt_ctr.c
	${MC_DIR}/extensions/crypt/crypt_curve25519.c
	${MC_DIR}/extensions/crypt/crypt_des.c
	${MC_DIR}/extensions/crypt/crypt_digest.c
	${MC_DIR}/extensions/crypt/crypt_ecb.c
	${MC_DIR}/extensions/crypt/crypt_ed25519_c.c
	${MC_DIR}/extensions/crypt/crypt_md5.c
	${MC_DIR}/extensions/crypt/crypt_mode.c
	${MC_DIR}/extensions/crypt/crypt_poly1305.c
	${MC_DIR}/extensions/crypt/crypt_rc4.c
	${MC_DIR}/extensions/crypt/crypt_rng.c
	${MC_DIR}/extensions/crypt/crypt_sha1.c
	${MC_DIR}/extensions/crypt/crypt_sha256.c
	${MC_DIR}/extensions/crypt/crypt_sha512.c
	${MC_DIR}/extensions/crypt/crypt_srp.c
	${MC_DIR}/extensions/crypt/crypt_stream.c
	${MC_DIR}/extensions/crypt/crypt_tdes.c
	${MC_DIR}/extensions/crypt/crypt_x509.c
	${MC_DIR}/extensions/crypt/kcl.c
	${MC_DIR}/extensions/crypt/kcl/bn.c
	${MC_DIR}/extensions/crypt/kcl/chacha.c
	${MC_DIR}/extensions/crypt/kcl/fips180.c
	${MC_DIR}/extensions/crypt/kcl/fips197.c
	${MC_DIR}/extensions/crypt/kcl/fips46.c
	${MC_DIR}/extensions/crypt/kcl/kcl_arith.c
	${MC_DIR}/extensions/crypt/kcl/kcl_symmetric.c
	${MC_DIR}/extensions/crypt/kcl/rc.c
	${MC_DIR}/extensions/crypt/kcl/rfc1321.c
	${MC_DIR}/extensions/crypt/poly1305-donna/poly1305-donna.c
	${MC_DIR}/extensions/ext_bin.c
	${MC_DIR}/extensions/ext_dhcpd.c
	${MC_DIR}/extensions/ext_mdns.c
	${MC_DIR}/extensions/hap/hap_ie.c
	${MC_DIR}/extensions/hap/hap_tlv.c
	${MC_DIR}/extensions/inetd_telnetd.c
	${MC_DIR}/extensions/inetd_tftpd.c
	${MC_DIR}/extensions/io/io_a2d.c
	${MC_DIR}/extensions/io/io_i2c.c
	${MC_DIR}/extensions/io/io_uart.c
	${MC_DIR}/extensions/mc_telnetd.c
	${MC_DIR}/extensions/mc_tftpd.c
	${MC_DIR}/extensions/pinmux.c
	${MC_DIR}/extensions/pins/pins.c
	${MC_DIR}/extensions/soft_crc32.c
	${MC_DIR}/modules/xm_console.c
	${MC_DIR}/modules/xm_debug.c
	${MC_DIR}/modules/xm_env.c
	${MC_DIR}/modules/xm_files.c
	${MC_DIR}/modules/xm_socket.c
	${MC_DIR}/modules/xm_system.c
	${MC_DIR}/modules/xm_timeinterval.c
	${MC_DIR}/modules/xm_watchdogtimer.c
	${MC_DIR}/modules/xm_wifi.c
)
target_link_libraries(libmodule ${LINK_OPTIONS} ${MOD_LINK_OPTIONS} ${MOD_LIBRARIES})
set_target_properties(libmodule PROPERTIES OUTPUT_NAME "module")

add_library(mc SHARED ${DEST_DIR}/mc.xs.c)
target_link_libraries(mc ${LINK_OPTIONS} ${MOD_LINK_OPTIONS} ${MOD_LIBRARIES} libmc libmodule)
set_target_properties(mc PROPERTIES OUTPUT_NAME "mc" PREFIX "" SUFFIX ${SUFFIX})
add_dependencies(mc mc.xsa host_fs)

add_custom_command(TARGET mc
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E remove ${XS6_MODULES_DIR}/mc.so
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mc> ${XS6_MODULES_DIR}
	)
