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
project(KCT)

set(SHEBANG "#! /usr/bin/env node\n")
file(READ ${XS6}/tools/kct6.js KCT6)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/kct6 ${SHEBANG})
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/kct6 ${KCT6})

file(COPY ${CMAKE_CURRENT_BINARY_DIR}/kct6
	DESTINATION ${XS6_TOOLS_BIN}
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
