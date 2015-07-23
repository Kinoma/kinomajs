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
var url = mergeURI(shell.url, "./main");
var host = new Host({left:0, top:0, right:0, bottom:0}, url, "xsbug.kinoma.com", true);
shell.add(host);
shell.behavior = {
	onInvoke: function(shell, message) {
		if (message.name == "quit")
			shell.quit(); 
	},
	onQuit: function(shell) {
		host.quit();
	},
};
host.adapt();
host.launch();
