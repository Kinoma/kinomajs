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

var ButtonBehavior = Behavior.template({
	onCreate: function(container, data) {
		this.data = data;
		container.state = container.active ? 1 : 0;
	},
	onTouchBegan: function(container, id, x, y, ticks) {
 		this.changeState(container, 2);
	},
	onTouchCancelled: function(container, id, x, y, ticks) {
 		this.changeState(container, 1);
	},
	onTouchEnded: function(container, id, x, y, ticks) {
		this.changeState(container, 1);
		container.delegate("onTap");
 	},
	changeState: function(container, state) {
		container.state = state;
		if (false == ("length" in container))
			debugger
		if (container.length > 0) {
			var content = container.first;
			while (content) {
				content.state = state;
				content = content.next;
			}
		}
 	},
 	
 	// public
 	onTap: function(container){}
});
 
var Button = Container.template(function($) { return {
	active: true, style: THEME.buttonStyle, skin: THEME.buttonSkin, behavior: ButtonBehavior
}});

var ToggleButtonBehavior = ButtonBehavior.template({
	selectedVariant: 0,
	unselectedVariant: 0,
	
	onTouchBegan: function(container, id, x, y, ticks) {
		ButtonBehavior.prototype.onTouchBegan.call(this, container, id, x, y, ticks);
		container.delegate("doToggle", false);	
	},	

	// public
	
	onSelected: function(container) {
	},
	onUnselected: function(container) {
	},
	doToggle: function(container, silent) {
		var button = container
		if (button.state != 0)
			button.variant = (button.variant == this.selectedVariant) ? this.unselectedVariant : this.selectedVariant
		if (! silent) {
			if (button.variant == this.selectedVariant) 
				container.delegate("onSelected");
			else
				container.delegate("onUnselected");	
		}	           
	},
	isSelected: function(container) {
		return container.variant == this.selectedVariant;
	},
	setSelected: function(container, selected, silent) {
		if (selected != container.delegate("isSelected"))
			container.delegate("doToggle", silent);
	},
});
	
var CheckboxBehavior = ToggleButtonBehavior.template({
	selectedVariant: THEME.CHECKBOX_SELECTED,
	unselectedVariant: THEME.CHECKBOX_UNSELECTED,
});

var Checkbox = Container.template(function($) { return {
	active: true, variant: THEME.CHECKBOX_UNSELECTED, skin: THEME.glyphSkin, behavior: CheckboxBehavior
}});

var RadioButtonBehavior = ToggleButtonBehavior.template({
	onTouchBegan: function(container, id, x, y, ticks) {
		this.ignoreTouchEnded = false;
		if (container.variant == this.selectedVariant)
			this.ignoreTouchEnded = true;
		else {
			ButtonBehavior.prototype.onTouchBegan.call(this, container, id, x, y, ticks);
			container.delegate("doToggle", false);	
		}		           
	},
	onTouchEnded: function(container, id, x, y, ticks) {
		if (! this.ignoreTouchEnded)
			ButtonBehavior.prototype.onTouchEnded.call(this, container, id, x, y, ticks);
	},
});

var RadioBehavior = RadioButtonBehavior.template({
	selectedVariant: THEME.RADIO_SELECTED,
	unselectedVariant: THEME.RADIO_UNSELECTED
});
	
var Radio = Container.template(function($) { return {
	active: true, variant: THEME.RADIO_UNSELECTED, skin: THEME.glyphSkin, behavior: RadioBehavior
}});

var LabeledButtonBehavior = Behavior.template({
	onTap: function(container) {
	}
});

var LabeledButton = Container.template(function($) { return {
	style: THEME.buttonStyle,
	behavior: LabeledButtonBehavior,
	contents: [
		Button( $, {
			left: 0, top: 0, right: 0, bottom: 0,
			behavior: ButtonBehavior({
				onTap: function(container) {
					container.container.delegate("onTap");       
				}
			})
		}),
		Label( $, {left: 0, right: 0, string: $.name} )
	]
}});

