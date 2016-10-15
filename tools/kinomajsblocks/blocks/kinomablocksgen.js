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
// Custom Kinoma/Blockly language
/**
 * @fileoverview Blockly JavaScript generator's for Kinoma Blockly
 * @author brookelfnichols@gmail.com (Brook Elf Nichols) 2016
 */

'use strict';

goog.provide('Blockly.JavaScript.customgen');

goog.require('Blockly.JavaScript');

// rgbhex colour to kinomaJS supported strings
function colourString(rgbhex){
  var colours = { '#000000':"'black'", '#c0c0c0':"'silver'", '#808080':"'gray'", '#ffffff':"'white'", '#800000':"'maroon'", '#ff0000':"'red'",
   '#800080':"'purple'", '#ff00ff':"'fuchsia'", '#008000':"'green'", '#00ff00':"'lime'", '#808000':"'olive'", '#ffff00':"'yellow'",
   '#000080':"'navy'", '#0000ff':"'blue'", '#008080':"'teal'", '#00ffff':"'aqua'", '#ffa500':"'orange'" };
  var cutString = rgbhex.substring(1,rgbhex.length-1);
  if (cutString in colours) {
    return colours[cutString];
  } else { return rgbhex; }
}

//Global Module Inclusion
Main.require_modules=[];
Main.require_media=[];


// KinomaJS Reference
// http://kinoma.com/develop/documentation/javascript/

// ################################
// ######## UI ELEMENTS GEN #######
// ################################

// Container Instantiator
Blockly.JavaScript['container_template'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_container = Blockly.JavaScript.valueToCode(block, 'Container', Blockly.JavaScript.ORDER_ATOMIC);
  var value_colour = colourString(Blockly.JavaScript.valueToCode(block, 'Colour', Blockly.JavaScript.ORDER_ATOMIC));
  var statement_content = Blockly.JavaScript.statementToCode(block, 'Content');
  var code = variable_container+' = new Container({'
    if(value_colour) code+= ' left: 0, right: 0, top: 0, bottom: 0, skin: new Skin({ fill: '+value_colour+' }), // Code Screen Container \n';
  	else code+= ' left: 0, right: 0, top: 0, bottom: 0, skin: new Skin({ fill: \'white\' }), \n';
    code += '  behavior: Behavior({  \n'
    + '    onCreate: function(container){ // Contained UI Elements\n\n'
    + Blockly.JavaScript.prefixLines(statement_content, goog.string.repeat(Blockly.JavaScript.INDENT,2))
    + '    }\n'
    + '  })\n'
    + '});\n'
    + 'application.empty();\n'
    + 'application.add('+variable_container+');\n'
    + '\n';
  return code;
};

// Label Block
Blockly.JavaScript['label_content'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_content = Blockly.JavaScript.valueToCode(block, 'Content', Blockly.JavaScript.ORDER_ATOMIC);
  var value_text = Blockly.JavaScript.valueToCode(block,'Text',Blockly.JavaScript.ORDER_ATOMIC);
  var value_textcolour = colourString(Blockly.JavaScript.valueToCode(block,'TextColour',Blockly.JavaScript.ORDER_ATOMIC));
  var value_xcoord = Blockly.JavaScript.valueToCode(block,'X',Blockly.JavaScript.ORDER_ATOMIC);
  var value_ycoord = Blockly.JavaScript.valueToCode(block,'Y',Blockly.JavaScript.ORDER_ATOMIC);
  value_textcolour = colourString(value_textcolour);

  var code = variable_content+' = new Label({';
  if (value_xcoord) code+= ' left: '+value_xcoord;
  else code+= ' left: 0';
  if (value_ycoord) code+= ', top: '+value_ycoord;
  else code+= ', top: 0';
  if (value_text) code+= ', string: '+value_text+'';
  else code+= ', string: \'label \'';
  if(value_textcolour) code += ', style: new Style({ color: '+value_textcolour+', font: \'bold 36px Fira Sans\' })';
  else code+= ', style: new Style({ color: \'black\', font: \'bold 36px Fira Sans\' })';
    code+= ' }); // Label UI Element\n'
    + 'container.add('+variable_content+');\n\n';
  return code;
};

