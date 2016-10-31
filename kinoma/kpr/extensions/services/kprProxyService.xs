<?xml version="1.0" encoding="utf-8"?>
<package script="true"><import href="kpr.xs" link="dynamic"/><program>
	
class ProxyService @ "ProxyService_destructor" {
	constructor(from) @ "ProxyService_constructor";
	get(target, key) {
		let url = this.url + encodeURIComponent(key);
		return function(...params) {
			return new Promise(function(resolve, reject) {
				target(url, params, resolve, reject)
			});
		}
	}
};
	
function clearTimeout(it) @ "KPR_clearTimeout";
function setTimeout(callback, milliseconds) @ "KPR_setTimeout";
	
</program></package>
