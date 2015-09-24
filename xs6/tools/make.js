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

class File {
	constructor(path) {
		this.fd = FS.openSync(path, "w");
		this.slash = "/";
	}
	close() {
		FS.closeSync(this.fd);
		delete this.fd;
	}
	line(...strings) {
		for (var string of strings)
			this.write(string);
		this.write("\n");
	}
	write(string) {
		FS.writeSync(this.fd, string);
	}
}

export class Makefile {
	constructor(tree) {
		this.name = tree.name;
		this.cIncludes = tree.cIncludes;
		this.cOptions = tree.cOptions;
		this.cOptionsDebug = tree.cOptionsDebug;
		this.cOptionsRelease = tree.cOptionsRelease;
		this.headers = tree.headers;
		this.libraries = tree.libraries;
		this.objects = "$(TMP_DIR)/" + this.name + ".a";
		this.separate = tree.separate;
		this.sources = tree.sources;
		this.xs = tree.xs;
	}
	generateRule(tool, file, path) {
		var parts = tool.splitPath(path);
		if (parts.extension == ".gas") {
			file.write("$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".gas.o: $(HEADERS) $(");
			file.write(this.name);
			file.write("_HEADERS) ");
			file.write(path);
			if (parts.name.indexOf("neon") >= 0)
				file.write("\n\t$(AS_NEON) $(AS_NEON_OPTIONS) ");
			else
				file.write("\n\t$(AS) $(AS_OPTIONS) ");
			file.write(path);
			file.write(" -o $@\n");
		}
		else if (parts.extension == ".gas7") {
			file.write("$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".gas7.o: $(HEADERS) $(");
			file.write(this.name);
			file.write("_HEADERS) ");
			file.write(path);
			file.write("\n\t$(AS_V7) $(AS_V7_OPTIONS) ");
			file.write(path);
			file.write(" -o $@\n");
		}
		else if (parts.extension == ".wmmx") {
			file.write("$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".wmmx.o: $(HEADERS) $(");
			file.write(this.name);
			file.write("_HEADERS) ");
			file.write(path);
			file.write("\n\t$(AS_WMMX) $(AS_WMMX_OPTIONS) ");
			file.write(path);
			file.write(" -o $@\n");
		}
		else {
			file.write("$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".o: $(HEADERS) $(");
			file.write(this.name);
			file.write("_HEADERS) ");
			file.write(path);
			file.write("\n\t$(CC) ");
			file.write(path);
			file.write(" $(C_OPTIONS) $(");
			file.write(this.name);
			file.write("_C_OPTIONS) ");
			if (this.separate) {
				file.write("-UFSK_EMBED -UFSK_EXTENSION_EMBED ");
			}
			file.write("$(C_INCLUDES) ");
			file.write("$(");
			file.write(this.name);
			file.write("_C_INCLUDES) -c -o $@\n");
		}
	}
	generateRules(tool, file) {
		file.write("$(TMP_DIR)/");
		file.write(this.name);
		file.write(".a: $(");
		file.write(this.name);
		file.write("_OBJECTS)\n\t$(AR) $(TMP_DIR)/");
		file.write(this.name);
		file.write(".a $(");
		file.write(this.name);
		file.write("_OBJECTS)\n");
		for (let item of this.sources)
			this.generateRule(tool, file, item);
	}
	generateSeparateRules(tool, file) {
		file.write("$(SEPARATE_DIR)/lib" + this.name + ".so: " + "$(" + this.name + "_OBJECTS)\n");
		file.write("\t$(LINK) ");
		file.write("-Wl,-soname,lib" + this.name + ".so $(SEPARATE_LINK_OPTIONS) ");
		file.write("$(SEPARATE_LIBRARIES) $(" + this.name + "_LIBRARIES) ");
		file.write("$(" + this.name + "_OBJECTS) ");
		file.write("-o $@\n");
	}
	generateVariable(tool, file, path) {
		var parts = tool.splitPath(path);
		if (parts.extension == ".gas") {
			file.write(" \\\n\t$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".gas.o");
		}
		else if (parts.extension == ".gas7") {
			file.write(" \\\n\t$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".gas7.o");
		}
		else if (parts.extension == ".wmmx") {
			file.write(" \\\n\t$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".wmmx.o");
		}
		else if (tool.windows) {
			file.write(" \\\n\t$(TMP_DIR)\\");
			file.write(this.name);
			file.write("\\");
			file.write(parts.name);
			file.write(".o");
		}
		else {
			file.write(" \\\n\t$(TMP_DIR)/");
			file.write(this.name);
			file.write("/");
			file.write(parts.name);
			file.write(".o");
		}
	}
	generateVariables(tool, file, tmp) {
		file.write(this.name + "_C_INCLUDES = ")
		if (tool.windows) {
			for (let item of this.cIncludes) {
				file.write(" \\\n\t/I");
				file.write(item);
			}
		}
		else {
			for (let item of this.cIncludes) {
				file.write(" \\\n\t-I");
				file.write(item);
			}
		}
		file.write(" \\\n\t-I$(TMP_DIR)/");
		file.write(this.name);
		file.write("\n");
			
		file.write(this.name + "_C_OPTIONS = ");
		for (let item of this.cOptions) {
			file.write(" \\\n\t");
			file.write(item);
		}
		file.write("\n");
		file.write(this.name + "_HEADERS = ");
		for (let item of this.headers) {
			file.write(" \\\n\t");
			file.write(item);
		}
		file.write("\n");
		if (this.sources.length) {
			file.write(this.name + "_OBJECTS = ")
			for (let item of this.sources)
				this.generateVariable(tool, file, item);
			file.write("\n");
		}
		if (this.separate) {
			file.write(this.name + "_LIBRARIES = ")
			for (let library of this.libraries) {
				file.write(" \\\n\t");
				file.write(library);
			}
			file.write(" \n");
		}
	}
};

export class Manifest {
	constructor(tree) {
		this.xsIncludes = tree.xsIncludes;
		this.xsOptions = tree.xsOptions;
		this.xsSources = tree.xsSources;
		this.makefiles = tree.makefiles.map(makefile => new this.Makefile(makefile));
		this.tree = tree;
	}
	get Makefile() {
		return Makefile;
	}
	generate(tool, tmp, bin) {
		this.generateDirectories(tool, tmp);
		this.generateC(tool, tmp);
		this.generateXS(tool, tmp);
		this.generateMAKE(tool, tmp, bin);
	}
	generateC(tool, tmp) {
		var path = tool.joinPath({ directory: tmp, name: "FskManifest", extension: ".c" });
		var file = new File(path);
		file.write('/* WARNING: This file is automatically generated by config. Do not edit. */\n');
		
		file.write('#include "Fsk.h"\n');
		file.write('#include "FskECMAScript.h"\n');
		file.write('#include "FskEnvironment.h"\n');
		file.write('#include "FskExtensions.h"\n');
		file.write('#include "FskFiles.h"\n');
		file.write('#include "FskHardware.h"\n');
		file.write('#include "FskInstrumentation.h"\n');
		file.write('#include "FskMemory.h"\n');
		file.write('#include "FskString.h"\n');
		file.write('#define __FSKTEXT_PRIV__ 1\n');
		file.write('#include "FskText.h"\n');
		file.write('#include "xs.h"\n');
		file.write('\n');
		file.write('extern FskErr FskExtensionLoad(const char *name);\n');
		file.write('extern FskErr FskExtensionUnload(const char *name);\n');
		for (let makefile of this.makefiles) {
			if (!makefile.separate) {
				file.write('extern FskErr ');
				file.write(makefile.name);
				file.write('_fskLoad(FskLibrary library);\n');
				file.write('extern FskErr ');
				file.write(makefile.name);
				file.write('_fskUnload(FskLibrary library);\n');
			}
		}
		file.write('\n');
		file.write('void FskExtensionsEmbedLoad(char *vmName)\n');
		file.write('{\n');
		file.write('\tchar* value;\n');
//			file.write('\tFskTextEngine fte;\n');
		
		let instrument = this.tree.instrument;
		if (instrument) {
			file.write('#if SUPPORT_INSTRUMENTATION\n');
			file.write('\tFskInstrumentationSimpleClientConfigure(');
			file.write(instrument.trace ? '1, ' : '0, ');
			file.write(instrument.threads ? '1, ' : '0, ');
			file.write(instrument.times ? '1, "' : '0, "');
			file.write(instrument.log);
			file.write('", "');
			file.write(instrument.syslog);
			file.write(instrument.androidlog ? '", 1);\n' : '", 0);\n');
			var kinds = instrument.kinds;
			var c = kinds.length;
			for (var i = 0; i < c; i++) {
				var kind = kinds[i];
				file.write('\tFskInstrumentationSimpleClientAddType("');
				file.write(kind.type);
				file.write('", ');
				if (kind.flags) {
					file.write('(');
					file.write(kind.flags);
					file.write('<< 16) | ');
				}
				switch (kind.messages) {
				case "normal": file.write('kFskInstrumentationLevelUpToNormal);\n'); break;
				case "minimal": file.write('kFskInstrumentationLevelUpToMinimal);\n'); break;
				case "verbose": file.write('kFskInstrumentationLevelUpToVerbose);\n'); break;
				case "debug": file.write('kFskInstrumentationLevelUpToDebug);\n'); break;
				default:  file.write('0);\n'); break;
				}
			}
			file.write('#endif\n');
		}
		var buildPropertiesPath = tool.homePath + "/kinoma/kpr/cmake/build.properties";
		var buildProperties = FS.readFileSync(buildPropertiesPath);
		if (!buildProperties)
			xsTool.reportError(null, 0, "Unable to read " + buildPropertiesPath);
		var coreVersion = buildProperties.match(/core\.version=([0-9.]*)/);
		if (coreVersion[1])
			this.tree.environment["CORE_VERSION"] = coreVersion[1];

		let environment = this.tree.environment;
		for (var name in environment) {
			file.write('\tvalue = FskEnvironmentDoApply(FskStrDoCopy("');
			file.write(environment[name]);
			file.write('"));\n');
			file.write('\tFskEnvironmentSet("');
			file.write(name);
			file.write('", value);\n');
			file.write('\tFskMemPtrDispose(value);\n');
		}
		var split = tool.platform.split("/");
		var platform = split[0];
		if ((platform == "android") || (platform == "linux") || (platform == "threadx") || (platform == "win")) {
			let fonts = this.tree.fonts;
			if (!fonts)
				tool.reportError(null, 0, "no fonts!");
			else if ("path" in fonts) {
				file.write('\tFskTextFreeTypeInstallFonts("');
				file.write(fonts.path);
				if ("default" in fonts) {
					file.write('", "');
					file.write(fonts.default);
					file.write('");\n');
				}
				else {
					if ((platform == "linux") || (platform == "threadx")) {
						tool.reportError(null, 0, "No default font found!");
					}
					file.write('", NULL);\n');
				}
			}
			if (platform == "android") {
				file.write('\tFskTextFreeTypeInstallFonts("/system/fonts/", NULL);\n');
			}
		}
		
		for (let makefile of this.makefiles) {
			if (!makefile.separate) {
				file.write('\t');
				file.write(makefile.name);
				file.write('_fskLoad(NULL);\n');
			}
			else {
				file.write('\tFskExtensionLoad("');
				file.write(makefile.name);
				file.write('");\n');
			}
		}
		
		file.write('}\n');
		file.write('\n');
		file.write('void FskExtensionsEmbedUnload(char *vmName)\n');
		file.write('{\n');
		for (let makefile of this.makefiles) {
			if (!makefile.separate) {
				file.write('\t');
				file.write(makefile.name);
				file.write('_fskUnload(NULL);\n');
			}
			else {
				file.write('\tFskExtensionUnload("');
				file.write(makefile.name);
				file.write('");\n');
			}
		}
		file.write('}\n');
		
		file.close();
	}
	generateDirectories(tool, tmp, bin) {
		for (let item of this.tree.directoryPaths) {
			FS.mkdirSync(tmp + tool.slash + item);
		}
		for (let item of this.makefiles) {
			FS.mkdirSync(tmp + tool.slash + item.name);
		}
	}
	generateMAKE(tool, tmp, bin) {
		var path = tool.joinPath({ directory: tmp, name: "makefile", extension: "" });
		var file = new File(path);
		
		file.write('# WARNING: This file is automatically generated by config. Do not edit. #\n');
		if (tool.platform != "win")
			file.line(".NOTPARALLEL:");
		if (tool.windows) {
		}
		else {
			file.write("% : %.c\n");
			file.write("%.o : %.c\n");
		}
		this.generatePlatformVariables(tool, file, tmp, bin);

		file.write("TMP_DIR = ");
		file.write(tmp);
		file.write("\n");
		file.write("SUPPORT_XS_DEBUG=" + (tool.debug || this.tree.xsdebug.enabled || tool.xsdebug ? 1 : 0) + "\n");
	
		this.generateXSVariables(tool, file);
		this.generateResourcesVariables(tool, file);
		for (let makefile of this.makefiles)
			makefile.generateVariables(tool, file, tmp);
		
		if (tool.windows) {
			file.write("C_INCLUDES = /I$(F_HOME)\\xs6\\includes /I$(TMP_DIR) $(FskPlatform_C_INCLUDES)\n");
			file.write("C_OPTIONS = /Fd$(TMP_DIR)\\fsk.pdb /DXS6=1 $(FskPlatform_C_OPTIONS)");
			file.write(" /DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)\n");
			file.write("HEADERS = $(F_HOME)\\xs6\\includes\\xs.h $(FskPlatform_HEADERS)\n");
		}
		else {
			file.write("C_INCLUDES = -I$(F_HOME)/xs6/includes -I$(TMP_DIR) $(FskPlatform_C_INCLUDES)\n");
			file.write("C_OPTIONS = -DXS6=1 $(FskPlatform_C_OPTIONS)");
			file.write(" -DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)\n");
			file.write("HEADERS = $(F_HOME)/xs6/includes/xs.h $(FskPlatform_HEADERS)\n");
		}
		file.write("LIBRARIES =");
		for (let item of this.tree.libraries) {
			file.write(" \\\n\t");
			file.write(item);
		}
		file.write("\n");
		file.write("SEPARATE =");
		for (let makefile of this.makefiles) {
			if (makefile.separate) {
				file.write(" \\\n\t");
				file.write("$(SEPARATE_DIR)/lib" + makefile.name + ".so");
			}
		}
		file.write("\n");
		file.write("OBJECTS =");
		file.write(" \\\n\t$(TMP_DIR)");
		file.write(file.slash);
		file.write("FskManifest.xs.o");
		file.write(" \\\n\t$(TMP_DIR)");
		file.write(file.slash);
		file.write("FskManifest.o");
		for (let makefile of this.makefiles) {
			if (makefile.sources.length) {
				if (!makefile.separate) {
					file.write(" \\\n\t");
					file.write(makefile.objects);
				}
			}
		}
		file.write("\n");
		
		this.generateTargetRules(tool, file);
		this.generateManifestRules(tool, file);
		this.generateXSRules(tool, file);
		this.generateResourcesRules(tool, file);
		for (let makefile of this.makefiles) {
			if (makefile.sources.length) {
				makefile.generateRules(tool, file, tmp);
				if (makefile.separate)
					makefile.generateSeparateRules(tool, file, tmp);
			}
		}
		file.close();
	}
	generateManifestRules(tool, file) {
		file.line("$(TMP_DIR)/FskManifest.o: $(HEADERS) ", tool.manifestPath);
		file.line("\t$(CC) $(TMP_DIR)/FskManifest.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@");
		file.line("$(TMP_DIR)/FskManifest.xs.o: $(HEADERS) $(TMP_DIR)/FskManifest.xs.c");
		file.line("\t$(CC) $(TMP_DIR)/FskManifest.xs.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@");
	}
	generatePlatformVariables(tool, file, tmp, bin) {
		var variables = this.getPlatformVariables(tool, tmp, bin);
		for (let name in variables)
			file.line(name, " = ", variables[name]);
	}
	generateResourcesRules(tool, file) {
		for (let item of this.tree.xmlPaths) {
			let parts = tool.splitPath(item.destinationPath);
			file.line("$(TMP_DIR)", tool.slash, item.destinationPath, ".js : ", item.sourcePath);
			file.line("\t$(KPR2JS) ", item.sourcePath, " -o $(TMP_DIR)", tool.slash, parts.directory);
			file.line("$(TMP_DIR)", tool.slash, item.destinationPath, ".xsb : $(TMP_DIR)", tool.slash, item.destinationPath, ".js")
			if (tool.debug)
				file.line("\t$(XSC) -d ", "$(TMP_DIR)", tool.slash, item.destinationPath, ".js -o $(TMP_DIR)", tool.slash, parts.directory);
			else
				file.line("\t$(XSC) ", "$(TMP_DIR)", tool.slash, item.destinationPath, ".js -o $(TMP_DIR)", tool.slash, parts.directory);
		}
		for (let item of this.tree.jsPaths) {
			let parts = tool.splitPath(item.destinationPath);
			file.line("$(TMP_DIR)", tool.slash, item.destinationPath, ".xsb : ", item.sourcePath);
			if (tool.debug)
				file.line("\t$(XSC) -d ", item.sourcePath, " -o $(TMP_DIR)", tool.slash, parts.directory);
			else
				file.line("\t$(XSC) ", item.sourcePath, " -o $(TMP_DIR)", tool.slash, parts.directory);
		}
		for (let path of this.tree.directoryPaths) {
			file.write("$(APP_DIR)");
			file.write(tool.slash);
			file.write(path);
			if (tool.windows) {
				file.write(":\n\tif not exist $(APP_DIR)");
				file.write(tool.slash);
				file.write(path);
				file.write("/$(NULL) mkdir $(APP_DIR)");
			}
			else
				file.write(" :\n\tmkdir -p $(APP_DIR)");
			file.write(tool.slash);
			file.write(path);
			file.write("\n");
		}
		
		for (let item of this.tree.otherPaths) {
			file.write("$(APP_DIR)");
			file.write(tool.slash);
			file.write(item.destinationPath);
			file.write(": ");
			file.write(item.sourcePath);
			if (tool.windows)
				file.write("\n\tcopy /Y ");
			else
				file.write("\n\tcp -p ");
			file.write(item.sourcePath);
			file.write(" $(APP_DIR)");
			file.write(tool.slash);
			file.write(item.destinationPath);
			file.write("\n");
		}
	}
	generateResourcesVariables(tool, file) {
		file.write("MODULES =");
		file.write(" \\\n\t");
		file.write("$(TMP_DIR)");
		file.write(tool.slash);
		file.write("FskManifest.xsb");
		for (let item of this.tree.xmlPaths) {
			file.write(" \\\n\t$(TMP_DIR)");
			file.write(tool.slash);
			file.write(item.destinationPath);
			file.write(".xsb");
		}
		for (let item of this.tree.jsPaths) {
			file.write(" \\\n\t$(TMP_DIR)");
			file.write(tool.slash);
			file.write(item.destinationPath);
			file.write(".xsb");
		}
		file.write("\n");
		file.write("FOLDERS =");
		for (let path of this.tree.directoryPaths) {
			file.write(" \\\n\t$(APP_DIR)");
			file.write(tool.slash);
			file.write(path);
		}
		file.write("\n");
		file.write("FILES =");
		for (let item of this.tree.otherPaths) {
			file.write(" \\\n\t$(APP_DIR)");
			file.write(tool.slash);
			file.write(item.destinationPath);
		}
		file.write("\n");
	}
	generateTargetRules(tool, file) {
		file.write(this.getTargetRules(tool));
	}
	generateXS(tool, tmp, bin) {
		var path = tool.joinPath({ directory: tmp, name: "FskManifest", extension: ".xs" });
		var file = new File(path);
		file.write('<!-- WARNING: This file is automatically generated by config. Do not edit. -->\n');
		file.write('<?xml version="1.0" encoding="UTF-8"?>\n');
		file.write("<package>\n");
		for (let makefile of this.makefiles) {
			if (makefile.xs) {
				file.write('\t<import href="');
				file.write(makefile.name);
				file.write('.xs"/>\n');
			}
		}
		file.write("</package>\n");
		file.close();
	}
	generateXSRules(tool, file) {
		file.line("$(TMP_DIR)", tool.slash, "FskManifest.xs.c: $(MODULES)");
		file.line("\t$(XSL) $(MODULES) -a FskManifest -b $(TMP_DIR) -o $(APP_DIR)");
		
		file.line("$(TMP_DIR)", tool.slash,"FskManifest.xsb: $(TMP_DIR)", tool.slash, "FskManifest.js");
		file.line("\t$(XSC) $(TMP_DIR)", tool.slash ,"FskManifest.js -c -d -e -p -o $(TMP_DIR)");
			
		file.line("$(TMP_DIR)", tool.slash, "FskManifest.js: $(XSC_PACKAGES) ", tool.manifestPath);
		file.line("\t$(XS2JS) $(TMP_DIR)", tool.slash, "FskManifest.xs $(XSC_OPTIONS) -p -o $(TMP_DIR)");
	}
	generateXSVariables(tool, file) {
		file.write("XSC_OPTIONS =");
		file.write(" \\\n\t-b");
		if (tool.debug)
			file.write(" \\\n\t-d");
		for (let item of this.xsIncludes) {
			file.write(" \\\n\t-i ");
			file.write(item);
		}
		if (tool.debug)
			file.write(" \\\n\t-t debug");
		file.write(" \\\n\t-t KPR_CONFIG");
		file.write(" \\\n\t-t XS6");
		for (let item of this.xsOptions) {
			file.write(" \\\n\t");
			file.write(item);
		}
		file.write("\n");
		file.write("XSC_PACKAGES =");
		for (let item of this.xsSources) {
			file.write(" \\\n\t");
			file.write(item);
		}
		file.write("\n");
	}
	getPlatformVariables(tool, tmp, bin) {
		throw new Error("unsupported platform!");
	}
	getTargetRules(tool) {
		throw new Error("unsupported platform!");
	}
};
