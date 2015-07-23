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

class Makefile extends MAKE.Makefile {
	constructor(tree) {
		super(tree);
		this.objects = "$(" + this.name + "_OBJECTS)";
	}
	generateRules(tool, file) {
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
	getPlatformVariables(tool, tmp, bin) {
		return {
			KPR_APPLICATION: tool.application,
		//	KPR_CONFIGURATION: tool.debug ? "debug" : "release",
			TOOL_PREFIX: "arm-marvell-linux-gnueabi-",

			CC: "$(TOOL_PREFIX)gcc",
			CXX: "$(TOOL_PREFIX)g++",
			AS: "$(TOOL_PREFIX)gcc",
			AS_OPTIONS: "-c -x assembler-with-cpp",
			AS_V7: "$(TOOL_PREFIX)as",
			AS_V7_OPTIONS: "",
			AS_NEON: "$(TOOL_PREFIX)as",
			AS_NEON_OPTIONS: "-mfpu=neon",
			AS_WMMX: "$(TOOL_PREFIX)as",
			AS_WMMX_OPTIONS: "-mwmmxt",
			AR: "$(TOOL_PREFIX)ar -cr",
			LINK: "$(TOOL_PREFIX)gcc",
			STRIP: "$(TOOL_PREFIX)strip",

			KPR2JS: "$(F_HOME)/xs6/bin/linux/debug/kpr2js6",
			XS2JS: "$(F_HOME)/xs6/bin/linux/debug/xs2js6",
			XSC: "$(F_HOME)/xs6/bin/linux/debug/xsc6",
			XSL: "$(F_HOME)/xs6/bin/linux/debug/xsl6",

			APP_DIR: bin,

			FREETYPE_PLATFORM_C_OPTIONS: "",
		}
	}
	getTargetRules(tool) {
		var rules = `

all: $(APP_DIR) $(APP_DIR)/Kpl $(FOLDERS) $(FILES) copy

copy: $(FOLDERS) $(FILES)
	cd $(APP_DIR)/..; tar czf $(KPR_APPLICATION).tgz $(KPR_APPLICATION)

$(APP_DIR):
	mkdir -p $(APP_DIR)

$(APP_DIR)/Kpl: FREETYPE $(TMP_DIR)/KplAudioLinuxALSA.o $(OBJECTS)
	$(LINK) -Wl,-Map,$(TMP_DIR)/Kpl.map -o $(APP_DIR)/Kpl $(FREETYPE_OBJECTS) $(TMP_DIR)/KplAudioLinuxALSA.o $(OBJECTS) $(LIBRARIES)
ifeq "$(KPR_CONFIGURATION)" "release"
	$(STRIP) $(APP_DIR)/Kpl
endif

$(TMP_DIR)/KplAudioLinuxALSA.o: $(HEADERS) $(F_HOME)/build/linux/kpl/KplAudioLinuxALSA.c
	$(CC) $(F_HOME)/build/linux/kpl/KplAudioLinuxALSA.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@

`;
		var source = tool.homePath + "/libraries/freetype/freetype.make";
		var buffer = FS.readFileSync(source).toString();
		
		return rules + "\n" + buffer + "\n";
	}
};

export default Manifest;
