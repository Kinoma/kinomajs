export default class @ "KPR_debug" {
	constructor(dictionary) @ "KPR_Debug";
	get behavior() @ "KPR_debug_get_behavior"
	set behavior(it) @ "KPR_debug_set_behavior"
	get machines() @ "KPR_debug_get_machines"
	abort() @ "KPR_debug_abort"
	addBreakpoint(address, path, line) @ "KPR_debug_addBreakpoint"
	addBreakpoints(address, breakpoints) @ "KPR_debug_addBreakpoints"
	close() @ "KPR_debug_close"
	file(address, view, path, line) @ "KPR_debug_file"
	go() @ "KPR_debug_go"
	logout() @ "KPR_debug_logout"
	removeBreakpoint(address, path, line) @ "KPR_debug_removeBreakpoint"
	resetBreakpoints(address) @ "KPR_debug_resetBreakpoints"
	step() @ "KPR_debug_step"
	stepIn() @ "KPR_debug_stepIn"
	stepOut() @ "KPR_debug_stepOut"
	toggle(address, view, value) @ "KPR_debug_toggle"
};
