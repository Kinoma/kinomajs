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
export default class TimeInterval @ "xs_timeInterval_destructor" {
	constructor(f, t) @ "xs_timeInterval_constructor";
	start() @ "xs_timeInterval_start";
	stop() @ "xs_timeInterval_stop";
	reschedule(t) @ "xs_timeInterval_reschedule";
	get interval() @ "xs_timeInterval_getInterval";
	close() @ "xs_timeInterval_close";
};
