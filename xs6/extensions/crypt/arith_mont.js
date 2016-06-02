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
import Arith from "arith";

export default class Mont extends Arith.Module {
	constructor(dict) {
		super(dict.z, dict.m);
		var method = dict.method === undefined ? this.LR : dict.method;
		this._mont_init(method, dict.sw_param);
	}
	_mont_init(method, options) @ "xs_mont_init";
	// any better way??
	get LR() { return 0; };
	get SW() { return 1; };
};