// Button Block
Blockly.JavaScript['button_content'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_content = Blockly.JavaScript.valueToCode(block, 'Content', Blockly.JavaScript.ORDER_ATOMIC);
  var value_text = Blockly.JavaScript.valueToCode(block,'Text',Blockly.JavaScript.ORDER_ATOMIC);
  var value_colour = colourString(Blockly.JavaScript.valueToCode(block,'Colour',Blockly.JavaScript.ORDER_ATOMIC));
  var value_xcoord = Blockly.JavaScript.valueToCode(block,'X',Blockly.JavaScript.ORDER_ATOMIC);
  var value_ycoord = Blockly.JavaScript.valueToCode(block,'Y',Blockly.JavaScript.ORDER_ATOMIC);
  var statement_ontouchbegan = Blockly.JavaScript.statementToCode(block, 'OnTouchBegan');
  var statement_ontouchended = Blockly.JavaScript.statementToCode(block, 'OnTouchEnded');


  var code = variable_content+' = new Container({ left: '+value_xcoord+', top: '+value_ycoord+', active: true, height: 50,  skin: new Skin({ fill: '+value_colour+'}), // Button UI Element\n'
  + '  behavior: Behavior({\n'
  + '    onCreate: function(container){\n'
  + '      var '+variable_content+'Label = new Label({ string: '+value_text+', style: new Style({ left: 8, right: 8, color: \'black\', font: \'bold 36px Fira Sans\' }) });\n'
  + '      container.add('+variable_content+'Label);\n'
  + '    },\n'
  + '    onTouchBegan: function(container){\n'+Blockly.JavaScript.prefixLines(statement_ontouchbegan, goog.string.repeat(Blockly.JavaScript.INDENT,2))+'\n'
  + '    },\n'
  + '    onTouchEnded: function(container){\n'+Blockly.JavaScript.prefixLines(statement_ontouchended, goog.string.repeat(Blockly.JavaScript.INDENT,2))+'\n'
  + '    }\n'
  + '  })\n'
  + '});\n'
  + 'container.add('+variable_content+');\n\n';
  return code;
};

// Rectangle Block
Blockly.JavaScript['rectangle_content'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_content = Blockly.JavaScript.valueToCode(block, 'Content', Blockly.JavaScript.ORDER_ATOMIC);
  var value_colour = colourString(Blockly.JavaScript.valueToCode(block,'Colour',Blockly.JavaScript.ORDER_ATOMIC));
  var value_xcoord = Blockly.JavaScript.valueToCode(block,'X',Blockly.JavaScript.ORDER_ATOMIC);
  var value_ycoord = Blockly.JavaScript.valueToCode(block,'Y',Blockly.JavaScript.ORDER_ATOMIC);
  var value_width = Blockly.JavaScript.valueToCode(block,'Width',Blockly.JavaScript.ORDER_ATOMIC);
  var value_height = Blockly.JavaScript.valueToCode(block,'Height',Blockly.JavaScript.ORDER_ATOMIC);

  var code = variable_content+' = new Content({ left: '+value_xcoord+', top: '+value_ycoord+', width: '+value_width+', height: '+value_height+', skin: new Skin({ fill: '+value_colour+' }) }); // Rectangle UI Element\n'
  + 'container.add('+variable_content+');\n\n';
  return code;
};

// Picture Block
Blockly.JavaScript['picture_content'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_content = Blockly.JavaScript.valueToCode(block, 'Content', Blockly.JavaScript.ORDER_ATOMIC);
  var value_picture = Blockly.JavaScript.valueToCode(block,'PictureUrl',Blockly.JavaScript.ORDER_ATOMIC);
  var value_scale = Blockly.JavaScript.valueToCode(block,'Scale',Blockly.JavaScript.ORDER_ATOMIC);
  var value_xcoord = Blockly.JavaScript.valueToCode(block,'X',Blockly.JavaScript.ORDER_ATOMIC);
  var value_ycoord = Blockly.JavaScript.valueToCode(block,'Y',Blockly.JavaScript.ORDER_ATOMIC);

  var code = variable_content + ' = new Picture({ left: '+value_xcoord+', top: '+value_ycoord+', url: '+value_picture+' });\n'
  + variable_content + '.scale = { x: '+value_scale+', y: '+value_scale+' }; // Picture UI Element\n'
  + 'container.add('+variable_content+');\n\n';
  return code;
};

