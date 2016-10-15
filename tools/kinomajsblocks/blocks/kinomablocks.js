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
 // Custom KinomaJS Blockly Blocks
 /**
 * @fileoverview Block Definitions for Kinoma Blockly
 * @author brookelfnichols@gmail.com (Brook Elf Nichols) 2016
 */

'use strict';
//import { devices } from '../device';
goog.provide('Blockly.Blocks.custom');

goog.require('Blockly.Blocks');

const Main = {}; // provide namespace for global vars
Main.timermodule = false;
Main.wsServer = false;
Main.wsClient = false;
Main.socketArray = [];
Main.elementLED = false;

/**
 * device selected global, and default message for non-platform-compliant blocks
 * @type {string}
 * @type {string}
 */
Main.device = 'create';
Main.block_device_mismatch_message = '//This Block is not available on target device.\n';

// global help URL
Main.help_url = 'http://kinoma.com/develop/documentation/blockly/';

// ################################
// #### UI ELEMENTS BLOCKS ########
// ################################

// Assign variable with new Mainscreen
Blockly.Blocks['container_template'] = {
  init: function() {
    this.appendValueInput("Container")
        .appendField("screen ")
        .setCheck("Variable");
    this.appendValueInput("Colour")
        .setCheck("Colour")
        .appendField("default colour");
    this.appendStatementInput("Content")
        .setCheck("Content")
        .appendField("onCreate ");
    this.setInputsInline(true);
    this.setPreviousStatement(true, "Top Level");
    this.setNextStatement(true, "Top Level");
    this.setColour('#7dbf2e');
    this.setTooltip('New Main Screen');
    this.setHelpUrl(Main.help_url);
  }
};

// Label Instantiator
Blockly.Blocks['label_content'] = {
  init: function(){
    this.appendValueInput("Content")
        .appendField("label ")
        .setCheck("Variable");
    this.appendValueInput("Text")
        .setCheck("String")
        .appendField("text");
    this.appendValueInput("TextColour")
        .setCheck("Colour")
        .appendField("text colour");
    this.appendValueInput("X")
        .setCheck("Number")
        .appendField("left");
     this.appendValueInput("Y")
        .setCheck("Number")
        .appendField("top")
    this.setInputsInline(true);
    this.setPreviousStatement(true, "Content");
    this.setNextStatement(true, "Content");
    this.setColour('#649925');
    this.setTooltip('New Label');
    this.setHelpUrl(Main.help_url);
  }

}

// Button Instantiator
Blockly.Blocks['button_content'] = {
  init: function(){
    this.appendValueInput("Content")
        .appendField("button ")
        .setCheck("Variable");
    this.appendValueInput("Text")
        .setCheck("String")
        .appendField("text");
    this.appendValueInput("Colour")
        .setCheck("Colour")
        .appendField("colour");
    this.appendValueInput("X")
        .setCheck("Number")
        .appendField("left");
     this.appendValueInput("Y")
        .setCheck("Number")
        .appendField("top");
    this.appendStatementInput("OnTouchBegan")
        .setCheck("Content")
        .appendField("onTouch ");
    this.appendStatementInput("OnTouchEnded")
        .setCheck("Content")
        .appendField("offTouch ");
    this.setInputsInline(true);
    this.setPreviousStatement(true, "Content");
    this.setNextStatement(true, "Content");
    this.setColour('#649925');
    this.setTooltip('New Button');
    this.setHelpUrl(Main.help_url);
  }
}

// Color Rectangle Instantiator
Blockly.Blocks['rectangle_content'] = {
  init: function(){
    this.appendValueInput("Content")
        .appendField("rectangle ")
        .setCheck("Variable");
    this.appendValueInput("Colour")
        .setCheck("Colour")
        .appendField("colour");
    this.appendValueInput("Width")
        .setCheck("Number")
        .appendField("width");
    this.appendValueInput("Height")
        .setCheck("Number")
        .appendField("height");
    this.appendValueInput("X")
        .setCheck("Number")
        .appendField("left");
     this.appendValueInput("Y")
        .setCheck("Number")
        .appendField("top");
    this.setInputsInline(true);
    this.setPreviousStatement(true, "Content");
    this.setNextStatement(true, "Content");
    this.setColour('#649925');
    this.setTooltip('New Color Rectangle');
    this.setHelpUrl(Main.help_url);
  }
}

