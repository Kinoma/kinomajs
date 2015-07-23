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
export default {
	reportError(path, line, message) {
		console.log(path + ":" + line + ":", message);
	},
	scan(buffer) @ "infoset_scan",
	document: {
		children: undefined,
		kind: 0,
		element: undefined,
		encoding: undefined,
		version: undefined,
	},
	element: {
		attributes: undefined,
		children: undefined,
		kind: 1,
		name: undefined,
		namespace: undefined,
		parent: undefined,
		prefix: undefined,
		xmlnsAttributes: undefined,
	},
	attribute: {
		kind: 2,
		name: undefined,
		namespace: undefined,
		parent: undefined,
		prefix: undefined,
		value: undefined,
	},
	cdata: {
		kind: 3,
		parent: undefined,
		value: undefined,
	},
	comment: {
		kind: 4,
		parent: undefined,
		value: undefined,
	},
	pi: {
		kind: 5,
		name: undefined,
		namespace: undefined,
		parent: undefined,
		prefix: undefined,
		value: undefined,
	},
};
