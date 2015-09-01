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