// static picture urls
Blockly.JavaScript['picture_url'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var dropdown_picture = block.getFieldValue('Picture');
  return [dropdown_picture, Blockly.JavaScript.ORDER_ATOMIC];
};
// Object property Setter
Blockly.JavaScript['object_property_setter'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_object = Blockly.JavaScript.variableDB_.getName(block.getFieldValue('Object'), Blockly.Variables.NAME_TYPE);
  var dropdown_propertytype = block.getFieldValue('PropertyType');
  var value_propertyvalue = Blockly.JavaScript.valueToCode(block,'Value',Blockly.JavaScript.ORDER_ATOMIC);

  var code = variable_object+'.';
  if(dropdown_propertytype == 'SkinColour') code+= 'skin = new Skin({ fill: '+value_propertyvalue+' });';
  if(dropdown_propertytype == 'TextColour') code+= 'style = new Style({ color: '+value_propertyvalue+', font: "bold 36px Fira Sans" });';
  if(dropdown_propertytype == 'left') code+= 'moveBy( '+value_propertyvalue+' - '+variable_object +'.coordinates.left, 0);';
  if(dropdown_propertytype == 'top') code+= 'moveBy( 0, '+value_propertyvalue+' - '+variable_object +'.coordinates.top);';
  if(dropdown_propertytype == 'String') code+= 'string = '+value_propertyvalue+';';
  if(dropdown_propertytype == 'Width') code+= 'width = '+value_propertyvalue+';';
  if(dropdown_propertytype == 'Height') code+= 'height = '+value_propertyvalue+';';
  if(dropdown_propertytype == 'Scale') code += 'scale = { x: '+value_propertyvalue+', y: '+value_propertyvalue+'};';
  if(dropdown_propertytype == 'Url') code+= 'url = '+value_propertyvalue+';';
  code +="// Property Setter\n";
  return code;

};

// Object property Getter
Blockly.JavaScript['object_property_getter'] = function(block) {
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var variable_object = Blockly.JavaScript.variableDB_.getName(block.getFieldValue('Object'), Blockly.Variables.NAME_TYPE);
  var dropdown_propertytype = block.getFieldValue('PropertyType');

  if (dropdown_propertytype == 'Time') {var code= 'new Date().getTime() - '+variable_object;}
  else {
    var code = variable_object+'.';
    if(dropdown_propertytype == 'SkinColour') code+= 'skin.fillColors[0]';
    if(dropdown_propertytype == 'TextColour') code+= 'style.colors[0]';
    if(dropdown_propertytype == 'left') code+= 'coordinates.left';
    if(dropdown_propertytype == 'top') code+= 'coordinates.top';
    if(dropdown_propertytype == 'String') code+= 'string';
    if(dropdown_propertytype == 'Width') code+= 'width';
    if(dropdown_propertytype == 'Height') code+= 'height';
    if(dropdown_propertytype == 'Scale') code += 'scale.x';
    if(dropdown_propertytype == 'Url') code+= 'url';
}

  return [code, Blockly.JavaScript.ORDER_ATOMIC];
};

// ########################
// ######### PINS GEN #####
// ########################

// Pins.config def.
Blockly.JavaScript['pin_config'] = function(block) {

  var statements_pins = Blockly.JavaScript.statementToCode(block, 'Pins');

  var statements_pincall = Blockly.JavaScript.statementToCode(block, 'PinCall');

  var code = 'var Pins = require(\'pins\'); // Pin Configuration \n'
  + 'Pins.configure(\n'
  + '  (function(){\n'
  + '    var configpins = {}; // Pin Definitions\n';
  code+= Blockly.JavaScript.prefixLines(statements_pins, goog.string.repeat(Blockly.JavaScript.INDENT,1))
  + '    return configpins;\n'
  + '  })(),\n'
  + '  function(success) {\n'
  + '    if (!success) trace(\'failed to configure\');\n'
  + '    else { // On Pin Setup Success\n\n';
  code+= Blockly.JavaScript.prefixLines(statements_pincall, goog.string.repeat(Blockly.JavaScript.INDENT,2))
  + '    }\n'
  + '  }\n'
  + ');\n'
  + '\n';

  return code;
};

//define pin
Blockly.JavaScript['pin_define'] = function(block){
  var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
  var dropdown_pintype = block.getFieldValue('PinType');
  var value_pinnumber = Blockly.JavaScript.valueToCode(block, 'PinNumber', Blockly.JavaScript.ORDER_ATOMIC);

  if (value_pinname[0] == "'") {
    // Cut 's off the string
    value_pinname = value_pinname.substring(1,(value_pinname.length-1));
    var code = 'configpins.'+value_pinname+' = { ';
  }
  else{
    // Evaluate the variable
    var code = 'configpins['+value_pinname+'] = { ';
  }
    code+=  'pin: '+value_pinnumber+', ';
    if(dropdown_pintype =="PWM"){ code+= 'type: \''+dropdown_pintype+'\' '
    }
    if (dropdown_pintype == "Analog"){ code+= 'type: \''+dropdown_pintype+'\', '
    +  'direction: \'input\' ';
    }
    if (dropdown_pintype == "DigitalIn") {code+= 'type: \'Digital\', '
    +  'direction: \'input\' ';
    }
    if (dropdown_pintype == "DigitalOut") {code+= 'type: \'Digital\', '
    +  'direction: \'output\' ';
    }
    if (dropdown_pintype == "Power3.3V") {code+= 'type: \'Power\', '
    +  'voltage: 3.3 ';
    }
    if (dropdown_pintype == "Power5V" && Main.device === 'create') {code+= 'type: \'Power\', '
    +  'voltage: 5 ';
    }
    if (dropdown_pintype == "Ground") code+= 'type: \''+dropdown_pintype+'\' ';
    code+= '};\n';

  return code;
}

