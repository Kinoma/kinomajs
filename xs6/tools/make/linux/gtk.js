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

			KPR2JS: "$(F_HOME)/xs6/bin/linux/debug/kpr2js6",
			XS2JS: "$(F_HOME)/xs6/bin/linux/debug/xs2js6",
			XSC: "$(F_HOME)/xs6/bin/linux/debug/xsc6",
			XSL: "$(F_HOME)/xs6/bin/linux/debug/xsl6",

			APP_DIR: bin,
		}
	}
	getTargetRules(tool) {
		return `
FskPlatform_C_OPTIONS += $(shell pkg-config --cflags glib-2.0)
FskPlatform_C_OPTIONS += $(shell pkg-config --cflags gtk+-3.0)
LIBRARIES += $(shell pkg-config --libs glib-2.0)
LIBRARIES += $(shell pkg-config --libs gtk+-3.0)

all: $(APP_DIR) $(APP_DIR)/Kpl $(FOLDERS) $(FILES)

$(APP_DIR):
	mkdir -p $(APP_DIR)

$(APP_DIR)/Kpl: $(TMP_DIR)/KplAudioLinuxALSA.o $(OBJECTS)
	$(CXX) -o $(APP_DIR)/Kpl $(TMP_DIR)/KplAudioLinuxALSA.o $(OBJECTS) $(LIBRARIES)
ifeq "$(KPR_CONFIGURATION)" "release"
	$(STRIP) $(APP_DIR)/Kpl
endif

$(TMP_DIR)/KplAudioLinuxALSA.o: $(HEADERS) $(F_HOME)/build/linux/kpl/KplAudioLinuxALSA.c
	$(CC) $(F_HOME)/build/linux/kpl/KplAudioLinuxALSA.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@

`;
	}
};

export default Manifest;
