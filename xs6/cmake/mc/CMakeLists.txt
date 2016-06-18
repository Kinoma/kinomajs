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
project(MC)

set(MC_DIR ${XS6}/sources/mc)

add_custom_target(mc
	ALL
	COMMAND env XS6_TOOL_DIR="${XS6_TOOLS_BIN}" make -j1 host
	WORKING_DIRECTORY ${MC_DIR}
	DEPENDS tools
	)
