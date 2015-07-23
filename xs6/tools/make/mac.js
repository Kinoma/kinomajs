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

class Manifest extends MAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var icns = parts.directory + "/mac/fsk.icns";
		var nib = parts.directory + "/mac/fsk.nib";
		var plist = parts.directory + "/mac/fsk.plist";
		var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release"
		return {
			AR: "libtool -static -o",
			KPR2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa kpr2js",
			XS2JS: path + "/xsr6 -a " +  path + "/modules/tools.xsa xs2js",
			XSC: path + "/xsc6",
			XSL: path + "/xsl6",
			
			APP_DIR: bin + ".app/Contents/MacOS",
			TMP_DIR: tmp,
			
			ICNS: FS.existsSync(icns) ? icns : "$(F_HOME)/build/mac/fsk.icns",
			NIB: FS.existsSync(nib) ? nib : "$(F_HOME)/build/mac/fsk.nib",
			PLIST: FS.existsSync(plist) ? plist : "$(F_HOME)/build/mac/fsk.plist",
			// FskPlatform.mk
			SDKVER: "10.8",
		};
	}
	getTargetRules(tool) {
		return `
all: $(APP_DIR) $(APP_DIR)/fsk $(FOLDERS) $(FILES) 

$(APP_DIR): $(ICNS) $(NIB) $(PLIST)
	mkdir -p $(APP_DIR)
	mkdir -p $(APP_DIR)/../Resources/English.lproj
	cp -rf $(ICNS) $(APP_DIR)/../Resources
	cp -rf $(NIB) $(APP_DIR)/../Resources/English.lproj
	cp -rf $(PLIST) $(APP_DIR)/../Info.plist
	echo APPLTINY > $(APP_DIR)/../PkgInfo

$(APP_DIR)/fsk: $(TMP_DIR)/main.o $(OBJECTS)
	$(CC) -mmacosx-version-min=$(SDKVER) -arch i386 -ObjC $(TMP_DIR)/main.o $(OBJECTS) $(LIBRARIES) -o $(APP_DIR)/fsk

$(TMP_DIR)/main.o: $(HEADERS) $(F_HOME)/kinoma/kpr/patches/main.m
	$(CC) $(F_HOME)/kinoma/kpr/patches/main.m $(C_OPTIONS) $(C_INCLUDES) -c -o $@

`;
	}
};

export default Manifest;
