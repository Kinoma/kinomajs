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

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	getGenerator() {
		return "Unix Makefiles";
	}
	getIDEGenerator() {
		return "Xcode";
	}
	openIDE(tool, path) {
		process.then("open", `${path}${tool.slash}fsk.xcodeproj`);
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var icns = parts.directory + "/mac/fsk.icns";
		var nib = parts.directory + "/mac/fsk.nib";
		var plist = parts.directory + "/mac/fsk.plist";
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		return {
			APP_NAME: tool.application,

			APP_DIR: tool.outputPath + "/bin/" + tool.platform + "/${CONFIG_TYPE}/" + this.tree.application + ".app/Contents/MacOS",
			BUILD_APP_DIR: "${TMP_DIR}/${CMAKE_CFG_INTDIR}/" + this.tree.application + ".app/Contents/MacOS",
			
			ICNS: FS.existsSync(icns) ? icns : "$(F_HOME)/build/mac/fsk.icns",
			NIB: FS.existsSync(nib) ? nib : "$(F_HOME)/build/mac/fsk.nib",
			// FskPlatform.mk
		};
	}
	getTargetRules(tool) {
		var application = this.tree.application;
		var namespace = this.tree.environment.NAMESPACE;
		var year = new Date().getFullYear();
		if (!namespace)
			namespace = `com.marvell.kinoma.${application.toLowerCase()}`;

		return `
BUILD(APPLICATION ${application} NAMESPACE ${namespace} YEAR ${year})
`;
	return output;
	}
}

export default Manifest;