// Picture Instantiator
Blockly.Blocks['picture_content'] = {
  init: function(){
    this.appendValueInput("Content")
        .appendField("picture")
        .setCheck("Variable");
    this.appendValueInput("PictureUrl")
        .appendField("url")
        .setCheck('String');
    this.appendValueInput("Scale")
        .appendField("scale")
        .setCheck("Number");
     this.appendValueInput("X")
        .setCheck("Number")
        .appendField("left");
     this.appendValueInput("Y")
        .setCheck("Number")
        .appendField("top");
    this.setInputsInline(true);
    this.setPreviousStatement(true, "Content");
    this.setNextStatement(true, "Content");
    this.setColour('#649925');
    this.setTooltip('New Color Rectangle');
    this.setHelpUrl(Main.help_url);

  }
}
Blockly.Blocks['picture_url'] = {
  init: function(){
    this.appendDummyInput()
        .appendField(new Blockly.FieldDropdown([
        ["turtle", "\'http://images.clipartpanda.com/turtle-clip-art-4ncBLGBTA.png\'"],
        ["dog", "\'http://www.cliparthut.com/clip-arts/155/dog-clip-art-155846.jpg\'"],
        ["house", "\'http://www.filesaveas.com/images/sketch01.jpg\'"],
        ["cat", "\'http://images.clipartpanda.com/cute-cat-clipart-Kcjg6LBcq.jpeg\'"],
        ["kinoma", "\'http://blog.kinoma.com/wp-content/uploads/2014/05/Kinoma-Ball-256px-150x150.png\'"],
        ]),"Picture");
    this.setOutput(true,'String');
    this.setColour('#5BA58C');
    this.setTooltip('Picture Url Dropdown');
    this.setHelpUrl(Main.help_url);
  }
}
// generic object property setter
Blockly.Blocks['object_property_setter'] = {
  init: function(){
    this.appendValueInput("Value")
        .appendField("set ")
        .appendField( new Blockly.FieldVariable('Screen1'), 'Object')
        .appendField( new Blockly.FieldDropdown([["colour", "SkinColour"],["textcolour", "TextColour"],["left", "left"],["top", "top"],["width", "Width"],["height", "Height"],["text", "String"],["url", "Url"],["scale","Scale"]]), "PropertyType")
        .appendField("to ");
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#000099');
    this.setTooltip('Sets object property');
    this.setHelpUrl(Main.help_url);
    },
    /**
    * Return all variables referenced by this block.
    * @return {!Array.<string>} List of variable names.
    * @this Blockly.Block
    */
   getVars: function() {
     return [this.getFieldValue('Object')];
   },
   /**
    * Notification that a variable is renaming.
    * If the name matches one of this block's variables, rename it.
    * @param {string} oldName Previous name of variable.
    * @param {string} newName Renamed variable.
    * @this Blockly.Block
    */
   renameVar: function(oldName, newName) {
     if (Blockly.Names.equals(oldName, this.getFieldValue('Object'))) {
       this.setFieldValue(newName, 'Object');
     }
   }
};

// generic object property getter
Blockly.Blocks['object_property_getter'] = {
  init: function(){
    this.appendDummyInput()
        .appendField("get ")
        .appendField(new Blockly.FieldVariable("Button1"), "Object")
        .appendField(new Blockly.FieldDropdown([["colour", "SkinColour"],["textcolour", "TextColour"],["left", "left"],["top", "top"],["width", "Width"],["height", "Height"],["text", "String"],["url", "Url"],["scale","Scale"]]), "PropertyType")
    this.setOutput(true);
    this.setColour('#000099');
    this.setTooltip('Get object property');
    this.setHelpUrl(Main.help_url);
    },
    getVars: function() {
     return [this.getFieldValue('Object')];
   },
   /**
    * Notification that a variable is renaming.
    * If the name matches one of this block's variables, rename it.
    * @param {string} oldName Previous name of variable.
    * @param {string} newName Renamed variable.
    * @this Blockly.Block
    */
   renameVar: function(oldName, newName) {
     if (Blockly.Names.equals(oldName, this.getFieldValue('Object'))) {
       this.setFieldValue(newName, 'Object');
     }
   }
};

