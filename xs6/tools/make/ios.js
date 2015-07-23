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
import * as MAKE from "make";
import PLIST from "plistGrammar";

class Manifest extends MAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	addResources(tool) {
		var parts = tool.splitPath(tool.manifestPath);
		var path = parts.directory + "/ios";
		if (!FS.existsSync(path)) {
			path = parts.directory + "/iphone";
			if (!FS.existsSync(path)) {
				path = tool.resolveDirectoryPath("$(F_HOME)/build/iphone");
			}
		}
		var files = this.tree.otherPaths;
		var names = FS.readDirSync(path);
		for (var name of names) {
			if (name.endsWith(".png"))
				files.push({
					sourcePath: path + "/" + name,
					destinationPath: name
				});
		}
	}
	completeInfo() {
		var environment = this.tree.environment;
		var info = this.tree.info;
		if (!("CFBundleDisplayName" in info))
			info.CFBundleName = environment.NAME;
		if (!("CFBundleName" in info))
			info.CFBundleName = environment.NAME;
		if (!("CFBundleVersion" in info))
			info.CFBundleVersion = environment.VERSION;
		if (!("UIDeviceFamily" in info))
			info.UIDeviceFamily = [ 1, 2 ];
		if (!("UIRequiredDeviceCapabilities" in info))
			info.UIRequiredDeviceCapabilities = [ "armv7" ];
		if (!("UISupportedInterfaceOrientations" in info))
			info.UISupportedInterfaceOrientations = [ "UIInterfaceOrientationPortrait", "UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight"];
		if (!("UISupportedInterfaceOrientations~ipad" in info))
			info["UISupportedInterfaceOrientations~ipad"] = [ "UIInterfaceOrientationPortrait", "UIInterfaceOrientationPortraitUpsideDown", "UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight"];
		info.CFBundleExecutable = "fsk";
		info.CFBundleIdentifier = this.entitlements["application-identifier"];
		info.CFBundleSupportedPlatforms = [ "iPhoneOS" ];
		info.LSRequiresIPhoneOS = true;
		this.info = info;
	}
	findIdentity(tool) {
		var namespace = this.tree.environment.NAMESPACE;
		var text = tool.execute("security find-identity -v -p codesigning");
		var regexp = /[0-9]\) ([^ ]+) "([^"]+)"/gm;
		var identities = [];
		for (;;) {
			var results = regexp.exec(text);
			if (!results)
				break;
			var identity = {
				hash: results[1],
				name: results[2],
				certificates: [],
			}
			if (tool.identityName) {
				if (identity.name == tool.identityName)
					identities.push(provisions);
			}
			else
				identities.push(identity);
		}
		if (identities.length == 0) {
			if (tool.identityName)
				throw new Error("no \"" + tool.identityName + "\" code signing identities found!");
			throw new Error("no code signing identities found!");
		}
		for (var identity of identities) {
			var text = tool.execute("security find-certificate -a -c '" + identity.name  + "' -p");
			var certificates = text.split("-----BEGIN CERTIFICATE-----").slice(1);
			for (var certificate of certificates)
				identity.certificates.push(text.split("\n").slice(1, -2).join(""));
		}
		var directory = process.getenv("HOME") + "/Library/MobileDevice/Provisioning Profiles",
		var names = FS.readDirSync(directory);
		var now = Date.now();
		var foundProvisions = [];
		for (var name of names) {
			var path = directory + "/" + name;
			var text = tool.execute("security cms -D -i '" + path + "'");
			var provision = PLIST.parse(text, path);
			provision.path = path;
			if (now <= provision.ExpirationDate.valueOf()) {
				if (tool.provisionName) {
					if (provision.Name == tool.provisionName)
						foundProvisions.push(provision);
				}
				else if (provision.Entitlements["get-task-allow"])
					foundProvisions.push(provision);
			}
		}
		if (foundProvisions.length == 0) {
			if (tool.provisionName)
				throw new Error("no \"" + tool.provisionName + "\" provisioning profiles found!");
			throw new Error("no development provisioning profiles found!");
		}
		var entitledProvisions = [];
		for (var provision of foundProvisions) {
			var entitlements = provision.Entitlements;
			var applicationIdentifier = entitlements["application-identifier"].replace("*", namespace);
			if (applicationIdentifier == entitlements["com.apple.developer.team-identifier"] + "." + namespace) {
				entitlements["application-identifier"] = applicationIdentifier;
				if ("keychain-access-groups" in entitlements)
					entitlements["keychain-access-groups"] = entitlements["keychain-access-groups"].map(s => s.replace("*", namespace));
				entitledProvisions.push(provision);
			}
		}
		if (entitledProvisions.length == 0) {
			if (tool.provisionName)
				throw new Error("no \"" + tool.provisionName + "\" provisioning profiles entitled for \"" + namespace + "\"!");
			throw new Error("no development provisioning profiles entitled for \"" + namespace + "\"!");
		}
		for (var provision of entitledProvisions) {
			for (var developerCertificate of provision.DeveloperCertificates) {
				for (var identity of identities) {
					for (var certificate of identity.certificates) {
						if (developerCertificate == certificate) {
							this.entitlements = provision.Entitlements;						
							this.identityHash = identity.hash;	
							this.provisionPath = provision.path.replace(" ", "\\ ");
							return;
						}
					}
				}
			}
		}
		if (tool.identityName) {
			if (tool.provisionName)
				throw new Error("no \"" + tool.identityName + "\" code signing identities certified for \"" + tool.provisionName + "\" provisioning profiles!");
			throw new Error("no \"" + tool.identityName + "\" code signing identities certified for development provisioning profiles!");
		}
		if (tool.provisionName)
			throw new Error("no code signing identities certified for \"" + tool.provisionName + "\" provisioning profiles!");
		throw new Error("no code signing identities certified for development provisioning profiles!");
	}
	generate(tool, tmp, bin) {
		this.findIdentity(tool);
		this.completeInfo();
		this.addResources(tool);
		super.generate(tool, tmp, bin);
		FS.writeFileSync(tmp + "/Entitlements.plist", PLIST.stringify(this.entitlements));
		FS.writeFileSync(tmp + "/Info.plist", PLIST.stringify(this.info));
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
			TMP_DIR: tmp,
			
			APP_IPA: bin + "/" + this.tree.application + ".ipa",
			CODESIGN_ALLOCATE: "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/codesign_allocate",
			IDENTITY: this.identityHash,
			MANIFEST: tool.manifestPath,
			PROVISION: this.provisionPath,
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
