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
			LINUXSDK: "$(BG3CDP_TOP)/linuxsdk",
			BG3CDP_OPENGL: "$(LINUXSDK)/vendor/marvell/generic/frameworks/gpu/sdk",
			BG3CDP_SYSROOT_LIB: "$(LINUXSDK)/vendor/marvell-sdk/MV88DE3100_SDK/Customization_Data/File_Systems/rootfs_linux",

			KPR_APPLICATION: tool.application,
			KPR_CONFIGURATION: tool.debug ? "debug" : "release",
			TOOL_PREFIX: "$(BG3CDP_GNUEABI)/bin/arm-linux-gnueabihf-",

			CC: "$(TOOL_PREFIX)gcc",
			CXX: "$(TOOL_PREFIX)gcc",
			AS: "$(TOOL_PREFIX)gcc",
			AS_OPTIONS: "-c -x assembler-with-cpp",
			AS_V7: "$(TOOL_PREFIX)gcc",
			AS_V7_OPTIONS: "-c -x assembler-with-cpp -ftree-vectorize -mfpu=neon",
			AS_NEON: "$(TOOL_PREFIX)gcc",
			AS_NEON_OPTIONS: "-c -x assembler-with-cpp -ftree-vectorize -mfpu=neon",
			AS_WMMX: "$(TOOL_PREFIX)as",
			AS_WMMX_OPTIONS: "",
			AR: "$(TOOL_PREFIX)ar -cr",
			LINK: "$(TOOL_PREFIX)gcc",
			STRIP: "$(TOOL_PREFIX)strip",

			KPR2JS: "$(F_HOME)/xs6/bin/linux/debug/kpr2js6",
			XS2JS: "$(F_HOME)/xs6/bin/linux/debug/xs2js6",
			XSC: "$(F_HOME)/xs6/bin/linux/debug/xsc6",
			XSL: "$(F_HOME)/xs6/bin/linux/debug/xsl6",

			APP_DIR: bin,
			RES_DIR: bin,
			TMP_DIR: tmp,

			FREETYPE_DIR: "$(TMP_DIR)/freetype-$(FREETYPE_VERSION)",
		}
	}
	getTargetRules(tool) {
		return `
all: $(APP_DIR) $(APP_DIR)/Kpl $(FOLDERS) $(FILES) copy

copy: $(FOLDERS) $(FILES)
	cd $(APP_DIR)/..; tar czf $(KPR_APPLICATION).tgz $(KPR_APPLICATION)

$(APP_DIR):
	mkdir -p $(APP_DIR)

$(APP_DIR)/Kpl: $(FREETYPE_DIR) $(TMP_DIR)/KplAudioLinuxBG3.o $(OBJECTS)
	$(LINK) -Wl,-Map,$(TMP_DIR)/Kpl.map -o $(APP_DIR)/Kpl -Wl,--whole-archive $(FREETYPE_OBJECTS) -Wl,--no-whole-archive $(MORE_LIBRARIES) $(TMP_DIR)/KplAudioLinuxBG3.o $(OBJECTS) $(LIBRARIES)
	cp $(F_HOME)/libraries/libdns_sd.so $(APP_DIR)/.
ifeq "$(KPR_CONFIGURATION)" "release"
	$(STRIP) $(APP_DIR)/Kpl
endif

MORE_LIBRARIES = \\
	-L$(F_HOME)/libraries 	\\
	-L$(BG3CDP_SYSROOT_LIB)/usr/lib 	\\
	-L$(BG3CDP_SYSROOT_LIB)/vendor/lib 	\\
	-Wl,-rpath=.	\\
	-Wl,-rpath=/vendor/lib	\\
	-Wl,-rpath-link,$(BG3CDP_SYSROOT_LIB)/usr/lib 	\\
	-Wl,-rpath-link,$(BG3CDP_SYSROOT_LIB)/vendor/lib 	\\
	-Wl,-z,muldefs

$(FREETYPE_DIR):
	cd $(TMP_DIR) ; tar -jxvf $(F_HOME)/libraries/freetype/freetype-$(FREETYPE_VERSION).tar.bz2
	cd $(FREETYPE_DIR); mkdir obj
	cd $(FREETYPE_DIR)/obj; cmake -DCMAKE_TOOLCHAIN_FILE=$(F_HOME)/kinoma/kpr/cmake/linux/bg3cdp/toolchain.cmake ..
	cmake --build $(FREETYPE_DIR)/obj

FREETYPE_OBJECTS = $(FREETYPE_DIR)/obj/libfreetype.a

$(TMP_DIR)/KplAudioLinuxBG3.o: $(HEADERS) $(F_HOME)/build/linux/kpl/KplAudioLinuxBG3.c
	$(CC) $(F_HOME)/build/linux/kpl/KplAudioLinuxBG3.c $(C_OPTIONS) $(C_INCLUDES) -c -o $@

`;
	}
};

export default Manifest;