// Pins read def.
Blockly.JavaScript['pin_call_read'] = function(block) {

    var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
    var value_repeattime = Blockly.JavaScript.valueToCode(block, 'Millis', Blockly.JavaScript.ORDER_ATOMIC);
    var dropdown_calltype = block.getFieldValue('CallType');
    var variable_result = Blockly.JavaScript.valueToCode(block, 'Result', Blockly.JavaScript.ORDER_ATOMIC);

    var statement_onpinread = Blockly.JavaScript.statementToCode(block, 'OnPinRead');

    if(dropdown_calltype == 'repeating'){dropdown_calltype='repeat'};
    if(dropdown_calltype == 'once'){dropdown_calltype='invoke'};

    if(value_pinname.substring(0,1) === '\''){

      value_pinname = ['\'', '/', value_pinname.slice(1,value_pinname.length)].join('');
      value_pinname = [value_pinname.slice(0, -1), '/read', '\''].join('');
      var code = 'Pins.'+dropdown_calltype+'('+value_pinname+',';
    }  else { var code = 'Pins.'+dropdown_calltype+'(\'/\' + '+value_pinname+' + \'/read\','; }

      if (dropdown_calltype == 'repeat') code+=' '+value_repeattime+',';

      code += ' function(result) {\n'
      + '  var '+variable_result+' = result;  // Pin Read\n'
      + '  if('+variable_result+'!== undefined){\n'+Blockly.JavaScript.prefixLines(statement_onpinread, goog.string.repeat(Blockly.JavaScript.INDENT,1))+'\n'
      + '  }\n'
      + '});\n\n';
    return code;
};

// Pins write def.
Blockly.JavaScript['pin_call_write'] = function(block) {
    var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
    var value_command = Blockly.JavaScript.valueToCode(block, 'Command', Blockly.JavaScript.ORDER_ATOMIC);

    if(value_pinname.substring(0,1) === '\''){
      value_pinname = ['\'', '/', value_pinname.slice(1,value_pinname.length)].join('');
      value_pinname = [value_pinname.slice(0, -1), '/write', '\''].join('');
      var code = 'Pins.invoke('+value_pinname+', '+value_command+'); // Pin Write\n\n';;
    }  else { var code = 'Pins.invoke(\'/\' + '+value_pinname+' + \'/write\', '+value_command+'); // Pin Write\n\n'; }
    return code;
};

Blockly.JavaScript['trace'] = function(block) {
    var value_pinname = Blockly.JavaScript.valueToCode(block, 'Tracer', Blockly.JavaScript.ORDER_ATOMIC);

    var code = 'trace('+value_pinname+'+"\\n");\n';
    return code;
};

Blockly.JavaScript['trace_JSON'] = function(block) {
    var value_pinname = Blockly.JavaScript.valueToCode(block, 'Tracer', Blockly.JavaScript.ORDER_ATOMIC);

    var code = 'trace(JSON.stringify('+value_pinname+')+"\\n");\n';
    return code;
};

Blockly.JavaScript['element_LED'] = function(block){
  Blockly.elementLED = true;
  var variable_ledname = Blockly.JavaScript.valueToCode(block, 'Led', Blockly.JavaScript.ORDER_ATOMIC);
  var dropdown_ledcolour = block.getFieldValue('Colour');
  var color_array;
  if(dropdown_ledcolour === 'red') color_array = '[1,0,0]';
  else if(dropdown_ledcolour === 'green') color_array = '[0,1,0]';
  else if(dropdown_ledcolour === 'blue') color_array = '[0,0,1]';
  else if(dropdown_ledcolour === 'yellow') color_array = '[1,1,0]';
  else if(dropdown_ledcolour === 'purple') color_array = '[1,0,1]';
  else if(dropdown_ledcolour === 'teal') color_array = '[0,1,1]';
  else if(dropdown_ledcolour === 'white') color_array = '[1,1,1]';
  else if(dropdown_ledcolour === 'green') color_array = '[0,1,0]';

  var code = 'var '+variable_ledname+' = new LED({onColor: '+color_array+', offColor: [0, 0, 0]});\n'
  +  variable_ledname+'.on(1);\n';
  return code;
}

