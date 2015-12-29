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
class _Iterator @ "xs_env_iter_destructor" {
	constructor(self) @ "xs_env_Iterator";
	next(pattern) @ "xs_env_next";
};

export default class Environment @ "xs_env_destructor" {
	constructor(path, autosave, encrypt) @ "xs_env_constructor";
	"get"(name) @ "xs_env_get";
	"set"(name, val) @ "xs_env_set";
	save() @ "xs_env_save";
	unset(name) @ "xs_env_unset";
	clear() @ "xs_env_clear";
	close() @ "xs_env_close";
	getIterator() {
		return new _Iterator(this);
	}
	dump(flags) @ "xs_env_dump";
	static init() @ "xs_env_init";
};

Environment.init();