// ########################
// ###### PINS BLOCKS #####
// ########################

// Pins Configure
Blockly.Blocks['pin_config'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("pins");
    this.appendStatementInput("Pins")
        .setCheck("Pin Block")
        .appendField("onCreate ");
    this.appendStatementInput("PinCall")
        .setCheck("Pin Call Block")
        .appendField("onReady ");
    this.setPreviousStatement(true, "Top Level");
    this.setNextStatement(true, "Top Level");
    this.setColour('#005c8a');
    this.setTooltip('Pins Configure Block');
    this.setHelpUrl(Main.help_url);
  }
};

// Pin Definition
Blockly.Blocks['pin_define'] = {
  init: function(){
    if(Main.device === 'element') var drop_array = [["power 3.3V", "Power3.3V"],["ground", "Ground"],["analog", "Analog"],["digital in", "DigitalIn"],["digital out", "DigitalOut"],["pwm","PWM"]];
    else var drop_array = [["power 3.3V", "Power3.3V"],["power 5V", "Power5V"],["ground", "Ground"],["analog", "Analog"],["digital in", "DigitalIn"],["digital out", "DigitalOut"],["pwm","PWM"]];
    this.appendDummyInput()
        .appendField("make ")
        .appendField(new Blockly.FieldDropdown(drop_array), "PinType");
    this.appendValueInput("PinNumber")
        .setCheck("Number")
        .appendField("on pin");
    this.appendValueInput("PinName")
        .appendField("with name ")
        .setCheck("String");
    this.setInputsInline(true);
    this.setPreviousStatement(true, 'Pin Block');
    this.setNextStatement(true, 'Pin Block');
    this.setColour('#196C96');
    this.setTooltip('Pins Definition Block');
    this.setHelpUrl(Main.help_url);
    }

};

// Pins repeat/once read message
// Pins repeat/once read message
Blockly.Blocks['pin_call_read'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("read ");
    this.appendValueInput("PinName")
        .setCheck("String");
    this.appendValueInput("Result")
        .appendField("to")
        .setCheck("Variable");
    this.appendDummyInput()
        .appendField(new Blockly.FieldDropdown([["once", "invoke"],["repeating", "repeat"]], function(option){
            this.sourceBlock_.updateShape_(option);
        }), "CallType");
    this.option = null;
    this.setInputsInline(true);
    this.setPreviousStatement(true,"Pin Call Block");
    this.setNextStatement(true,"Pin Call Block");
    this.setColour('#71A4BD');
    this.setTooltip('Pins Read Call');
    this.setHelpUrl(Main.help_url);
  },
  mutationToDom: function() {
  var container = document.createElement('mutation');

  if(this.getFieldValue('CallType') == 'repeat' || this.getFieldValue('CallType') == 'repeating' ){
    container.setAttribute('type', 'repeat');
  }
  else if(this.getFieldValue('CallType') == 'invoke' || this.getFieldValue('CallType') == 'once' ){
    container.setAttribute('type', 'invoke');
  }
  return container;
  },
   /**
   * Parse XML to restore the argument inputs.
   * @param {!Element} xmlElement XML storage element.
   * @this Blockly.Block
   */
  domToMutation: function(xmlElement) {

    var option = xmlElement.getAttribute('type');

    this.updateShape_(option);
  },
  /**
   * Update the display of inputs for this shape block
   * @private
   * @this Blockly.Block
   */
  updateShape_: function(option) {
    // switch value block(s) based on dropdown
    if (option !== this.option){
        this.option = option;
        if(option == 'repeat' ){
            this.removeInput('OnPinRead');
            this.appendValueInput('Millis')
            .setCheck('Number')
            .appendField('every');
            this.appendDummyInput('dummytext')
            .appendField('ms');
            this.appendStatementInput("OnPinRead")
            .setCheck("Pin Read")
            .appendField("onRead ");
        }
        if(option == 'invoke'){
            this.removeInput('Millis');
            this.removeInput('OnPinRead');
            this.removeInput('dummytext');
            this.appendStatementInput("OnPinRead")
            .setCheck("Pin Read")
            .appendField("onRead ");
        }
    }
  }
};