// ##############################
// ####### TIME GEN #############
// ##############################

// setInterval
Blockly.JavaScript['set_interval'] = function(block) {
  if(Main.device === 'create'){
    Main.timermodule = true;
    if(Main.require_modules.indexOf('timers') === -1){
      Main.require_modules.push('timers');
    }
  }
  var value_millis = Blockly.JavaScript.valueToCode(block,'Interval',Blockly.JavaScript.ORDER_ATOMIC);
  var variable_timer = Blockly.JavaScript.valueToCode(block, 'Timer', Blockly.JavaScript.ORDER_ATOMIC);
  var statement_oncomplete = Blockly.JavaScript.statementToCode(block, 'OnFinished');
  if(value_millis < 1) { value_millis = 1;} // to prevent bad case
  var code = variable_timer+' = setInterval(function(){\n'
  + Blockly.JavaScript.prefixLines(statement_oncomplete, goog.string.repeat(Blockly.JavaScript.INDENT))
  + '},' + value_millis + ');\n\n';
  return code;
};
// clearInterval
Blockly.JavaScript['clear_interval'] = function(block) {

  var variable_timer = Blockly.JavaScript.valueToCode(block, 'Timer', Blockly.JavaScript.ORDER_ATOMIC);

  var code = 'clearInterval(' + variable_timer + ');\n\n';
  return code;
};
// setTimeOut
Blockly.JavaScript['set_timeout'] = function(block) {

  if(Main.device === 'create'){
    Main.timermodule = true;
    if(Main.require_modules.indexOf('timers') === -1){
      Main.require_modules.push('timers');
    }
  }

  var value_millis = Blockly.JavaScript.valueToCode(block,'Time',Blockly.JavaScript.ORDER_ATOMIC);
  var variable_timer = Blockly.JavaScript.valueToCode(block, 'Timer', Blockly.JavaScript.ORDER_ATOMIC);

  var statement_oncomplete = Blockly.JavaScript.statementToCode(block, 'OnFinished');

  var code = variable_timer + ' = setTimeout(function(){\n'
  + Blockly.JavaScript.prefixLines(statement_oncomplete, goog.string.repeat(Blockly.JavaScript.INDENT))
  + '},' + value_millis + ');\n\n';
  return code;
};
// clearTimeOut
Blockly.JavaScript['clear_timeout'] = function(block) {

  var variable_timer = Blockly.JavaScript.valueToCode(block, 'Timer', Blockly.JavaScript.ORDER_ATOMIC);

  var code = 'clearTimeout(' + variable_timer + ');\n\n';
  return code;
};

//get current time
Blockly.JavaScript['init_clock'] = function(block) {
  var variable_content = Blockly.JavaScript.valueToCode(block, 'Clock', Blockly.JavaScript.ORDER_ATOMIC);
  var code = variable_content + ' = Date.now();\n\n'
  return code;
};

// Clock Time Getter
Blockly.JavaScript['clock_time_getter'] = function(block) {
  var variable_object = Blockly.JavaScript.variableDB_.getName(block.getFieldValue('Object'), Blockly.Variables.NAME_TYPE);
  var dropdown_propertytype = block.getFieldValue('PropertyType');

  var code = '(new Date().getTime() - ' + variable_object + ')';
  if (dropdown_propertytype == 'S')  code += '/1000';
  if (dropdown_propertytype == 'MIN')  code += '/60000';


  return [code, Blockly.JavaScript.ORDER_ATOMIC];
};
// ##################################
// ####### SOUND BLOCKS #############
// ##################################

// play a wav file sound
Blockly.JavaScript['play_sound'] = function(block){
  if(Main.device === 'element') return Main.block_device_mismatch_message;
  var dropdown_sound = block.getFieldValue('SoundType');
  var value_volume = Blockly.JavaScript.valueToCode(block,'Volume',Blockly.JavaScript.ORDER_ATOMIC);
  (Main.require_media.indexOf(dropdown_sound) == -1 ? Main.require_media.push(dropdown_sound) : null);
  var code = 'Sound.volume = ' + (value_volume ? value_volume : .1) + ';\n'
  + 'new Sound(\'' + dropdown_sound + '.wav\')' + '.play();\n';
  return code;
}

// ##################################
// ####### ADVANCED GEN #############
// ##################################

