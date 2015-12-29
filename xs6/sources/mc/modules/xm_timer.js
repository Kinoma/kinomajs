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
import TimeInterval from "timeinterval";
import Launcher from "launcher";

export function setInterval(cb, t)
{
	var o = new TimeInterval(cb, t);
	o.start();
	Launcher.add(o, function(o) {
		o.close();
	});
	return o;
}

export function clearInterval(o)
{
	o.close();
	Launcher.remove(o);
}

export function setTimeout(cb, t)
{
	return setInterval(function() {
		clearInterval(this);
		cb();
	}, t);
}

export function clearTimeout(o)
{
	o.close();
	Launcher.remove(o);
}