//Pin single write message
Blockly.Blocks['pin_call_write'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("write ");
    this.appendValueInput("PinName")
        .setCheck("String");
    this.appendValueInput("Command")
        .setCheck("Number")
        .appendField("with");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#71A4BD');
    this.setTooltip('Pins Write Call');
    this.setHelpUrl(Main.help_url);
  }
};

//standard trace
Blockly.Blocks['trace'] = {
  init: function() {
    this.appendValueInput("Tracer")
        .appendField("trace ")
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour(100);
    this.setTooltip('Trace text');
    this.setHelpUrl(Main.help_url);
  }
};

//JSON trace
Blockly.Blocks['trace_JSON'] = {
  init: function() {
    this.appendValueInput("Tracer")
        .appendField("Trace JSON ")
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour(100);
    this.setTooltip('Trace Call');
    this.setHelpUrl(Main.help_url);
  }
};

Blockly.Blocks['element_LED'] = {
  init: function(){
    this.appendValueInput("Led")
      .appendField("Onboard light")
      .setCheck("Variable");
    this.appendDummyInput()
        .appendField("set to ")
        .appendField(new Blockly.FieldDropdown([["red", "red"],["blue", "blue"],["green", "green"],["yellow", "yellow"],["purple", "purple"],["teal", "teal"],["white", "white"]]), "Colour");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#71A4BD');
    this.setTooltip('element LED');
    this.setHelpUrl(Main.help_url);
  }
}

// ##############################
// ####### TIME BLOCKS ##########
// ##############################

//wait for x ms
Blockly.Blocks['set_interval'] = {
  init: function() {
    this.appendValueInput("Timer")
        .appendField("interval ")
        .setCheck("Variable");
    this.appendDummyInput()
        .appendField("every");
    this.appendValueInput("Interval")
        .setCheck("Number");
    this.appendDummyInput()
        .appendField("ms");
    this.appendStatementInput("OnFinished")
        .appendField("onInterval ");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#FF7519');
    this.setTooltip('Set Interval');
    this.setHelpUrl(Main.help_url);
  }
};

Blockly.Blocks['clear_interval'] = {
  init: function() {
    this.appendValueInput("Timer")
        .appendField("clear ")
        .setCheck("Variable");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#FF944D');
    this.setTooltip('Clear Interval');
    this.setHelpUrl(Main.help_url);
  }
};

Blockly.Blocks['set_timeout'] = {
  init: function() {
    this.appendValueInput("Timer")
        .appendField("timeout ")
        .setCheck("Variable");
    this.appendDummyInput()
        .appendField("on");
    this.appendValueInput("Time")
        .setCheck("Number");
    this.appendDummyInput()
        .appendField("ms");
    this.appendStatementInput("OnFinished")
        .appendField("onTimeout ");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#FF7519');
    this.setTooltip('Set Timeout');
    this.setHelpUrl(Main.help_url);
  }
};

Blockly.Blocks['clear_timeout'] = {
  init: function() {
    this.appendValueInput("Timer")
        .appendField("clear ")
        .setCheck("Variable");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#FF944D');
    this.setTooltip('Clear Timeout');
    this.setHelpUrl(Main.help_url);
  }
};

Blockly.Blocks['init_clock'] = {
  init: function(){
    this.appendValueInput("Clock")
        .appendField("start clock")
        .setCheck("Variable");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#FF7530');
    this.setTooltip('Start a Clock');
    this.setHelpUrl(Main.help_url);
  }
}

// generic object property getter
Blockly.Blocks['clock_time_getter'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("get ")
        .appendField(new Blockly.FieldVariable("Clock1"), "Object")
        .appendField("time in")
        .appendField(new Blockly.FieldDropdown([["milliseconds", "MS"],["seconds", "S"],["minutes", "MIN"]]), "PropertyType")
    this.setOutput(true);
    this.setColour('#FF8040');
    this.setTooltip('Get object property');
    this.setHelpUrl(Main.help_url);
    },
    getVars: function()  {
     return [this.getFieldValue('Object')];
   },
   /**
    * Notification that a variable is renaming.
    * If the name matches one of this block's variables, rename it.
    * @param {string} oldName Previous name of variable.
    * @param {string} newName Renamed variable.
    * @this Blockly.Block
    */
   renameVar: function(oldName, newName) {
     if (Blockly.Names.equals(oldName, this.getFieldValue('Object'))) {
       this.setFieldValue(newName, 'Object');
     }
   }
};


