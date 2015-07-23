//@program
/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
shell.menus = [
	{ 
		title: "Debug",
		items: [
			{ title: "Kill", key: "K", command: "Kill" },
			null,
			{ title: "Run", key: "R", command: "Run" },
			{ title: "Step", key: "S", command: "Step" },
			{ title: "Step In", key: "I", command: "StepIn" },
			{ title: "Step Out", key: "O", command: "StepOut" },
			null,
			{ title: "Set Breakpoint", key: "B", command: "SetBreakpoint" },
			{ title: "Clear Breakpoint", key: "Shift+B", command: "ClearBreakpoint" },
			{ title: "Clear All Breakpoints", key: "Alt+Shift+B", command: "ClearAllBreakpoints" },
		],
	},
	{ 
		title: "Edit",
		items: [
			{ title: "Undo", key: "Z", command: "Undo" },
			null,
			{ title: "Cut", key: "X", command: "Cut" },
			{ title: "Copy", key: "C", command: "Copy" },
			{ title: "Paste", key: "V", command: "Paste" },
			null,
			{ title: "Find", key: "F", command: "Find" },
			{ title: "Find Next", key: "G", command: "FindNext" },
			{ title: "Find Previous", key: "Shift+G", command: "FindPrevious" },
		],
	},
	{ 
		title: "Machine",
		items: [
		],
	},
	{
		title: "View",
		items: [
			{ title: "Calls", key: "Alt+C", command: "Calls" },
			{ title: "Globals", key: "Alt+G", command: "Globals" },
			{ title: "Modules", key: "Alt+M", command: "Grammars" },
			{ title: "Files", key: "Alt+F", command: "Files" },
			{ title: "Breakpoints", key: "Alt+B", command: "Breakpoints" },
			{ title: "Kinoma", key: "Alt+K", command: "Kinoma" },
		],
	}
];

include("./xsbug");
