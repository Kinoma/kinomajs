import * as FS from "fs";
import * as CMAKE from "cmake";
import PLIST from "plistGrammar";

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	getPlatformLanguages() {
		return ["ASM"];
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
		info.CFBundleExecutable = this.tree.application;
		info.CFBundleIdentifier = this.entitlements["application-identifier"].replace(/^[^\.]*\./, '');
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
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release";
		return {
			KPR2JS: path + `/xsr6 -a ${path}/modules/tools.xsa kpr2js`,
			XS2JS: path + `/xsr6 -a ${path}/modules/tools.xsa xs2js`,
			XSC: `${path}/xsc6`,
			XSL: `${path}/xsl6`,

			APP_DIR: `${bin}/${this.tree.application}.app`,
			TMP_DIR: tmp,

			APP_IPA: `${bin}/${this.tree.application}.ipa`,
			MANIFEST: tool.manifestPath,
			PROVISION: this.provisionPath,
		};
	}
	getTargetRules(tool, file, tmp, bin) {
		var application = this.tree.application;
		var identifier = this.info.CFBundleIdentifier;
		var identity = this.identityName;
		var output = "";
		if (tool.ide)
			output += `
SET(APP_TYPE MACOSX_BUNDLE)
`;

		output += `
ADD_EXECUTABLE(${application} \${APP_TYPE} \${SOURCES} \${FskPlatform_SOURCES} \${F_HOME}/kinoma/kpr/patches/main.m)
TARGET_LINK_LIBRARIES(${application} \${LIBRARIES} \${OBJECTS} -ObjC)
`;
	if (tool.ide) {
		output += `
SET_TARGET_PROPERTIES(${application}
	PROPERTIES
	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${this.identityName}"
	MACOSX_BUNDLE_INFO_PLIST \${TMP_DIR}/Info.plist
	MACOSX_BUNDLE_GUI_IDENTIFIER ${identifier}
	)
`;
	}

	output += `
COPY(SOURCE \${TMP_DIR}/Info.plist DESTINATION \${APP_DIR}/Info.plist)
COPY(SOURCE \${PROVISION} DESTINATION \${APP_DIR}/embedded.mobileprovision)

ADD_CUSTOM_COMMAND(
	TARGET ${application}
	POST_BUILD
	`;
	if (tool.ide) {
		output += `COMMAND \${CMAKE_COMMAND} -E copy_directory \${APP_DIR} \${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>-iphoneos/${application}.app
	COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>-iphoneos/${application}.app/${application} \${APP_DIR}`;
	} else {
		output += `COMMAND \${CMAKE_COMMAND} -E copy  $<TARGET_FILE:${application}> \${APP_DIR}
	COMMAND dsymutil $<TARGET_FILE:${application}> -o $<TARGET_FILE:${application}>.dSYM`;
	}
	output += `
	COMMAND codesign -f -v -s "${identity}" --entitlements \${TMP_DIR}/Entitlements.plist \${APP_DIR}
	COMMAND xcrun -sdk iphoneos PackageApplication \${APP_DIR} -o \${APP_IPA}
	VERBATIM
	)
`;
		return output;
	}
};

export default Manifest;