// ##################################
// ####### SOUND BLOCKS #############
// ##################################

// play a sound
Blockly.Blocks['play_sound'] = {
  init: function(){
    this.appendDummyInput()
        .appendField(new Blockly.FieldLabel('play ', 'sound_block_text_css'))
        .appendField(new Blockly.FieldDropdown([["applause", "applause"],["boing", "boing"],["cat", "cat"],["chicken", "chicken"],
            ["chime", "chime_up"],["dog", "dog"],["fart", "fart"],["sheep", "sheep"],["Hooray!", "yay"]]), "SoundType");
    this.appendValueInput('Volume')
        .setCheck('Number')
        .appendField(new Blockly.FieldLabel(' at volume ', 'sound_block_text_css'));
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#FFD633');
    this.setTooltip('Play a Sound');
    this.setHelpUrl(Main.help_url);
  }
}

// ##################################
// ####### ADVANCED BLOCKS ##########
// ##################################

// Creates new WebSocketServer for receiving function calls
Blockly.Blocks['server_config'] = {
  init: function() {
    this.setColour(5);
    this.appendDummyInput()
        .appendField("server");
    this.appendStatementInput("CallFunctions")
        .appendField("serveFunctions")
        .setCheck("Server Function");
    this.setInputsInline(true);
    this.setPreviousStatement(true,"Top Level");
    this.setNextStatement(true,"Top Level");
    this.setTooltip("Configure server");
    }
}

// Specifies which functions to make available for remote call
Blockly.Blocks['server_function'] = {
  init: function() {
    this.setColour(10);
    this.appendDummyInput()
        .appendField("serve function")
        .appendField(new Blockly.FieldTextInput("do something"), "ServerFunction")
    this.setInputsInline(true);
    this.setPreviousStatement(true,'Server Function');
    this.setNextStatement(true,'Server Function');
    this.setTooltip("Set served function");
    }
}

