/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
import PLIST from "plistGrammar";
import IOS from "shared/ios";

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
		this.ios = new IOS(tree);
	}
	getIDEGenerator() {
		return "Xcode";
	}
	openIDE(tool, path) {
		process.then("open", `${path}${tool.slash}fsk.xcodeproj`);
	}
	getPlatformLanguages() {
		return ["ASM"];
	}
	generate(tool, tmp, bin) {
		this.ios.findIdentity(tool);
		this.ios.addResources(tool);
		this.ios.completeInfo(tool);
		this.ios.info.CFBundleExecutable = this.tree.application;
		super.generate(tool, tmp, bin);
		FS.writeFileSync(tmp + "/Entitlements.plist", PLIST.stringify(this.ios.entitlements));
		FS.writeFileSync(tmp + "/Info.plist", PLIST.stringify(this.ios.info));
	}
	getPlatformVariables(tool, tmp, bin) {
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		return {
			APP_DIR: tool.outputPath + "/bin/" + tool.platform + "/${CONFIG_TYPE}/" + this.tree.application + "/" + this.tree.application + ".app",
			TMP_DIR: tmp,

			APP_IPA: tool.outputPath + "/bin/" + tool.platform + "/${CONFIG_TYPE}/" + this.tree.application + "/" + this.tree.application + ".ipa",
			MANIFEST: tool.manifestPath,
			PROVISION: this.ios.provisionPath,
		};
	}
	getTargetRules(tool, file, tmp, bin) {
		var application = this.tree.application;
		var identity = this.ios.identityName;
		var bundleIdentifier = this.ios.info.CFBundleIdentifier;
		return `
BUILD(APPLICATION ${this.tree.application} IDENTITY "${this.ios.identityName}" IDENTIFIER this.ios.info.CFBundleIdentifier HASH ${this.ios.identityHash})
`;
	}
};

export default Manifest;
