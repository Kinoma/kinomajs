import { 
	model,
} from "shell/main";

import {
	tabBreakpointSkin,
	tabBreakpointStyle,
	tabBrokenSkin,
	tabSkin,
	tabStyle,
	tabsPaneSkin,
	buttonsSkin,
} from "shell/assets";

import {
	ButtonBehavior
} from "common/control";

import {
	ScrollerBehavior,
} from "common/scrollbar";

class TabsPaneBehavior extends Behavior {
	onCreate(layout, data) {
		this.data = data;
		this.onMachinesChanged(layout, data.machines);
	}
	onMachinesChanged(layout, machines) {
		let scroller = layout.first;
		let line = scroller.first;
		line.empty(0);
		line.add(new BreakpointsTab(null));
		machines.forEach(machine => line.add(new Tab(machine)));
		this.onMeasureHorizontally(layout);
	}
	onMeasureHorizontally(layout, width) {
		let scroller = layout.first;
		let line = scroller.first;
		let sum = this.data.machines.reduce((sum, machine) => sum + tabStyle.measure(machine.title).width, 62);
		line.width = Math.max(sum, shell.width);
		return width;
	}
};

class BreakpointsTabBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
		var content = container.first.first;
		while (content) {
			content.state = state;
			content = content.next;
		}
	}
	onCreate(container, machine) {
		this.machine = machine;
		this.onMachineSelected(container, model.currentMachine);
	}
	onMachineChanged(container, machine) {
		if (this.machine == machine) {
			container.first.first.visible = machine.broken;
		}
	}
	onMachineSelected(container, machine) {
		this.changeState(container, this.machine == machine ? 0 : 1);
	}
	onTouchBegan(container) {
		this.changeState(container, 0);
		model.selectMachine(this.machine);
	}
	onMouseEntered(container, x, y) {
		shell.behavior.cursorShape = system.cursors.arrow;
		this.changeState(container, this.machine != model.currentMachine ? 2 : 0);
	}
	onMouseExited(container, x, y) {
		this.changeState(container, this.machine != model.currentMachine ? 1 : 0);
	}
};

class TabBehavior extends BreakpointsTabBehavior {
	onMouseEntered(container, x, y) {
		container.last.visible = this.machine.closable;
		super.onMouseEntered(container, x, y);
	}
	onMouseExited(container, x, y) {
		container.last.visible = false;
		super.onMouseExited(container, x, y);
	}
};

class CloseButtonBehavior extends ButtonBehavior {
	onTap(content) {
		model.debug.closeMachine(this.data.address);
	}
}

export var TabsPane = Layout.template($ => ({
	left:0, right:0, top:0, height:27, skin:tabsPaneSkin, Behavior:TabsPaneBehavior,
	contents: [
		Scroller($, {
			left:0, right:0, top:0, bottom:1, clip:true, active:true, Behavior:ScrollerBehavior, 
			contents: [
				Line($, {
					left:0, width:0, top:0, bottom:0, 
					contents: [
					]
				}),
			]
		}),
	]
}));

var Tab = Container.template($ => ({
	top:0, bottom:0, skin:tabSkin, active:true, Behavior:TabBehavior,
	contents: [
		Container($, { 
			top:0, bottom:0,
			contents: [
				Content($, { left:3, width:20, visible:$.broken, skin:tabBrokenSkin, }),
				Label($, { top:0, bottom:0, style:tabStyle, string:$.title }),
			],
		}),
		Content($, { left:0, width:26, active:true, visible:false, skin:buttonsSkin, variant:6, state:1, Behavior:CloseButtonBehavior }),
	],
}));

var BreakpointsTab = Container.template($ => ({
	width:52, top:0, bottom:0, skin:tabSkin, active:true, Behavior:BreakpointsTabBehavior,
	contents: [
		Label($, { 
			left:5, right:5, height:16, skin:tabBreakpointSkin, style:tabBreakpointStyle,
			Behavior: class extends Behavior {
				onCreate(label) {
					this.onBreakpointsChanged(label);
				}
				onBreakpointsChanged(label) {
					label.string = model.breakpoints.items.length;
				}
			},
		}),
	],
}));
