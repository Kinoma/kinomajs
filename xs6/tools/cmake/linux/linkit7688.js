
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
		return [];
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
