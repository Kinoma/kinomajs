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
import * as CMAKE from "cmake";

class Makefile extends CMAKE.Makefile {
	constructor(tree) {
		super(tree);
	}
	// generateRules(tool, file, path) {
	// 	file.line("ADD_LIBRARY(", this.name, " OBJECT ${", this.name, "_SOURCES})");
	// 	file.line("LIST(APPEND TARGET_OBJECTS $<TARGET_OBJECTS:", this.name, ">)");
	// 	file.line("SET(TARGET_OBJECTS ${TARGET_OBJECTS} PARENT_SCOPE)");
	// 	file.line("ADD_DEPENDENCIES(", this.name, " FskManifest.xsa)");
	// }
};

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	get Makefile() {
		return Makefile;
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var resource = parts.directory + "\\win\\resource.rc";
		var path = process.debug ? "$(F_HOME)\\xs6\\bin\\win\\debug" : "$(F_HOME)\\xs6\\bin\\win\\release"
		return {
			KPR2JS: path + "\\xsr6 -a " + path + "\\modules\\tools.xsa kpr2js",
			XS2JS: path + "\\xsr6 -a " +  path + "\\modules\\tools.xsa xs2js",
			XSC: path + "\\xsc6",
			XSL: path + "\\xsl6",
		
			APP_DIR: bin,
			TMP_DIR: tmp,
			
			APP_NAME: tool.application,
			RESOURCE: FS.existsSync(resource) ? resource : "$(F_HOME)\\kinoma\\kpr\\cmake\\win\\resource.rc",
			// FskPlatform.mk
			BUILD_TMP: tmp,
		};
	}
	getTargetRules(tool) {
		// return `ADD_EXECUTABLE(\${APP_NAME} \${SOURCES} \${FskPlatform_SOURCES} \${TARGET_OBJECTS})
		return `LIST(APPEND SOURCES \${RESOURCE})
ADD_EXECUTABLE(\${APP_NAME} WIN32 \${SOURCES} \${FskPlatform_SOURCES})
TARGET_LINK_LIBRARIES(\${APP_NAME} \${LIBRARIES} \${OBJECTS})

ADD_CUSTOM_COMMAND(
	TARGET \${APP_NAME}
	POST_BUILD
	COMMAND \${CMAKE_COMMAND} -E copy \$<TARGET_FILE:\${APP_NAME}> \${APP_DIR}
	)
`;
	}
};

export default Manifest;
