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
import PLIST from "plistGrammar";

class IOS {
	constructor(tree) {
		this.tree = tree;
	}
	addResources(tool) {
		var parts = tool.splitPath(tool.manifestPath);
		var environment = this.tree.environment;
		var path = parts.directory + "/ios/" + environment.NAMESPACE;
		if (!FS.existsSync(path)) {
			var path = parts.directory + "/ios";
			if (!FS.existsSync(path)) {
				path = parts.directory + "/iphone";
				if (!FS.existsSync(path)) {
					path = tool.resolveDirectoryPath("$(F_HOME)/build/iphone");
				}
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
	completeInfo(tool) {
		var environment = this.tree.environment;
		var info = this.tree.info;
		if (!("CFBundleDisplayName" in info))
			info.CFBundleName = environment.NAME;
		if (!("CFBundleName" in info))
			info.CFBundleName = environment.NAME;
		if (!("CFBundleVersion" in info))
			info.CFBundleVersion = environment.VERSION;
		if (!("CFBundleShortVersionString" in info))
			info.CFBundleShortVersionString = environment.VERSION;
		if (!("UIDeviceFamily" in info))
			info.UIDeviceFamily = [ 1, 2 ];
		if (!("UIRequiredDeviceCapabilities" in info))
			info.UIRequiredDeviceCapabilities = [ "armv7" ];
		if (!("UISupportedInterfaceOrientations" in info))
			info.UISupportedInterfaceOrientations = [ "UIInterfaceOrientationPortrait", "UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight"];
		if (!("UISupportedInterfaceOrientations~ipad" in info))
			info["UISupportedInterfaceOrientations~ipad"] = [ "UIInterfaceOrientationPortrait", "UIInterfaceOrientationPortraitUpsideDown", "UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight"];
		if (!("UIRrequiresFullScreen" in info))
			info.UIRrequiresFullScreen = true;
		info.CFBundleExecutable = "fsk";
		if (!("CFBundleIdentifier" in info))
			info.CFBundleIdentifier = this.entitlements["application-identifier"].replace(`${this.entitlements['com.apple.developer.team-identifier']}.`, "");
		if (!("ITSAppUsesNonExemptEncryption" in info))
			info.ITSAppUsesNonExemptEncryption = false;
		info.MinimumOSVersion = "6.0";
		// Required for App Store
		var sdkVersion = tool.execute("xcodebuild -sdk iphoneos -version");
		var xcodeVersion = tool.execute("xcodebuild -version");
		info.CFBundlePackageType = "APPL";
		info.BuildMachineOSBuild = tool.execute("sw_vers -buildVersion").trim();
		info.DTCompiler = "com.apple.compilers.llvm.clang.1_0";
		info.DTPlatformBuild = sdkVersion.match(/SDKVersion: (.*)/)[1];
		info.DTSDKBuild = sdkVersion.match(/ProductBuildVersion: (.*)/)[1];
		info.DTSDKName = "iphoneos" + info.DTPlatformBuild;
		info.DTXcode = "0" + xcodeVersion.match(/Xcode (.*)/)[1].replace(/\./g, '');
		info.DTXcodeBuild = xcodeVersion.match(/Build version (.*)/)[1];
		info.NSAppTransportSecurity = { NSAllowsArbitraryLoads: true };
		info.CFBundleSupportedPlatforms = [ "iPhoneOS" ];
		info.LSRequiresIPhoneOS = true;
		info.CFBundleIconFiles = [];
		for (let file of this.tree.otherPaths)
			if (file.destinationPath.match(/icon.*\.png/))
				info.CFBundleIconFiles.push(file.destinationPath);
		this.info = info;
	}
	findIdentity(tool) {
		var namespace = this.tree.environment.NAMESPACE;
		var text = tool.execute("security find-identity -v -p codesigning");
		var regexp = /[0-9]\) ([^ ]+) "([^"]+)"/gm;
		var identities = [];
		var isDistribution = false;
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
				if (identity.name.indexOf(tool.identityName) >= 0 || identity.hash == tool.identityName) {
					identities.push(identity);
					if (identity.name.indexOf("Distribution") > 0)
						isDistribution = true;
				}
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
				identity.certificates.push(certificate.split("\n").slice(1, -2).join(""));
		}
		var directory = process.getenv("HOME") + "/Library/MobileDevice/Provisioning Profiles",
		var names = FS.readDirSync(directory);
		var now = Date.now();
		var foundProvisions = [];
		for (var name of names) {
			if (name.indexOf(".mobileprovision") > 0) {
				var path = directory + "/" + name;
				var text = tool.execute("security cms -D -i '" + path + "'");
				var provision = PLIST.parse(text, path);
				provision.path = path;
				if (now <= provision.ExpirationDate.valueOf()) {
					if (tool.provisionName) {
						if (provision.Name == tool.provisionName || provision.path == tool.provisionName) {
							foundProvisions.push(provision);
						}
					}
					else if (provision.Entitlements["get-task-allow"] || isDistribution)
						foundProvisions.push(provision);
				}
			}
		}
		if (foundProvisions.length == 0) {
			if (tool.provisionName)
				throw new Error("no \"" + tool.provisionName + "\" provisioning profiles found!");
			throw new Error("no provisioning profiles found!");
		}
		var entitledProvisions = [];
		for (var provision of foundProvisions) {
			var entitlements = provision.Entitlements;
			var applicationIdentifier = entitlements["application-identifier"].replace("*", namespace);
			var applicationIdentifierRelease = entitlements["application-identifier"].replace("*", tool.application.toLowerCase());
			var entitlementsIdentifier =  entitlements["com.apple.developer.team-identifier"] + "." + namespace;
			if (applicationIdentifier == entitlementsIdentifier || applicationIdentifierRelease == entitlementsIdentifier) {
				if (isDistribution)
					entitlements["application-identifier"] = applicationIdentifierRelease;
				else
					entitlements["application-identifier"] = applicationIdentifier;
				if ("keychain-access-groups" in entitlements)
					entitlements["keychain-access-groups"] = entitlements["keychain-access-groups"].map(s => s.replace("*", namespace));
				entitledProvisions.push(provision);
			}
		}
		if (entitledProvisions.length == 0) {
			if (tool.provisionName)
				throw new Error("no \"" + tool.provisionName + "\" provisioning profiles entitled for \"" + namespace + "\"!");
			throw new Error("no provisioning profiles entitled for \"" + namespace + "\"!");
		}
		for (var provision of entitledProvisions) {
			for (var developerCertificate of provision.DeveloperCertificates) {
				for (var identity of identities) {
					for (var certificate of identity.certificates) {
						if (developerCertificate == certificate) {
							this.entitlements = provision.Entitlements;
							this.identityHash = identity.hash;
							this.identityName = identity.name;
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
			throw new Error("no \"" + tool.identityName + "\" code signing identities certified for provisioning profiles!");
		}
		if (tool.provisionName)
			throw new Error("no code signing identities certified for \"" + tool.provisionName + "\" provisioning profiles!");
		throw new Error("no code signing identities certified for provisioning profiles!");
	}
};

export default IOS;