// Opens Websocket connection and sends function call JSON message, then closes
Blockly.Blocks['send_function_call'] = {
  init: function() {
    this.setColour('#EA4335');
    this.appendDummyInput()
        .appendField("call")
        .appendField(new Blockly.FieldTextInput("do something"), "Function")
        .appendField('', 'PARAMS');
    this.appendValueInput("Address")
        .setCheck("String")
        .appendField("at ");
    this.setMutator(new Blockly.Mutator(['procedures_mutatorarg']));
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setTooltip("Call remote function");
    this.arguments_ = [];
  },

  /**
   * Update the display of parameters for this function call block
   * Display a warning if there are duplicately named parameters.
   * @private
   * @this Blockly.Block
   */
  updateParams_: function() {
    // Check for duplicated arguments.
    var badArg = false;
    var hash = {};
    for (var i = 0; i < this.arguments_.length; i++) {
      if (hash['arg_' + this.arguments_[i].toLowerCase()]) {
        badArg = true;
        break;
      }
      hash['arg_' + this.arguments_[i].toLowerCase()] = true;
    }
    if (badArg) {
      this.setWarningText(Blockly.Msg.PROCEDURES_DEF_DUPLICATE_WARNING);
    } else {
      this.setWarningText(null);
    }
    // Merge the arguments into a human-readable list.
    var paramString = '';
    if (this.arguments_.length) {
      paramString = Blockly.Msg.PROCEDURES_BEFORE_PARAMS +
          ' ' + this.arguments_.join(', ');
    }
    this.setFieldValue(paramString, 'PARAMS');
  },
  /**
   * Create XML to represent the argument inputs.
   * @return {!Element} XML storage element.
   * @this Blockly.Block
   */
  mutationToDom: function() {
    var container = document.createElement('mutation');
    for (var i = 0; i < this.arguments_.length; i++) {
      var parameter = document.createElement('arg');
      parameter.setAttribute('name', this.arguments_[i]);
      container.appendChild(parameter);
    }
    return container;
  },
  /**
   * Parse XML to restore the argument inputs.
   * @param {!Element} xmlElement XML storage element.
   * @this Blockly.Block
   */
  domToMutation: function(xmlElement) {
    this.arguments_ = [];
    for (var i = 0, childNode; childNode = xmlElement.childNodes[i]; i++) {
      if (childNode.nodeName.toLowerCase() == 'arg') {
        this.arguments_.push(childNode.getAttribute('name'));
      }
    }
    this.updateParams_();
  },
  /**
   * Populate the mutator's dialog with this block's components.
   * @param {!Blockly.Workspace} workspace Mutator's workspace.
   * @return {!Blockly.Block} Root block in mutator.
   * @this Blockly.Block
   */
  decompose: function(workspace) {
    var containerBlock = Blockly.Block.obtain(workspace,
                                              'procedures_mutatorcontainer');
    containerBlock.initSvg();
    containerBlock.getInput('STATEMENT_INPUT').setVisible(false);
    // Parameter list.
    var connection = containerBlock.getInput('STACK').connection;
    for (var i = 0; i < this.arguments_.length; i++) {
      var paramBlock = Blockly.Block.obtain(workspace, 'procedures_mutatorarg');
      paramBlock.initSvg();
      paramBlock.setFieldValue(this.arguments_[i], 'NAME');
      // Store the old location.
      paramBlock.oldLocation = i;
      connection.connect(paramBlock.previousConnection);
      connection = paramBlock.nextConnection;
    }

    return containerBlock;
  },
  /**
   * Reconfigure this block based on the mutator dialog's components.
   * @param {!Blockly.Block} containerBlock Root block in mutator.
   * @this Blockly.Block
   */
  compose: function(containerBlock) {
    // Parameter list.
    this.arguments_ = [];
    this.paramIds_ = [];
    var paramBlock = containerBlock.getInputTargetBlock('STACK');
    while (paramBlock) {
      this.arguments_.push(paramBlock.getFieldValue('NAME'));
      this.paramIds_.push(paramBlock.id);
      paramBlock = paramBlock.nextConnection && paramBlock.nextConnection.targetBlock();
    }
    this.updateParams_();

  },
  /**
   * Return all variables referenced by this block.
   * @return {!Array.<string>} List of variable names.
   * @this Blockly.Block
   */
  getVars: function() {
    return this.arguments_;
  },
  /**
   * Notification that a variable is renaming.
   * If the name matches one of this block's variables, rename it.
   * @param {string} oldName Previous name of variable.
   * @param {string} newName Renamed variable.
   * @this Blockly.Block
   */
  renameVar: function(oldName, newName) {
    var change = false;
    for (var i = 0; i < this.arguments_.length; i++) {
      if (Blockly.Names.equals(oldName, this.arguments_[i])) {
        this.arguments_[i] = newName;
        change = true;
      }
    }
    if (change) {
      this.updateParams_();
      // Update the mutator's variables if the mutator is open.
      if (this.mutator.isVisible()) {
        var blocks = this.mutator.workspace_.getAllBlocks();
        for (var i = 0, block; block = blocks[i]; i++) {
          if (block.type == 'procedures_mutatorarg' &&
              Blockly.Names.equals(oldName, block.getFieldValue('NAME'))) {
            block.setFieldValue(newName, 'NAME');
          }
        }
      }
    }
  },
  /**
   * Add custom menu options to this block's context menu.
   * @param {!Array} options List of menu options to add to.
   * @this Blockly.Block
   */
  customContextMenu: function(options) {
    // Add option to create caller.
    var option = {enabled: true};
    var name = this.getFieldValue('NAME');
    option.text = Blockly.Msg.PROCEDURES_CREATE_DO.replace('%1', name);
    var xmlMutation = goog.dom.createDom('mutation');
    xmlMutation.setAttribute('name', name);
    for (var i = 0; i < this.arguments_.length; i++) {
      var xmlArg = goog.dom.createDom('arg');
      xmlArg.setAttribute('name', this.arguments_[i]);
      xmlMutation.appendChild(xmlArg);
    }
    var xmlBlock = goog.dom.createDom('block', null, xmlMutation);
    xmlBlock.setAttribute('type', this.callType_);
    option.callback = Blockly.ContextMenu.callbackFactory(this, xmlBlock);
    options.push(option);

    // Add options to create getters for each parameter.
    if (!this.isCollapsed()) {
      for (var i = 0; i < this.arguments_.length; i++) {
        var option = {enabled: true};
        var name = this.arguments_[i];
        option.text = Blockly.Msg.VARIABLES_SET_CREATE_GET.replace('%1', name);
        var xmlField = goog.dom.createDom('field', null, name);
        xmlField.setAttribute('name', 'VAR');
        var xmlBlock = goog.dom.createDom('block', null, xmlField);
        xmlBlock.setAttribute('type', 'variables_get');
        option.callback = Blockly.ContextMenu.callbackFactory(this, xmlBlock);
        options.push(option);
      }
    }
  }
};

