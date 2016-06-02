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

class Makefile extends MAKE.Makefile {
	constructor(tree) {
		super(tree);
		this.objects = "$(TMP_DIR)\\" + this.name + ".lib";
	}
	generateRule(tool, file, path) {
		var parts = tool.splitPath(path);
		file.write("$(TMP_DIR)\\");
		file.write(this.name);
		file.write("\\");
		file.write(parts.name);
		file.write(".o: $(HEADERS) $(");
		file.write(this.name);
		file.write("_HEADERS) ");
		file.write(path);
		file.write("\n\tcl ");
		file.write(path);
		file.write(" $(C_OPTIONS) $(");
		file.write(this.name);
		file.write("_C_OPTIONS) ");
		if (this.separate) {
			file.write("-UFSK_EMBED -UFSK_EXTENSION_EMBED ");
		}
		file.write("$(");
		file.write(this.name);
		file.write("_C_INCLUDES) $(C_INCLUDES) /Fo$@\n");
	}
	generateRules(tool, file) {
		file.write("$(TMP_DIR)\\");
		file.write(this.name);
		file.write(".lib: $(");
		file.write(this.name);
		file.write("_OBJECTS)\n\tlib /NOLOGO /OUT:$(TMP_DIR)\\");
		file.write(this.name);
		file.write(".lib $(");
		file.write(this.name);
		file.write("_OBJECTS)\n");
		for (let item of this.sources)
			this.generateRule(tool, file, item);
	}
};

class Manifest extends MAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	get Makefile() {
		return Makefile;
	}
	generateManifestRules(tool, file) {
		file.line("$(TMP_DIR)\\FskManifest.o: $(HEADERS) ", tool.manifestPath);
		file.line("\tcl $(TMP_DIR)\\FskManifest.c $(C_OPTIONS) $(C_INCLUDES) /Fo$@");
		file.line("$(TMP_DIR)\\FskManifest.xs.o: $(HEADERS) $(TMP_DIR)\\FskManifest.xs.c");
		file.line("\tcl $(TMP_DIR)\\FskManifest.xs.c $(C_OPTIONS) $(C_INCLUDES) /Fo$@");
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var resource = parts.directory + "\\win\\resource.rc";
		var path = process.debug ? "$(F_HOME)\\xs6\\bin\\win\\debug" : "$(F_HOME)\\xs6\\bin\\win\\release"
		return {
			KPR2JS: path + "\\xsr6 -a " + path + "\\modules\\tools.xsa kpr2js",
			XS2JS: path + "\\xsr6 -a " +  path + "\\modules\\tools.xsa xs2js",
			XSC: path + "\\xsc6",
			XSL: path + "\\xsl6",
		
			APP_DIR: bin,
			RES_DIR: bin,
			TMP_DIR: tmp,
			
			APP:this.tree.application,
			APP_EXE: bin + "\\" + this.tree.application + ".exe",
			RESOURCE: FS.existsSync(resource) ? resource : "$(F_HOME)\\build\\win\\resource.rc",
			// FskPlatform.mk
			BUILD_TMP: tmp,
		};
	}
	getTargetRules(tool) {
		return `
all: $(APP_EXE) $(FOLDERS) $(FILES)

$(APP_EXE): $(TMP_DIR)\\resource.res $(OBJECTS)
	link /OUT:$(APP_EXE) $(LIBRARIES) $(TMP_DIR)\\resource.res $(OBJECTS)
	
$(TMP_DIR)\\resource.res: $(RESOURCE)
	rc /fo$(TMP_DIR)\\resource.res $(RESOURCE) 
	
`;
	}
};

export default Manifest;