var LabeledCheckboxBehavior = Behavior.template({
	onSelected: function(container) {
	},
	onUnselected: function(container) {
	}
});

var LabeledCheckbox = Line.template(function($) { return {
	style: THEME.labeledButtonStyle,
	behavior : LabeledCheckboxBehavior,
	contents: [
		Checkbox( $, {
			name: "button", left: 0,
			behavior : CheckboxBehavior({
				onSelected: function(container) {
					container.container.delegate("onSelected");
				},
				onUnselected: function(container) {
					container.container.delegate("onUnselected");
				}
			})
		}),
		Label( $, {name: "buttonLabel", left: 0, string: $.name} )
	]
}});

var LabeledRadioBehavior = Behavior.template({
	onSelected: function(container) {
	},
	onUnselected: function(container) {
	}
});

var LabeledRadio = Line.template(function($) { return {
	style: THEME.labeledButtonStyle,
	behavior : LabeledRadioBehavior,
	contents: [
		Radio( $, {
			name: "button", left: 0,
			behavior : RadioBehavior({
				onSelected: function(container) {
					var labeledRadio = container.container;
					var radioGroup = labeledRadio.container;
					labeledRadio.delegate("onSelected");
					radioGroup.delegate("onGroupButtonSelected", labeledRadio);     
				},
				onUnselected: function(container) {
					container.container.delegate("onUnselected");       
				}
			})
		}),
		Label( $, {name: "buttonLabel", left: 0, string: $.name} )
	]
}});

var RadioGroupBehavior = Behavior.template({
	onCreate: function(column, data) {
		this.data = data;
	},
	onDisplaying: function(column) {
		var data = this.data;
		var buttonStr = "buttonNames" in data ? data.buttonNames : "please add,buttonNames,and,selected,properties,to data";
		var buttonNames = buttonStr.split(",");
		var selectedName = "selected" in data ? data.selected : buttonNames[0];
		for (var i=0; i < buttonNames.length; i++) {
			var buttonName = buttonNames[i];
			var button = new LabeledRadio( { name : buttonName } );
			button.coordinates = { left : 0, top : undefined, right : undefined, bottom : undefined };
			column.add( button );
			if (buttonName == selectedName)
				button.button.delegate("doToggle", true);
		}
	},
	onGroupButtonSelected: function(column, labeledButton) {
		var aLabeledButton = column.first;
		while (aLabeledButton) {
			if (aLabeledButton === labeledButton)
				aLabeledButton.button.delegate("setSelected", true, true);
			else
				aLabeledButton.button.delegate("setSelected", false, true);
			aLabeledButton = aLabeledButton.next;
		}
		
		var buttonName = labeledButton.buttonLabel.string
		this.onRadioButtonSelected(buttonName);							// call deprecated
		
		column.delegate("onRadioGroupButtonSelected", buttonName);		// call new version
	},
	onRadioGroupButtonSelected: function(column, buttonName) {
	},
	onRadioButtonSelected: function(buttonName) {		// deprecated - use onRadioGroupButtonSelected
	},
});

var RadioGroup = Column.template(function($) { return {
	active: true,
	behavior: RadioGroupBehavior
}});



exports.ButtonBehavior = ButtonBehavior;
exports.Button = Button;
exports.ToggleButtonBehavior = ToggleButtonBehavior;
exports.CheckboxBehavior = CheckboxBehavior;
exports.Checkbox = Checkbox;
exports.RadioButtonBehavior = RadioButtonBehavior;
exports.RadioBehavior = RadioBehavior;
exports.Radio = Radio;
exports.LabeledButtonBehavior = LabeledButtonBehavior;
exports.LabeledButton = LabeledButton;
exports.LabeledCheckboxBehavior = LabeledCheckboxBehavior;
exports.LabeledCheckbox = LabeledCheckbox;
exports.LabeledRadioBehavior = LabeledRadioBehavior;
exports.LabeledRadio = LabeledRadio;
exports.RadioGroupBehavior = RadioGroupBehavior;
exports.RadioGroup = RadioGroup;
