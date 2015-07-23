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
export default class Tool {
	constructor(argv) {
		this.errorCount = 0;
		this.warningCount = 0;
	}
	get currentDirectory() @ "Tool_prototype_get_currentDirectory";
	set currentDirectory(it) @ "Tool_prototype_set_currentDirectory";
	get currentPlatform() @ "Tool_prototype_get_currentPlatform";
	execute(command) @ "Tool_prototype_execute";
	joinPath(parts) @ "Tool_prototype_joinPath";
	report(message) @ "Tool_prototype_report";
	reportError(message) @ "Tool_prototype_reportError";
	reportWarning(message) @ "Tool_prototype_reportWarning";
	resolveDirectoryPath(message) @ "Tool_prototype_resolveDirectoryPath";
	resolveFilePath(message) @ "Tool_prototype_resolveFilePath";
	resolvePath(message) @ "Tool_prototype_resolvePath";
	splitPath(path) @ "Tool_prototype_splitPath";
}
