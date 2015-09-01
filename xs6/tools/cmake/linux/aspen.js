import * as FS from "fs";
import * as MAKE from "cmake";

class Makefile extends MAKE.Makefile {
	constructor(tree) {
		super(tree);
		this.objects = "$(" + this.name + "_OBJECTS)";
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
			KPR2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa kpr2js",
			XS2JS: path + "/xsr6 -a " +  path + "/modules/tools.xsa xs2js",
			XSC: path + "/xsc6",
			XSL: path + "/xsl6",

			KPR_APPLICATION: tool.application,
			KPR_CONFIGURATION: tool.debug ? "debug" : "release",


			APP_DIR: bin,
		}
	}
	getTargetRules(tool) {
		return `
FILE(MAKE_DIRECTORY \${FREETYPE_DIR})

ADD_SUBDIRECTORY(\${F_HOME}/libraries/freetype/xs6 FreeType)

ADD_EXECUTABLE(Kpl \${SOURCES} \${FskPlatform_SOURCES} \${TARGET_OBJECTS})
ADD_DEPENDENCIES(Kpl FreeType)
TARGET_INCLUDE_DIRECTORIES(Kpl  PRIVATE \${C_INCLUDES})
TARGET_COMPILE_DEFINITIONS(Kpl PRIVATE \${C_DEFINITIONS})
TARGET_COMPILE_OPTIONS(Kpl PRIVATE \${C_OPTIONS})
TARGET_LINK_LIBRARIES(Kpl -Wl,--whole-archive -Wl,-Map,\${TMP_DIR}/Kpl.map \${OBJECTS} \${LIBRARIES})

IF(RELEASE)
	ADD_CUSTOM_COMMAND(
			TARGET Kpl
			POST_BUILD
			COMMAND \${TOOLCHAIN_BIN}/\${TOOL_PREFIX}strip \$<TARGET_FILE:Kpl>
			)
ENDIF()

ADD_CUSTOM_COMMAND(
		TARGET Kpl
		POST_BUILD
		COMMAND \${CMAKE_COMMAND} -E copy $<TARGET_FILE:Kpl> \${APP_DIR}
		COMMAND \${CMAKE_COMMAND} -E tar czf \${KPR_APPLICATION}.tgz \${KPR_APPLICATION}
		WORKING_DIRECTORY \${APP_DIR}/..
		)
`;
	}
};

export default Manifest;
