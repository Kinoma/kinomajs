cmake_minimum_required(VERSION 2.8)
project (mDNSResponder)

add_definitions(-D_GNU_SOURCE)
add_definitions(-DNOT_HAVE_SA_LEN)
add_definitions(-DUSES_NETLINK)
add_definitions(-DHAVE_LINUX)
add_definitions(-DTARGET_OS_LINUX)
add_definitions(-DMDNS_DEBUGMSGS=0)

include_directories(mDNSCore)
include_directories(mDNSShared)

list(APPEND lib_sources mDNSShared/dnssd_clientlib.c)
list(APPEND lib_sources mDNSShared/dnssd_clientstub.c)
list(APPEND lib_sources mDNSShared/dnssd_ipc.c)

list(APPEND client_sources Clients/dns-sd.c)
list(APPEND client_sources Clients/ClientCommon.c)

list(APPEND server_sources mDNSPosix/PosixDaemon.c)
list(APPEND server_sources mDNSPosix/mDNSPosix.c)
list(APPEND server_sources mDNSPosix/mDNSUNP.c)
list(APPEND server_sources mDNSCore/mDNS.c)
list(APPEND server_sources mDNSCore/DNSDigest.c)
list(APPEND server_sources mDNSCore/uDNS.c)
list(APPEND server_sources mDNSCore/DNSCommon.c)
list(APPEND server_sources mDNSShared/uds_daemon.c)
list(APPEND server_sources mDNSShared/mDNSDebug.c)
list(APPEND server_sources mDNSShared/dnssd_ipc.c)
list(APPEND server_sources mDNSShared/GenLinkedList.c)
list(APPEND server_sources mDNSShared/PlatformCommon.c)
list(APPEND server_sources mDNSCore/CryptoAlg.c)
list(APPEND server_sources mDNSCore/anonymous.c)

add_library(dns_sd SHARED ${lib_sources})
list(APPEND LIBRARIES dns_sd)
set(LIBRARIES "${LIBRARIES}" PARENT_SCOPE)

add_executable(dns-sd ${client_sources})
target_link_libraries(dns-sd dns_sd)

add_executable(mdnsd ${server_sources})

add_custom_command(
	TARGET dns-sd
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}/lib
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libdns_sd.so ${APP_DIR}/lib
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/dns-sd ${APP_DIR}/lib
	)

add_custom_command(
	TARGET mdnsd
	COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_DIR}/lib
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/mdnsd ${APP_DIR}/lib
	)

add_custom_target(mDNSResponder DEPENDS dns_sd dns-sd mdnsd)
