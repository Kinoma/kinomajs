#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
set(CMAKE_COLOR_MAKEFILE OFF)

add_custom_command(
	OUTPUT ${BUILD_TMP}/resource.res
	COMMAND ${CMAKE_RC_COMPILER} /fo${BUILD_TMP}/resource.res ${F_HOME}/kinoma/kpr/make/win/resource.rc
	DEPENDS ${F_HOME}/kinoma/kpr/make/win/resource.rc
)
add_custom_target(resource_res DEPENDS ${BUILD_TMP}/resource.res)
list(APPEND PROJECT_DEPENDS resource_res)

list(APPEND EXTRA_LIBS ${BUILD_TMP}/resource.res)