//##############################
//## BLOCKLY EXTENSION BLOCKS ##
//##############################

Blockly.Blocks['hex_to_rgb'] = {
  init: function() {
    this.setColour(Blockly.Blocks.lists.HUE);
    this.appendValueInput('Hex')
        .setCheck('Colour');
    this.appendDummyInput()
        .appendField('to [ R, G, B ] list');
    this.setInputsInline(true);
    this.setOutput(true, 'List');
  }
}

//#######################################
//## EXPERIMENTAL UNIMPLEMENTED BLOCKS ##
//#######################################

// TODO: Send Code to be executed // ws server
Blockly.Blocks['send_code'] = {
  init: function() {
    this.setColour('#EA4335');
    this.appendDummyInput()
        .appendField("call")
        .appendField(new Blockly.FieldTextInput("do something"), "Function")
        .appendField('', 'PARAMS');
     this.appendDummyInput()
        .appendField("at  ")
       .appendField(new Blockly.FieldTextInput("10.85.20.121"), "Address")
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setTooltip("Send code remotely");
  }
}

// Color Rectangle Instantiator // UI
Blockly.Blocks['shape_content'] = {
  init: function(){
    this.appendValueInput("Content")
        .appendField("shape ")
        .setCheck("Variable");
    this.appendDummyInput()
        .appendField("of type")
        .appendField(new Blockly.FieldDropdown([["square", "square"],["circle", "circle"],["triangle","triangle"],["rectangle","rectangle"]], function(option){
            this.sourceBlock_.updateShape_(option);
        }), "Shape");
    this.appendValueInput("Colour")
        .setCheck("Colour")
        .appendField("colour");
    this.appendValueInput("X")
        .setCheck("Number")
        .appendField("left");
     this.appendValueInput("Y")
        .setCheck("Number")
        .appendField("top");
    this.setInputsInline(true);
    this.setPreviousStatement(true,"Content");
    this.setNextStatement(true,"Content");
    this.setColour('#649925');
    this.setTooltip('New Shape');
    this.setHelpUrl(Main.help_url);
  },
   /**
   * Create XML to represent the argument inputs.
   * @return {!Element} XML storage element.
   * @this Blockly.Block
   */
  mutationToDom: function() {
  var container = document.createElement('mutation');
  if(this.getFieldValue('Shape') == 'square'){
    container.setAttribute('shape', 'square');
  }
  else if(this.getFieldValue('Shape') == 'triangle'){
    container.setAttribute('shape', 'triangle');
  }
  else if(this.getFieldValue('Shape') == 'circle'){
    container.setAttribute('shape', 'circle');
  }
  else if(this.getFieldValue('Shape') == 'rectangle'){
    container.setAttribute('shape', 'rectangle');
  }
  return container;
  },
   /**
   * Parse XML to restore the argument inputs.
   * @param {!Element} xmlElement XML storage element.
   * @this Blockly.Block
   */
  domToMutation: function(xmlElement) {
    var option = xmlElement.getAttribute('shape');
    this.updateShape_(option);
  },
  /**
   * Update the display of inputs for this shape block
   * @private
   * @this Blockly.Block
   */
  updateShape_: function(option) {
    // switch value block(s) based on dropdown

    if(this.getInput('triangle')){this.removeInput('triangle'); }
    if(this.getInput('circle')){this.removeInput('circle'); }
    if(this.getInput('square')){this.removeInput('square'); }
    if(this.getInput('rectangle_h')){ this.removeInput('rectangle_h'); }
    if(this.getInput('rectangle_w')){  this.removeInput('rectangle_w'); }

    if(option == 'square'){
        this.appendValueInput(option)
        .setCheck('Number')
        .appendField('width');
    }
    if(option == 'triangle'){
        this.appendValueInput(option)
        .setCheck('Number')
        .appendField('length');
    }
    if(option == 'circle'){
        this.appendValueInput(option)
        .setCheck('Number')
        .appendField('radius');
    }

    if(option == 'rectangle'){
        this.appendValueInput('rectangle_h')
        .setCheck('Number')
        .appendField('height');
        this.appendValueInput('rectangle_w')
        .setCheck('Number')
        .appendField('width');
    }

  }
};

