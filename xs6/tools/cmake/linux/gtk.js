/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

import * as FS from "fs";
import * as MAKE from "cmake";

class Makefile extends MAKE.Makefile {
	constructor(tree) {
		super(tree);
	}
};

class Manifest extends MAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	get Makefile() {
		return Makefile;
	}
	getPlatformLanguages() {
		return ["ASM", "ASM_WMMX"];
	}
	getPlatformVariables(tool, tmp, bin) {
		var path = "${F_HOME}/xs6/bin/linux/";
		path += tool.execute("uname -m").trim();
		path += process.debug ? "/debug" : "/release";
		return {
			KPR_APPLICATION: tool.application,

			KPR2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa kpr2js",
			XS2JS: path + "/xsr6 -a " +  path + "/modules/tools.xsa xs2js",
			XSC: path + "/xsc6",
			XSL: path + "/xsl6",

			KPR_CONFIGURATION: tool.debug ? "debug" : "release",

			APP_DIR: bin,
		}
	}
	getTargetRules(tool) {
		return `
ADD_EXECUTABLE(Kpl \${SOURCES} \${FskPlatform_SOURCES} \${TARGET_OBJECTS})
TARGET_INCLUDE_DIRECTORIES(Kpl  PRIVATE \${C_INCLUDES})
TARGET_COMPILE_DEFINITIONS(Kpl PRIVATE \${C_DEFINITIONS})
TARGET_COMPILE_OPTIONS(Kpl PRIVATE \${C_OPTIONS})
TARGET_LINK_LIBRARIES(Kpl -Wl,--whole-archive -Wl,-Map,\${TMP_DIR}/Kpl.map \${OBJECTS} \${LIBRARIES})
SET_TARGET_PROPERTIES(Kpl PROPERTIES RUNTIME_OUTPUT_DIRECTORY \${APP_DIR})

IF(RELEASE)
	ADD_CUSTOM_COMMAND(
			TARGET Kpl
			POST_BUILD
			COMMAND \${TOOL_PREFIX}strip \$<TARGET_FILE:Kpl>
			)
ENDIF()
`;
	}
};

export default Manifest;
