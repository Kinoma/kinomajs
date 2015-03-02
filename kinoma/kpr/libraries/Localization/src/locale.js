//@module
/*
  Copyright 2011-2014 Marvell Semiconductor, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/
var locales = Object.create(Object.prototype, {
	fr: { configurable: true, enumerable: true, writable: true, value: Object.create(Object.prototype, {
		formatDate: { configurable: true, enumerable: true, writable: true, value: 
			function(kind, year, month, date, day) {
				var result = "";
				if (!kind) {
					if (date < 10) result += "0";
					result += date + "/";
					if (month < 10) result += "0";
					result += month + "/"; 
					result += year.toString().substring(2);
				}
				else {
					if (kind > 3)
						result += getDayName(day) + " ";
					else if (kind > 2)
						result += getDayAbbreviation(day) + " ";
					result += date + " ";
					if (kind > 1)
						result += getMonthName(month) + " ";
					else
						result += getMonthAbbreviation(month) + " ";
					result += year;
				}
				return result;
			}
		},
		formatTime: { configurable: true, enumerable: true, writable: true, value: 
			function(kind, hour, minute, second) {
				var result = "";
				if (hour < 10) result += "0";
				result += hour;
				result += ":";
				if (minute < 10) result += "0";
				result += minute;
				if (kind) {
					result += ":";
					if (second < 10) result += "0";
					result += second;
				}
				return result;
			}
		},
	})},
	jp: { configurable: true, enumerable: true, writable: true, value: Object.create(Object.prototype, {
		formatDate: { configurable: true, enumerable: true, writable: true, value: 
			function(kind, year, month, date, day) {
				var result = "";
				if (!kind) {
					result += year.toString().substring(2) + "/";
					if (month < 10) result += "0";
					result += month + "/";
					if (date < 10) result += "0";
					result += date;
				}
				else {
					result += year + "年";
					result += month + "月";
					result += date + "日";
					if (kind > 2)
						result += getDayName(day);
				}
				return result;
			}
		},
		formatTime: { configurable: true, enumerable: true, writable: true, value: 
			function(kind, hour, minute, second) {
				var result = "";
				result += hour;
				result += ":";
				if (minute < 10) result += "0";
				result += minute;
				if (kind) {
					result += ":";
					if (second < 10) result += "0";
					result += second;
				}
				return result;
			}
		},
	})},
	us: { configurable: true, enumerable: true, writable: true, value: Object.create(Object.prototype, {
		formatDate: { configurable: true, enumerable: true, writable: true, value: 
			function(kind, year, month, date, day) {
				var result = "";
				if (!kind) {
					result += month + "/"; 
					result += date + "/";
					result += year.toString().substring(2);
				}
				else {
					if (kind > 3)
						result += getDayName(day) + ", ";
					else if (kind > 2)
						result += getDayAbbreviation(day) + ", ";
					if (kind > 1)
						result += getMonthName(month) + " ";
					else
						result += getMonthAbbreviation(month) + " ";
					result += date + ", ";
					result += year;
				}
				return result;
			}
		},
		formatTime: { configurable: true, enumerable: true, writable: true, value: 
			function(kind, hour, minute, second) {
				var result = "";
				if (0 == hour)
					result = "12:"
				else if (hour <= 12)
					result = hour + ":"
				else
					result = (hour - 12) + ":"
				if (minute < 10) result += "0"
				result += minute
				if (kind) {
					result += ":";
					if (second < 10) result += "0";
					result += second;
				}
				result += (hour >= 12) ? " PM" : " AM"
				return result;
			}
		},
	})},
});
var formatAbsoluteDate = function(d, kind, utc) {
	var year, month, date, day;
	if (utc) {
		year = d.getUTCFullYear();
		month = 1 + d.getUTCMonth();
		date = d.getUTCDate();
		if (kind > 2) day = d.getUTCDay();
	}
	else {
		year = d.getFullYear();
		month = 1 + d.getMonth();
		date = d.getDate();
		if (kind > 2) day = d.getDay();
	}
	var locale = "us"; // this is a placeholder. logic for choosing format should be based on locale, not language. 
	if (locale in locales)
		return locales[locale].formatDate(kind, year, month, date, day);
	return locales.us.formatDate(kind, year, month, date, day);
}
var formatAbsoluteTime = function(d, kind, utc) {
	var hour, minute, second;
	if (utc) {
		hour = d.getUTCHours();
		minute = d.getUTCMinutes();
		if (kind) second = d.getUTCSeconds();
	} 
	else {
		hour = d.getHours();
		minute = d.getMinutes();
		if (kind) second = d.getSeconds();
	}
	var locale = "us"; // this is a placeholder. logic for choosing format should be based on locale, not language. 
	if (locale in locales)
		return locales[locale].formatTime(kind, hour, minute, second);
	return locales.us.formatTime(kind, hour, minute, second);
}
var formatRelativeDate = function(d, today, utc) {
	var v = d.valueOf()
	if (v >= today) {
		if (v < (today + 86400000))
			return "Today".toLocaleString();
		if (v < (today + 86400000 + 86400000))
			return "Tomorrow".toLocaleString();
		if (v < (today + (86400000 * 6)))
			return getDayName(utc ? d.getUTCDay() : d.getDay());
	}
	else {
		if (v >= (today - 86400000))
			return "Yesterday".toLocaleString();
		// past days?
	}
}
var formatRelativeTime = function(d, now, utc) {
	var v = d.valueOf()
	var delta = Math.round((now - v) / 1000);
	if (delta <= 0)
		return "Now".toLocaleString();
	if (delta < 60)
		return ((1 == delta) ? "$ second ago".toLocaleString() : "$ seconds ago".toLocaleString()).replace("$", delta);
	if (delta < ((59 * 60) + 30)) {
		delta = Math.round(delta / 60);
		if (delta < 60)
			return ((1 == delta) ? "$ minute ago".toLocaleString() : "$ minutes ago".toLocaleString()).replace("$", delta);
	}
	if (delta < ((23 * 60 * 60) + (30 * 60))) {
		delta = Math.round(delta / (60 * 60));
		return ((1 == delta) ? "$ hour ago".toLocaleString() : "$ hours ago".toLocaleString()).replace("$", delta);
	}
}
var formatDate = function(d, today, kind, utc) {
	if (today != undefined) {
		today = formatRelativeDate(d, today, utc);
		if (today)
			return today;
	}
	return formatAbsoluteDate(d, kind, utc);
}
var formatTime = function(d, now, kind, utc) {
	if (now != undefined) {
		now = formatRelativeTime(d, now);
		if (now)
			return now;
	}
	return formatAbsoluteTime(d, kind, utc);
}
var formatDateTime = function(d, today, dateKind, now, timeKind, utc) {
	if (now != undefined) {
		now = formatRelativeTime(d, now);
		if (now)
			return now;
	}
	if (timeKind < 0)
		return formatDate(d, today, dateKind, utc);
	return formatDate(d, today, dateKind, utc) + " " + formatAbsoluteTime(d, timeKind, utc);
}
var formatTimeDate = function(d, now, timeKind, today, dateKind, utc) {
	if (now != undefined) {
		now = formatRelativeTime(d, now);
		if (now)
			return now;
	}
	return formatAbsoluteTime(d, timeKind, utc) + " " + formatDate(d, today, dateKind, utc);
}
var getDayAbbreviation = function(day) {
	var result;
	switch (day) {
	case 0: result = "Sun".toLocaleString(); break;
	case 1: result = "Mon".toLocaleString(); break;
	case 2: result = "Tue".toLocaleString(); break;
	case 3: result = "Wed".toLocaleString(); break;
	case 4: result = "Thu".toLocaleString(); break;
	case 5: result = "Fri".toLocaleString(); break;
	case 6: result = "Sat".toLocaleString(); break;
	}
	return result;
}
var getDayName = function(day) {
	var result;
	switch (day) {
	case 0: result = "Sunday".toLocaleString(); break;
	case 1: result = "Monday".toLocaleString(); break;
	case 2: result = "Tuesday".toLocaleString(); break;
	case 3: result = "Wednesday".toLocaleString(); break;
	case 4: result = "Thursday".toLocaleString(); break;
	case 5: result = "Friday".toLocaleString(); break;
	case 6: result = "Saturday".toLocaleString(); break;
	}
	return result;
}
var getMonthAbbreviation = function(month) {
	var result;
	switch (month) {
	case 1: result = "JAN".toLocaleString(); break;
	case 2: result = "FEB".toLocaleString(); break;
	case 3: result = "MAR".toLocaleString(); break;
	case 4: result = "APR".toLocaleString(); break;
	case 5: result = "MAY".toLocaleString(); break;
	case 6: result = "JUN".toLocaleString(); break;
	case 7: result = "JUL".toLocaleString(); break;
	case 8: result = "AUG".toLocaleString(); break;
	case 9: result = "SEP".toLocaleString(); break;
	case 10: result = "OCT".toLocaleString(); break;
	case 11: result = "NOV".toLocaleString(); break;
	case 12: result = "DEC".toLocaleString(); break;
	}
	return result;
}
var getMonthName = function(month) {
	var result;
	switch (month) {
	case 1: result = "January".toLocaleString(); break;
	case 2: result = "February".toLocaleString(); break;
	case 3: result = "March".toLocaleString(); break;
	case 4: result = "April".toLocaleString(); break;
	case 5: result = "May".toLocaleString(); break;
	case 6: result = "June".toLocaleString(); break;
	case 7: result = "July".toLocaleString(); break;
	case 8: result = "August".toLocaleString(); break;
	case 9: result = "September".toLocaleString(); break;
	case 10: result = "October".toLocaleString(); break;
	case 11: result = "November".toLocaleString(); break;
	case 12: result = "December".toLocaleString(); break;
	}
	return result;
}
var formatBytes = function(size) {
	var units;

	if (size < 512)
		return "$ bytes".toLocaleString().replace("$", size);
	size /= 1024;
	units = "KB";
	if (size > 1024) {
		size /= 1024;
		units = "MB";
		if (size > 1024) {
			size /= 1024;
			units = "GB";
		}
	}
	return size.toFixed(1) + " " + units;
}
var formatNumber = function(number) {
	// rough placeholder. right now just handles inserting commas into integers using US convention
	var number = number.toString(), value = [];
	if (number.length <= 4) return number;
	while (number.length >= 3) {
		value.push(number.substring(number.length - 3))
		number = number.substring(0, number.length - 3)
	}
	if (number)
		value.push(number)
	value.reverse()
	return value.join(",")
}
var today = function() {
	var d = new Date;
	d.setHours(0, 0, 0, 0);
	return d.valueOf();
}
exports.formatBytes = formatBytes;
exports.formatDate = formatDate;
exports.formatDateTime = formatDateTime;
exports.formatNumber = formatNumber;
exports.formatTime = formatTime;
exports.formatTimeDate = formatTimeDate;
exports.getDayAbbreviation = getDayAbbreviation;
exports.getDayName = getDayName;
exports.getMonthAbbreviation = getMonthAbbreviation;
exports.getMonthName = getMonthName;
exports.today = today;

