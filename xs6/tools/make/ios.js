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
import * as MAKE from "make";
import PLIST from "plistGrammar";
import IOS from "shared/ios";

class Manifest extends MAKE.Manifest {
	constructor(tree) {
		super(tree);
		this.ios = new IOS(tree);
	}
	generate(tool, tmp, bin) {
		this.ios.findIdentity(tool);
		this.ios.addResources(tool);
		this.ios.completeInfo(tool);
		super.generate(tool, tmp, bin);
		FS.writeFileSync(tmp + "/Entitlements.plist", PLIST.stringify(this.ios.entitlements));
		FS.writeFileSync(tmp + "/Info.plist", PLIST.stringify(this.ios.info));
	}
	getPlatformVariables(tool, tmp, bin) {
		return {
			AR: "libtool -static -o",
			AS: "clang",
			AS_OPTIONS: "-c -x assembler-with-cpp -arch armv7s -arch armv7 -MMD -DSUPPORT_NEON_IOS=1",
			AS_V7: "$(AS)",
			AS_V7_OPTIONS: "$(AS_OPTIONS)",
			CC: "clang",
			KPR2JS: "$(F_HOME)/xs6/bin/mac/debug/xsr6 -a $(F_HOME)/xs6/bin/mac/debug/modules/tools.xsa kpr2js",
			XS2JS: "$(F_HOME)/xs6/bin/mac/debug/xsr6 -a $(F_HOME)/xs6/bin/mac/debug/modules/tools.xsa xs2js",
			XSC: "$(F_HOME)/xs6/bin/mac/debug/xsc6",
			XSL: "$(F_HOME)/xs6/bin/mac/debug/xsl6",

			APP_DIR: bin + "/" + this.tree.application + ".app",
			RES_DIR: bin + "/" + this.tree.application + ".app",
			TMP_DIR: tmp,

			APP_IPA: bin + "/" + this.tree.application + ".ipa",
			CODESIGN_ALLOCATE: "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/codesign_allocate",
			IDENTITY: this.ios.identityHash,
			MANIFEST: tool.manifestPath,
			PROVISION: this.ios.provisionPath,
			// FskPlatform.mk
			ARCH: "-arch armv7 -arch armv7s -arch arm64",
			SDKROOT: "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk",
			VERSION_MIN: "-miphoneos-version-min=6.0",
		};
	}
	getTargetRules(tool, file, tmp, bin) {
		return `
all: $(APP_DIR) $(APP_DIR)/fsk $(FOLDERS) $(FILES)
	env CODESIGN_ALLOCATE=$(CODESIGN_ALLOCATE) codesign -f -v -s $(IDENTITY) --entitlements $(TMP_DIR)/Entitlements.plist $(APP_DIR)
	xcrun -sdk iphoneos PackageApplication $(APP_DIR) -o $(APP_IPA)

$(APP_DIR): $(MANIFEST) $(PROVISION)
	mkdir -p $(APP_DIR)
	cp -rf $(TMP_DIR)/Info.plist $(APP_DIR)/Info.plist
	cp -rf $(PROVISION) $(APP_DIR)/embedded.mobileprovision

$(APP_DIR)/fsk: $(TMP_DIR)/main.o $(OBJECTS)
	$(CC) $(VERSION_MIN) $(ARCH) -isysroot $(SDKROOT) -ObjC $(TMP_DIR)/main.o $(OBJECTS) $(LIBRARIES) -o $(APP_DIR)/fsk
	dsymutil $(APP_DIR)/fsk -o $(APP_DIR).dSYM

$(TMP_DIR)/main.o: $(HEADERS) $(F_HOME)/kinoma/kpr/patches/main.m
	$(CC) $(F_HOME)/kinoma/kpr/patches/main.m $(C_OPTIONS) $(C_INCLUDES) -c -o $@

`;
	}
};

export default Manifest;
