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
			KPR_APPLICATION: this.tree.environment.KPR_BINARY_NAME ? this.tree.environment.KPR_BINARY_NAME : tool.application,
			APP_DIR: tool.outputPath + "/bin/" + tool.platform + "/${CMAKE_BUILD_TYPE}/" + tool.application,
		}
	}
	getTargetRules(tool) {
		return `
BUILD(APPLICATION ${tool.application})
`;
	}
};

export default Manifest;
