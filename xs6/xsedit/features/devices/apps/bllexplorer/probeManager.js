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
import Pins from "pins";

import {
	BLACK,
	WHITE,
} from "shell/assets";

import {
	blackSkin,
	blackStyle,
	greenSkin,
	whiteSkin,
} from "features/devices/assets";

import {
	SampleGraphProbeContainer,
	SampleGraphContainer,
} from "features/devices/graphic";

var probeLabelStyle = new Style({size:34, color:BLACK, left:10, right:10 });
var inOutLabelStyle = new Style({size:28, color:BLACK, left:20, right:20 });

var Separator = Content.template($ => ({ left:0, right:0, bottom:0, height:1, skin:blackSkin }));
var ProbeLabel = Label.template($ => ({ left:10, top:0, style:probeLabelStyle, string:$.name }));
var InputLabel = Label.template($ => ({ left:10, top:0, style:inOutLabelStyle, string:$.input.result.name }));
var OutputLabel = Label.template($ => ({ left:10, top:0, style:inOutLabelStyle, string:$.output.params.name }));

export class ProbeBehavior extends Behavior {
	static createProbe(pins, name, probe) {
		if (!probe) return;
		let inputs = [];
		let outputs = [];
		let metadata = probe.metadata;
		if (!metadata) return;
		if ("sources" in metadata) {
			for (let input of metadata.sources) {
				let path = "/" + name + "/" + input.name;
				trace("input: " + path + "\n");
				input.path = path;
				inputs.push(input);
			}
		}
		if ("sinks" in metadata) {
			for (let output of metadata.sinks) {
				let path = "/" + name + "/" + output.name;
				trace("output: " + path + "\n");
				output.path = path;
				outputs.push(output);
			}
		}
		if (!inputs.length && !outputs.length) return;
		return new ProbeContainer({ name, pins, probe, inputs, outputs });
	}
	static createInput(pins, input) {
		let type = input.result.type;
		let template = (type in gProbeInputTypes) ? gProbeInputTypes[type] : DefaultProbeInput;
		let container = input.container = new template({ pins, input });
		return container
	}
	static createOutput(pins, output) {
		return new DefaultProbeOutput({ pins, output });
	}
	onCreate(container, $) {
		this.data = $;
	}
	onDisplayed(container) {
 		this.data.inputs.forEach(input => this.startInput(container, input));
	}
	onUndisplayed(container) {
 		this.data.inputs.forEach(input => this.stopInput(container, input));
	}
	readInput(input, value) {
		if ("container" in input)
			input.container.graph.delegate("onUpdate", value);
	}
	startInput(container, input) {
 		input.repeat = this.data.pins.repeat(input.path, 100, value => this.readInput(input, value));
	}
	stopInput(container, input) {
		if ("repeat" in input) {
			input.repeat.close();
			delete input.repeat;
		}
	}
}

var ProbeContainer = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin,
	Behavior: ProbeBehavior,
	contents:[
		ProbeLabel($),
		ProbeInputColumn($),
//  		ProbeOutputColumn($),
		Separator($),
	],
}));

var ProbeInputColumn = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin,
	contents:[
		$.inputs.map(input => ProbeBehavior.createInput($.pins, input))
	],
}));

var ProbeOutputColumn = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin,
	contents:[
		$.outputs.map(output => ProbeBehavior.createOutput($.pins, output))
	],
}));

var DefaultProbeInput = Container.template($ => ({
	left:0, right:0, top:0, height:40,
	contents:[
		InputLabel($),
	],
}));

var AnalogProbeInput = Container.template($ => ({
	left:0, right:0, top:0, height:145,
	contents:[
		InputLabel($),
		SampleGraphContainer($),
		SampleGraphProbeContainer($),
	],
}));

var DigitalProbeInput = Container.template($ => ({
	left:0, right:0, top:0, height:145,
	contents:[
		InputLabel($),
		SampleGraphContainer($),
		SampleGraphProbeContainer($),
	],
}));

var DefaultProbeOutput = Container.template($ => ({
	left:0, right:0, top:0, height:40,
	contents:[
		OutputLabel($),
	],
}));

let gProbeInputTypes = {
	"Number":AnalogProbeInput,
	"Boolean":DigitalProbeInput,
	"String":AnalogProbeInput,
}
