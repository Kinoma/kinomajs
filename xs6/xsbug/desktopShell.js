//@program
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
