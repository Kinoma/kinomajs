<?xml version="1.0" encoding="UTF-8"?>
<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<package>
	<patch prototype="Crypt">
		<object name="wsu">
			<object name="date" prototype="Date.prototype">
				<function name="toString">
					// format utc time as "YYYY-MM-DDTHH:MM:SS.mmmmZ"
					return(this.getUTCFullYear() + "-" + (this.getUTCMonth()+1) + "-" + this.getUTCDate() + "T" + this.getUTCHours() + ":" + this.getUTCMinutes() + ":" + this.getUTCSeconds() + "." + this.getUTCMilliseconds() + "Z");
				</function>

				<function name="parse" params="text">
					if (!text)
						return;
					var a = text.split(/([0-9]*)-([0-9]*)-([0-9]*)T([0-9]*):([0-9]*):([0-9]*)\.([0-9]*)Z/);
					if (a.length < 9)
						return;
					return new Crypt.wsu.Date(parseInt(a[1]), parseInt(a[2]) - 1, parseInt(a[3]), parseInt(a[4]), parseInt(a[5]), parseInt(a[6]), parseInt(a[7]));
				</function>

				<function name="serialize" params="o">
					if (o && o instanceof Crypt.wsu.Date)
						return o.toString();
				</function>
			</object>
			<function name="Date" prototype="Crypt.wsu.date">
				Date.apply(this, arguments);
			</function>
		</object>
	</patch>
</package>