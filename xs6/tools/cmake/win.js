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
	generateRules(tool, file, path) {
		file.line(`add_library(${this.name} STATIC \${${this.name}_SOURCES})`);
		file.line(`set_target_properties(${this.name} PROPERTIES STATIC_LIBRARY_FLAGS /LTCG)`);
		file.line(`add_dependencies(${this.name} FskManifest.xsa)`);
		file.line(`list(APPEND OBJECTS ${this.name})`);
		file.line(`set(OBJECTS \${OBJECTS} PARENT_SCOPE)`);
	}
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
			APP_DIR: `${tool.outputPath}/bin/${tool.platform}/$<CONFIG>/${tool.application}`,
			TMP_DIR: tmp,
			
			APP_NAME: tool.application,
			RESOURCE: FS.existsSync(resource) ? resource : "$(F_HOME)\\kinoma\\kpr\\cmake\\win\\resource.rc",
			// FskPlatform.mk
			BUILD_TMP: tmp,
		};
	}
	getTargetRules(tool) {
		return `LIST(APPEND SOURCES \${RESOURCE})
add_executable(\${APP_NAME} WIN32 \${SOURCES} \${FskPlatform_SOURCES})
target_link_libraries(\${APP_NAME} \${LIBRARIES} \${OBJECTS})

add_custom_command(
	TARGET \${APP_NAME}
	POST_BUILD
	COMMAND \${CMAKE_COMMAND} -E make_directory \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${RES_DIR}/ \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy_directory \${TMP_DIR}/app \$<TARGET_FILE_DIR:\${APP_NAME}>
	COMMAND \${CMAKE_COMMAND} -E make_directory \${APP_DIR}
	COMMAND \${CMAKE_COMMAND} -E copy \$<TARGET_FILE:\${APP_NAME}> \${APP_DIR}
	)
`;
	}
};

export default Manifest;