// Pins Generic read def.
Blockly.JavaScript['pin_call_read_generic'] = function(block) {

    var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
    var value_repeattime = Blockly.JavaScript.valueToCode(block, 'RepeatTime', Blockly.JavaScript.ORDER_ATOMIC);
    var variable_pincall = block.getFieldValue('PinCall');
    var dropdown_calltype = block.getFieldValue('CallType');
    var variable_result = Blockly.JavaScript.valueToCode(block, 'Result', Blockly.JavaScript.ORDER_ATOMIC);

    var statement_onpinread = Blockly.JavaScript.statementToCode(block, 'OnPinRead');



    var code = 'Pins.'+dropdown_calltype+'(\'/\'+'+value_pinname+'+\'/'+variable_pincall+'\','
      if (dropdown_calltype == 'repeat') code+=' '+value_repeattime+',';

      code += ' function(result) {\n'
      + '  var '+variable_result+' = result;\n'
      + '  if('+variable_result+'!== undefined){\n' + Blockly.JavaScript.prefixLines(statement_onpinread, goog.string.repeat(Blockly.JavaScript.INDENT,2))
      + '  }\n'
      + '});\n\n';
    return code;
};

// Pins Generic write def.
Blockly.JavaScript['pin_call_write_generic'] = function(block) {
    var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
    var value_command = Blockly.JavaScript.valueToCode(block, 'Command', Blockly.JavaScript.ORDER_ATOMIC);
    var text_pincall = block.getFieldValue('PinCall');
    var dropdown_calltype = block.getFieldValue('CallType');
    var value_onpinread = Blockly.JavaScript.valueToCode(block, 'OnPinRead', Blockly.JavaScript.ORDER_ATOMIC);

    var code = 'Pins.invoke(\'/\'+'+value_pinname+'+\'/'+text_pincall+'\', ' +value_command+');\n\n';
    return code;
};

// ############################
// # WEBSOCKET SERVER GEN #####
// ############################

