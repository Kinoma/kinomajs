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
import * as CMAKE from "cmake";

class Manifest extends CMAKE.Manifest {
	constructor(tree) {
		super(tree);
	}
	getPlatformVariables(tool, tmp, bin) {
		var parts = tool.splitPath(tool.manifestPath);
		var icns = parts.directory + "/mac/fsk.icns";
		var nib = parts.directory + "/mac/fsk.nib";
		var plist = parts.directory + "/mac/fsk.plist"; var path = process.debug ? "$(F_HOME)/xs6/bin/mac/debug" : "$(F_HOME)/xs6/bin/mac/release"
		return {
			AR: "libtool -static -o",
			KPR2JS: path + "/xsr6 -a " + path + "/modules/tools.xsa kpr2js",
			XS2JS: path + "/xsr6 -a " +  path + "/modules/tools.xsa xs2js",
			XSC: path + "/xsc6",
			XSL: path + "/xsl6",
			CMAKE_OSX_ARCHITECTURES: "i386",
			
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
		return `if (CMAKE_GENERATOR STREQUAL "Xcode")
	set(CMAKE_XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR \${APP_DIR})
else ()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY \${APP_DIR})
endif ()
add_executable(fsk \${Fsk_SOURCES} \${F_HOME}/kinoma/kpr/patches/main.m)
target_link_libraries(fsk \${LIBRARIES} -ObjC)
target_include_directories(fsk PUBLIC \${C_INCLUDES})
target_compile_definitions(fsk PUBLIC \${C_DEFINITIONS})
target_compile_options(fsk PUBLIC \${C_OPTIONS})

COPY(SOURCE \${ICNS} DESTINATION \${APP_DIR}/../Resources/fsk.icns)
COPY(SOURCE \${NIB} DESTINATION \${APP_DIR}/../Resources/English.lproj/fsk.nib)
COPY(SOURCE \${PLIST} DESTINATION \${APP_DIR}/../Info.plist)
file(WRITE \${APP_DIR}/../PkgInfo "APPLTINY")
`;
	}
}

export default Manifest;