// Call any read function once/repeat // pins
Blockly.Blocks['pin_call_read_generic'] = {
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldTextInput("read"), "PinCall")
    this.appendValueInput("PinName")
        .setCheck("String");
    this.appendDummyInput()
        .appendField("of type")
        .appendField(new Blockly.FieldDropdown([["once", "invoke"],["repeating", "repeat"]]), "CallType");
    this.appendValueInput("Result")
        .appendField("result")
        .setCheck("Variable");
    this.appendValueInput("RepeatTime")
        .appendField("in")
        .setCheck("Number");
    this.appendDummyInput()
        .appendField("ms");
    this.appendStatementInput("OnPinRead")
        .setCheck()
        .appendField("onReturn ");
    this.setInputsInline(true,"Pin Call Block");
    this.setPreviousStatement(true,"Pin Call Block");
    this.setNextStatement(true);
    this.setColour('#71A4BD');
    this.setTooltip('Pins Generic Call');
    this.setHelpUrl(Main.help_url);
  }
};

// Call any write function // pins
Blockly.Blocks['pin_call_write_generic'] = {
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldTextInput("write"), "PinCall")
    this.appendValueInput("PinName")
        .setCheck("String");
    this.appendDummyInput()
        .appendField("of type")
        .appendField(new Blockly.FieldDropdown([["once", "invoke"],["repeating", "repeat"]]), "CallType");
    this.appendValueInput("Command")
        .appendField("with");
    this.setInputsInline(true);
    this.setPreviousStatement(true);
    this.setNextStatement(true);
    this.setColour('#71A4BD');
    this.setTooltip('Pins Write Call');
    this.setHelpUrl(Main.help_url);
  }
};

//i2c colorsensor // pins
Blockly.Blocks['pin_define_colorsensor'] = {
  init: function(){
    this.appendDummyInput()
        .appendField("make i2c colorsensor ");
    this.appendValueInput("SDA")
        .setCheck("Number")
        .appendField("on sda pin ");
    this.appendValueInput("Clock")
        .setCheck("Number")
        .appendField(" and clock pin ");
    this.appendValueInput("PinName")
        .appendField("with name ")
        .setCheck("String");
    this.setInputsInline(true);
    this.setPreviousStatement(true, 'Pin Block');
    this.setNextStatement(true, 'Pin Block');
    this.setColour('#196C96');
    this.setTooltip('Colorsensor Definition Block');
    this.setHelpUrl(Main.help_url);
    }

};

// i2c barometer // pins
Blockly.Blocks['pin_define_barometer'] = {
  init: function(){
    this.appendDummyInput()
        .appendField("make i2c barometer ");
    this.appendValueInput("SDA")
        .setCheck("Number")
        .appendField("on sda pin ");
    this.appendValueInput("Clock")
        .setCheck("Number")
        .appendField(" and clock pin ");
    this.appendValueInput("PinName")
        .appendField("with name ")
        .setCheck("String");
    this.setInputsInline(true);
    this.setPreviousStatement(true, 'Pin Block');
    this.setNextStatement(true, 'Pin Block');
    this.setColour('#196C96');
    this.setTooltip('Barometer Definition Block');
    this.setHelpUrl(Main.help_url);
    }

};

// i2c rangefinder // pins
Blockly.Blocks['pin_define_range'] = {
  init: function(){
    this.appendDummyInput()
        .appendField("make rangefinder");
    this.appendValueInput("PinNumber")
        .setCheck("Number")
        .appendField(" on analog pin ");
    this.appendValueInput("PinName")
        .appendField("with name ")
        .setCheck("String");
    this.setInputsInline(true);
    this.setPreviousStatement(true, 'Pin Block');
    this.setNextStatement(true, 'Pin Block');
    this.setColour(205);
    this.setTooltip('Rangesensor Definition Block');
    this.setHelpUrl(Main.help_url);
    }

};