Blockly.JavaScript['server_config'] = function(block){


  var statements_functions = Blockly.JavaScript.statementToCode(block, 'CallFunctions');
  if(Main.device === 'create'){
    var code = 'var wsServer = new WebSocketServer(9300); //open a new ws server\n'
    + 'var functions={};\n'
    + 'wsServer.onconnect = function(conn, options){\n'
    + '  conn.onopen = function(e){ // served functions\n'
    + Blockly.JavaScript.prefixLines(statements_functions, goog.string.repeat(Blockly.JavaScript.INDENT,1))
    + '  }\n'
    + '  conn.onmessage = function(e){\n'
    + '    var data = JSON.parse(e.data);\n'
    + '    if(data.type =="function"){ // Data passed will have type with function value\n'
    + '       var call = \'functions["\'+data.name+\'"]\' +\'(\';\n'
    + '       for (var i=0; i< Object.keys(data.params).length; i++){\n'
    + '         if(typeof data.params[\'arg\'+i] == "string"){\n'
    + '           call+= \'"\'+data.params[\'arg\'+i] + \'",\';\n'
    + '         }\n'
    + '         else call += data.params[\'arg\'+i] + \',\';\n'
    + '       }\n'
    + '       call+=\');\';\n'
    + '       try{ eval(call);} // Evaluate the passed function call\n'
    + '       catch(e){trace(\'check function name\');}\n'
    + '    }\n'
    + '  }\n'
    + '}\n'
    + '\n';
  } else{
    Main.wsServer = true;
    var code = 'this.wsServer = new WebSocketServer(9300); //open a new ws server\n'
    + 'var functions={};\n'
    + 'this.wsServer.onStart = function(conn, options){\n'
    + Blockly.JavaScript.prefixLines(statements_functions, goog.string.repeat(Blockly.JavaScript.INDENT,0))
    + '  conn.onmessage = function(e){\n'
    + '    var data = JSON.parse(e.data);\n'
    + '    if(data.type =="function"){ // Data passed will have type with function value\n'
    + '       var call = \'functions["\'+data.name+\'"]\' +\'(\';\n'
    + '       for (var i=0; i< Object.keys(data.params).length; i++){\n'
    + '         if(typeof data.params[\'arg\'+i] == "string"){\n'
    + '           call+= \'"\'+data.params[\'arg\'+i] + \'",\';\n'
    + '         }\n'
    + '         else if(Array.isArray(data.params[\'arg\'+i])){\n'
    + '           call+= \'[\'+data.params[\'arg\'+i] + \'],\';\n'
    + '         }\n'
    + '         else call += data.params[\'arg\'+i] + \',\';\n'
    + '       }\n'
    + '       call+=\');\';\n'
    + '       try{ eval(call);} // Evaluate the passed function call\n'
    + '       catch(e){trace(\'check function name\');}\n'
    + '    }\n'
    + '  }\n'
    + '}\n'
    + '\n';
  }

  return code;
}
Blockly.JavaScript['server_function'] = function(block){
  var text_function = block.getFieldValue('ServerFunction').split(' ').join('_');
  var code = 'functions[\''+ text_function+'\'] = '+text_function + ';\n';
  return code;
}
Blockly.JavaScript['send_function_call'] = function(block){
  if(Main.device === 'element') Main.wsClient = true;
 var funcName = block.getFieldValue('Function').split(' ').join('_');
 var value_address = Blockly.JavaScript.valueToCode(block, 'Address', Blockly.JavaScript.ORDER_ATOMIC);
 var branch = Blockly.JavaScript.statementToCode(block, 'STACK');
  if (Blockly.JavaScript.STATEMENT_PREFIX) {
    branch = Blockly.JavaScript.prefixLines(
        Blockly.JavaScript.STATEMENT_PREFIX.replace(/%1/g,
        '\'' + block.id + '\''), Blockly.JavaScript.INDENT) + branch;
  }
  if (Blockly.JavaScript.INFINITE_LOOP_TRAP) {
    branch = Blockly.JavaScript.INFINITE_LOOP_TRAP.replace(/%1/g,
        '\'' + block.id + '\'') + branch;
  }

  var args = [];
  var params = {};
  var connecton_n;
  var readyState;
  if(value_address.substring(0,1) === '\''){
      value_address = value_address.slice(1,value_address.length-1);
  } else{ value_address = '\' + ' + value_address + ' + \'';}

  //keep track of new socket Connections
  for(var i = 0; i < (Main.socketArray.length + 1); i++){
    if(Main.socketArray[i] == value_address){
      if(Main.device === 'element') { connecton_n = 'connection'+i;
      } else connecton_n = 'connection'+i;
      break;
    } else if(i == Main.socketArray.length){
      Main.socketArray.push(value_address);
      if(Main.device === 'element'){ connecton_n = 'connection'+(Main.socketArray.length - 1);
      } else connecton_n = 'connection'+(Main.socketArray.length - 1);
      break;
    }
  }
  if(Main.device === 'element') {
    readyState = 'statusCode';
  } else readyState = 'readyState';

  var arg_code = '';
  for (var x = 0; x < block.arguments_.length; x++) {
    args[x] = Blockly.JavaScript.variableDB_.getName(block.arguments_[x],
        Blockly.Variables.NAME_TYPE);

    arg_code += ' \'arg'+x+"\': "+args[x]+',';
  }

  var code = 'if(' + connecton_n + '.'+readyState+' == 0){\n'
  + '' + connecton_n + '.onopen = function(){\n'
  + '  ' + connecton_n + '.send(JSON.stringify({ type: "function", name: \''+funcName+'\', params: {'+arg_code+' } }) );\n'
  + ' };\n'
  + '}\n'
  + 'else ' + connecton_n + '.send(JSON.stringify({ type: "function", name: \''+funcName+'\', params: {'+arg_code+' } }) );\n';
  return code;
};

//##############################
//## BLOCKLY EXTENSION BLOCKS ##
//##############################

Blockly.JavaScript['hex_to_rgb'] = function(block) {
  // Blend two colours together.
  var value_colour = Blockly.JavaScript.valueToCode(block,'Hex',Blockly.JavaScript.ORDER_ATOMIC);

  var functionName = Blockly.JavaScript.provideFunction_(
      'hex_to_rgb',
      [ 'function ' + Blockly.JavaScript.FUNCTION_NAME_PLACEHOLDER_ +
        '(hex) {',
          ' var result = /^#?([a-f\\d]{2})([a-f\\d]{2})([a-f\\d]{2})$/i.exec(hex);',
           ' return result ? [',
                '  parseInt(result[1], 16),',
                '  parseInt(result[2], 16),',
                '  parseInt(result[3], 16)',
            ' ] : null;',
        '}\n']);
  var code = functionName + '(' + value_colour + ')';

  return [code, Blockly.JavaScript.ORDER_FUNCTION_CALL];
};

//#######################################
//## EXPERIMENTAL UNIMPLEMENTED BLOCKS ##
//#######################################


Blockly.JavaScript['pin_define_colorsensor'] = function(block){
  Main.require_modules.push('TCS34725');
  var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
  var variable_sda = Blockly.JavaScript.valueToCode(block, 'SDA', Blockly.JavaScript.ORDER_ATOMIC);
  var variable_clock = Blockly.JavaScript.valueToCode(block, 'Clock', Blockly.JavaScript.ORDER_ATOMIC);
  var variable_integrationtime = Blockly.JavaScript.valueToCode(block, 'IntTime', Blockly.JavaScript.ORDER_ATOMIC);

  if (value_pinname[0] == "'") {
    // Cut 's off the string
    value_pinname = value_pinname.substring(1,(value_pinname.length-1));
    var code = 'configpins.'+value_pinname+' = {';
  }
  else{
    // Evaluate the variable
    var code = 'configpins['+value_pinname+'] = {';
  }
  code+=  ' pins : {'
  + ' rgb : {'
  + ' sda : '+variable_sda+','
  + ' clock : '+variable_clock+','
  + ' integrationTime : 23 } },'
  + ' require : \'TCS34725\' };\n'
  return code;
};

Blockly.JavaScript['pin_define_barometer'] = function(block){
  Main.require_modules.push('BMP180');
  var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
  var variable_sda = Blockly.JavaScript.valueToCode(block, 'SDA', Blockly.JavaScript.ORDER_ATOMIC);
  var variable_clock = Blockly.JavaScript.valueToCode(block, 'Clock', Blockly.JavaScript.ORDER_ATOMIC);

  if (value_pinname[0] == "'") {
    // Cut 's off the string
    value_pinname = value_pinname.substring(1,(value_pinname.length-1));
    var code = 'configpins.'+value_pinname+' = {';
  }
  else{
    // Evaluate the variable
    var code = 'configpins['+value_pinname+'] = {';
  }
  code+=  ' pins : {'
  + ' bmp : {'
  + ' sda : '+variable_sda+','
  + ' clock : '+variable_clock+','
  + ' } }, require : \'BMP180\' };\n'
  return code;
};

Blockly.JavaScript['pin_define_range'] = function(block){
  Main.require_modules.push('MB1010');
  var value_pinname = Blockly.JavaScript.valueToCode(block, 'PinName', Blockly.JavaScript.ORDER_ATOMIC);
  var variable_pinnumber = Blockly.JavaScript.valueToCode(block, 'PinNumber', Blockly.JavaScript.ORDER_ATOMIC);

  if (value_pinname[0] == "'") {
    // Cut 's off the string
    value_pinname = value_pinname.substring(1,(value_pinname.length-1));
    var code = 'configpins.'+value_pinname+' = {';
  }
  else{
    // Evaluate the variable
    var code = 'configpins['+value_pinname+'] = {';
  }
  code+=  ' pins : {'
  + ' range : {'
  + ' pin : '+variable_pinnumber+','
  + ' require : \'MB1010\' };\n'
  return code;
};

Blockly.JavaScript['shape_content'] = function(block) {

  var variable_content = Blockly.JavaScript.valueToCode(block, 'Content', Blockly.JavaScript.ORDER_ATOMIC);
  var value_text = Blockly.JavaScript.valueToCode(block,'Text',Blockly.JavaScript.ORDER_ATOMIC);
  var value_colour = Blockly.JavaScript.valueToCode(block,'Colour',Blockly.JavaScript.ORDER_ATOMIC);
  value_colour = colourString(value_colour);
  var value_xcoord = Blockly.JavaScript.valueToCode(block,'X',Blockly.JavaScript.ORDER_ATOMIC);
  var value_ycoord = Blockly.JavaScript.valueToCode(block,'Y',Blockly.JavaScript.ORDER_ATOMIC);
  var statement_ontouchbegan = Blockly.JavaScript.statementToCode(block, 'OnTouchBegan');
  var statement_ontouchended = Blockly.JavaScript.statementToCode(block, 'OnTouchEnded');


  var code = variable_content+' = new Container({ left: '+value_xcoord+', top: '+value_ycoord+', active: true, height: 50,  skin: new Skin({ fill: '+value_colour+'}), // Button UI Element\n'
  + '      behavior: Behavior({\n'
  + '        onCreate: function(container){\n'
  + '          var '+variable_content+'Label = new Label({ string: '+value_text+', style: new Style({ left: 8, right: 8, color: \'black\', font: \'bold 36px Fira Sans\' }) });\n'
  + '          container.add('+variable_content+'Label);\n'
  + '        },\n'
  + '        onTouchBegan: function(container){\n\n'+statement_ontouchbegan+''
  + '        },\n'
  + '        onTouchEnded: function(container){\n\n'+statement_ontouchended+''
  + '        },\n'
  + '      }),\n'
  + '    });\n'
  + '    container.add('+variable_content+');\n\n';
  return code;
};
