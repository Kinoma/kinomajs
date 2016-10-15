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
// @fileoverview Device classes for defining Kinoma devices

// Blockly toolboxes for Kinoma Devices
const deviceXml = {
    create: `
    <xml id="toolbox" style="display: none">
    <sep></sep>
       <category id="catLogic" name="Logic" colour="210">
          <block type="controls_if"></block>
          <block type="logic_compare"></block>
          <block type="logic_operation"></block>
          <block type="logic_negate"></block>
          <block type="logic_boolean"></block>
          <block type="logic_null"></block>
          <block type="logic_ternary"></block>
        </category>
        <category id="catLoops" name="Loops" colour="120">
          <block type="controls_repeat_ext">
            <value name="TIMES">
              <shadow type="math_number">
                <field name="NUM">10</field>
              </shadow>
            </value>
          </block>
          <block type="controls_whileUntil"></block>
          <block type="controls_for">
            <value name="FROM">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
            <value name="TO">
              <shadow type="math_number">
                <field name="NUM">10</field>
              </shadow>
            </value>
            <value name="BY">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
          </block>
          <block type="controls_forEach"></block>
          <block type="controls_flow_statements"></block>
        </category>
        <category id="catMath" name="Math" colour="230">
          <block type="math_number"></block>
          <block type="math_arithmetic">
            <value name="A">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
            <value name="B">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
          </block>
          <block type="math_single">
            <value name="NUM">
              <shadow type="math_number">
                <field name="NUM">9</field>
              </shadow>
            </value>
          </block>
          <block type="math_trig">
            <value name="NUM">
              <shadow type="math_number">
                <field name="NUM">45</field>
              </shadow>
            </value>
          </block>
          <block type="math_constant"></block>
          <block type="math_number_property">
            <value name="NUMBER_TO_CHECK">
              <shadow type="math_number">
                <field name="NUM">0</field>
              </shadow>
            </value>
          </block>
          <block type="math_change">
            <value name="DELTA">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
          </block>
          <block type="math_round">
            <value name="NUM">
              <shadow type="math_number">
                <field name="NUM">3.1</field>
              </shadow>
            </value>
          </block>
          <block type="math_on_list"></block>
          <block type="math_modulo">
            <value name="DIVIDEND">
              <shadow type="math_number">
                <field name="NUM">64</field>
              </shadow>
            </value>
            <value name="DIVISOR">
              <shadow type="math_number">
                <field name="NUM">10</field>
              </shadow>
            </value>
          </block>
          <block type="math_constrain">
            <value name="VALUE">
              <shadow type="math_number">
                <field name="NUM">50</field>
              </shadow>
            </value>
            <value name="LOW">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
            <value name="HIGH">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>
          </block>
          <block type="math_random_int">
            <value name="FROM">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>
            <value name="TO">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>
          </block>
          <block type="math_random_float"></block>
        </category>
        <category id="catText" name="Text" colour="160">
          <block type="text"></block>
          <block type="text_join"></block>
          <block type="text_append">
            <value name="TEXT">
              <shadow type="text"></shadow>
            </value>
          </block>
          <block type="text_length">
            <value name="VALUE">
              <shadow type="text">
                <field name="TEXT">abc</field>
              </shadow>
            </value>
          </block>
          <block type="text_isEmpty">
            <value name="VALUE">
              <shadow type="text">
                <field name="TEXT"></field>
              </shadow>
            </value>
          </block>
          <block type="text_indexOf">
            <value name="VALUE">
              <block type="variables_get">
                <field name="VAR">text</field>
              </block>
            </value>
            <value name="FIND">
              <shadow type="text">
                <field name="TEXT">abc</field>
              </shadow>
            </value>
          </block>
          <block type="text_charAt">
            <value name="VALUE">
              <block type="variables_get">
                <field name="VAR">text</field>
              </block>
            </value>
          </block>
          <block type="text_getSubstring">
            <value name="STRING">
              <block type="variables_get">
                <field name="VAR">text</field>
              </block>
            </value>
          </block>
          <block type="text_changeCase">
            <value name="TEXT">
              <shadow type="text">
                <field name="TEXT">abc</field>
              </shadow>
            </value>
          </block>
          <block type="text_trim">
            <value name="TEXT">
              <shadow type="text">
                <field name="TEXT">abc</field>
              </shadow>
            </value>
          </block>

          <!-- <block type="text_print">
            <value name="TEXT">
              <shadow type="text">
                <field name="TEXT">abc</field>
              </shadow>
            </value>
          </block> -->
         <!--  <block type="text_prompt_ext">
            <value name="TEXT">
              <shadow type="text">
                <field name="TEXT">abc</field>
              </shadow>
            </value>
          </block> -->
        </category>
        <category id="catLists" name="Lists" colour="260">
          <block type="lists_create_with">
            <mutation items="0"></mutation>
          </block>
          <block type="lists_create_with"></block>
          <block type="lists_repeat">
            <value name="NUM">
              <shadow type="math_number">
                <field name="NUM">5</field>
              </shadow>
            </value>
          </block>
          <block type="lists_length"></block>
          <block type="lists_isEmpty"></block>
          <block type="lists_indexOf">
            <value name="VALUE">
              <block type="variables_get">
                <field name="VAR">list</field>
              </block>
            </value>
          </block>
          <block type="lists_getIndex">
            <value name="VALUE">
              <block type="variables_get">
                <field name="VAR">list</field>
              </block>
            </value>
          </block>
          <block type="lists_setIndex">
            <value name="LIST">
              <block type="variables_get">
                <field name="VAR">list</field>
              </block>
            </value>
          </block>
          <block type="lists_getSublist">
            <value name="LIST">
              <block type="variables_get">
                <field name="VAR">list</field>
              </block>
            </value>
          </block>
          <block type="lists_split">
            <value name="DELIM">
              <shadow type="text">
                <field name="TEXT">,</field>
              </shadow>
            </value>
          </block>
        </category>
        <category id="catColour" name="Colour" colour="20">
          <block type="colour_picker"></block>
          <block type="colour_random"></block>
          <block type="colour_rgb">
            <value name="RED">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>
            <value name="GREEN">
              <shadow type="math_number">
                <field name="NUM">50</field>
              </shadow>
            </value>
            <value name="BLUE">
              <shadow type="math_number">
                <field name="NUM">0</field>
              </shadow>
            </value>
          </block>
          <block type="colour_blend">
            <value name="COLOUR1">
              <shadow type="colour_picker">
                <field name="COLOUR">#ff0000</field>
              </shadow>
            </value>
            <value name="COLOUR2">
              <shadow type="colour_picker">
                <field name="COLOUR">#3333ff</field>
              </shadow>
            </value>
            <value name="RATIO">
              <shadow type="math_number">
                <field name="NUM">0.5</field>
              </shadow>
            </value>
          </block>
          <block type="hex_to_rgb">
            <value name="Hex">
              <shadow type="colour_picker">
                  <field name="COLOUR">#339933</field>
              </shadow>
            </value>
          </block>
        </category>
        <category id="catVariables" name="Variables" colour="330" custom="VARIABLE"></category>
        <category id="catFunctions" name="Functions" colour="290" custom="PROCEDURE"></category>
        <sep></sep>

        <!-- Begin Custom Blocks #f5f5f5-->
        <!-- Kinoma UI Elements -->
        <category id="catKUI" name="UI" colour="#7dbf2e" >
          <!-- MainScreen Container -->
          <block type="container_template">

            <value name="Container">
              <block type="variables_get">
                <field name="VAR">Screen1</field>
              </block>
            </value>

            <value name="Colour">
              <shadow type="colour_picker">
                <field name="COLOUR">#ffffff</field>
              </shadow>
            </value>

          </block>
          <!-- A simple Label Content -->
          <block type="label_content">

            <value name="Content">
              <block type="variables_get">
                <field name="VAR">Label1</field>
              </block>
            </value>

            <value name="Text">
              <shadow type="text">
                <field name="TEXT">Hello</field>
              </shadow>
            </value>

            <value name="TextColour">
              <shadow type="colour_picker">
                <field name="COLOUR">#000000</field>
              </shadow>
            </value>

            <value name="X">
              <shadow type="math_number">
                <field name="NUM">125</field>
              </shadow>
            </value>

            <value name="Y">
              <shadow type="math_number">
                <field name="NUM">150</field>
              </shadow>
            </value>

          </block>
          <!-- Touchable Button Content -->
          <block type="button_content">

            <value name="Content">
              <block type="variables_get">
                <field name="VAR">Button1</field>
              </block>
            </value>

            <value name="Text">
              <shadow type="text">
                <field name="TEXT">Press Me</field>
              </shadow>
            </value>

            <value name="Colour">
              <shadow type="colour_picker">
                <field name="COLOUR">#707070</field>
              </shadow>
            </value>

            <value name="X">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>

            <value name="Y">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>

          </block>
          <!-- Colored Rectangle Content -->
          <block type="rectangle_content">

            <value name="Content">
              <block type="variables_get">
                <field name="VAR">Rectangle1</field>
              </block>
            </value>

            <value name="Colour">
              <shadow type="colour_picker">
                <field name="COLOUR">#3333ff</field>
              </shadow>
            </value>

            <value name="Width">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>

            <value name="Height">
              <shadow type="math_number">
                <field name="NUM">100</field>
              </shadow>
            </value>

            <value name="X">
              <shadow type="math_number">
                <field name="NUM">0</field>
              </shadow>
            </value>

            <value name="Y">
              <shadow type="math_number">
                <field name="NUM">0</field>
              </shadow>
            </value>

          </block>
          <!-- Static Picture Content -->
          <block type="picture_content">
            <value name="Content">
              <block type="variables_get">
                <field name="VAR">Picture1</field>
              </block>
            </value>

            <value name="PictureUrl">
                <shadow type="picture_url">
                </shadow>
            </value>

            <value name="Scale">
              <shadow type="math_number">
                <field name="NUM">0.5</field>
              </shadow>
            </value>

            <value name="X">
              <shadow type="math_number">
                <field name="NUM">0</field>
              </shadow>
            </value>

            <value name="Y">
              <shadow type="math_number">
                <field name="NUM">0</field>
              </shadow>
            </value>

          </block>

          <!-- UI Property Setter Block -->
          <block type="object_property_setter">

            <value name="Value">
              <shadow type="colour_random"></shadow>
            </value>

          </block>
          <!-- UI Property Getter Block -->
          <block type="object_property_getter">
          </block>

        </category>

        <!-- Kinoms Pins Configuration/Calling -->
        <category id="catKPins" name="Pins" colour="#005c8a">

          <!-- Pin Configuration Block -->
          <block type="pin_config">

            <value name="Pins">
              <block type="pin_define">
                <value name="PinName">
                  <shadow type="text">
                    <field name="TEXT">GroundPin</field>
                  </shadow>
                </value>

                <field name="PinType">Ground</field>
                <value name="PinNumber">
                  <shadow type="math_number">
                    <field name="NUM">51</field>
                  </shadow>
                </value>

                <next>

                <block type="pin_define">
                  <value name="PinName">
                    <shadow type="text">
                      <field name="TEXT">PowerPin</field>
                    </shadow>
                  </value>
                  <field name="PinType">Power3.3V</field>
                  <value name="PinNumber">
                    <shadow type="math_number">
                      <field name="NUM">52</field>
                    </shadow>
                  </value>

                <next>
                  <block type="pin_define">
                    <value name="PinName">
                      <shadow type="text">
                        <field name="TEXT">Potentiometer</field>
                      </shadow>
                    </value>
                    <field name="PinType">Analog</field>
                    <value name="PinNumber">
                      <shadow type="math_number">
                        <field name="NUM">53</field>
                      </shadow>
                    </value>
                  </block>
                </next>
                </block>
                </next>
              </block>
            </value>

            <value name="PinCall">
              <block type="pin_call_read">
               <mutation type="repeat"></mutation>
                <value name="Millis">
                  <block type="math_number">
                    <field name="NUM">50</field>
                  </block>
                </value>
                <value name="PinName">
                  <shadow type="text">
                    <field name="TEXT">Potentiometer</field>
                  </shadow>
                </value>
                <value name="Result">
                  <block type="variables_get">
                    <field name="VAR">Result1</field>
                  </block>
                </value>
                <field name="CallType">repeating</field>
              </block>
            </value>

          </block>
          <!-- Empty Pin Configure Block -->
          <block type="pin_config">
          </block>
          <!-- Pin Definition Block -->
          <block type="pin_define">

            <value name="PinName">
               <shadow type="text">
                 <field name="TEXT">LED</field>
               </shadow>
            </value>

            <field name="PinType">DigitalOut</field>

            <value name="PinNumber">
              <shadow type="math_number">
                <field name="NUM">54</field>
              </shadow>
            </value>

          </block>
          <!-- Pin Read Block -->
          <block type="pin_call_read">
          <mutation type="invoke"></mutation>
            <value name="PinName">
              <shadow type="text">
                <field name="TEXT">Potentiometer</field>
              </shadow>
            </value>

            <value name="Result">
              <block type="variables_get">
                <field name="VAR">Result2</field>
              </block>
            </value>

          </block>
          <!-- Pin Write Block -->
          <block type="pin_call_write">

            <value name="PinName">
              <shadow type="text">
                <field name="TEXT">LED</field>
              </shadow>
            </value>

            <value name="Command">
              <shadow type="math_number">
                <field name="NUM">1</field>
              </shadow>
            </value>

          </block>

          <!-- Standard Trace -->
          <!-- <block type="trace">
            <value name="Tracer">
               <block type="text">
                 <field name="TEXT">Hello World!</field>
               </block>
             </value>
          </block> -->
          <!-- trace(JSON.stringify(__your_JSON_object__)) -->
          <!-- <block type="trace_JSON">
            <value name="Tracer">
               <block type="text">
                 <field name="TEXT">JSONData</field>
               </block>
             </value>
          </block>
     -->
        </category>
        <!-- Kinoma Timing Blocks Category -->
        <category id="catTime" name="Time" colour ="#FF7519">

            <block type="set_interval">
                <value name="Timer">
                  <block type="variables_get">
                      <field name="VAR">Interval1</field>
                    </block>
                </value>
                <value name="Interval">
                  <shadow type="math_number">
                    <field name="NUM">1000</field>
                  </shadow>
                </value>
            </block>

            <block type="clear_interval">
               <value name="Timer">
                  <block type="variables_get">
                      <field name="VAR">Interval1</field>
                    </block>
                </value>
            </block>

            <block type="set_timeout">
                <value name="Timer">
                  <block type="variables_get">
                      <field name="VAR">Timeout1</field>
                    </block>
                </value>
                <value name="Time">
                  <shadow type="math_number">
                    <field name="NUM">1000</field>
                  </shadow>
                </value>
            </block>

            <block type="clear_timeout">
                <value name="Timer">
                  <block type="variables_get">
                      <field name="VAR">Timeout1</field>
                    </block>
                </value>
            </block>

            <block type="init_clock">
                <value name="Clock">
                  <block type="variables_get">
                    <field name="VAR">Clock1</field>
                  </block>
                </value>
            </block>
            <block type="clock_time_getter">
                <field name="Object">Clock1</field>
                <field name="PropertyType">milliseconds</field>
            </block>
        </category>
        <!-- Kinoma Sound Blocks Category -->

        <category id="catSound" name="Sound" colour="#FFFF00">
            <block type="play_sound">
                <value name="Volume">
                  <shadow type="math_number">
                    <field name="NUM">0.5</field>
                  </shadow>
                </value>
            </block>
        </category>

        <!-- Kinoma Advanced Blocks Category -->

        <category id="catKAPins" name="Advanced" colour="#ea4335">
        <!-- Configures WebSocketServer -->
              <block type="server_config">
                  <value name="CallFunctions">
                    <block type="server_function">
                      <field name="ServerFunction">do something</field>
                    </block>
                  </value>
              </block>
              <!-- Makes local function remotely callable via Websocket server -->
              <block type="server_function">
                <field name="ServerFunction">do something else</field>
              </block>
              <!-- Call remote functions using Websocket call -->
              <block type="send_function_call">
                <value name="Address">
                     <shadow type="text">
                       <field name="TEXT">10.85.20.100</field>
                     </shadow>
                </value>
              </block>
            <!--  -->
             <!--  <block type="pin_call_read_generic">
                 <field name="PinCall">getColor</field>
                 <field name="CallType">repeat</field>
                 <value name="RepeatTime">
                  <block type="math_number">
                    <field name="NUM">10</field>
                  </block>
                </value>
                <value name="PinName">
                     <block type="text">
                       <field name="TEXT">I2C</field>
                     </block>
                   </value>
            <value name="Result">
                <block type="variables_get">
                  <field name="VAR">Color1</field>
                </block>
              </value> </block>

              <block type="pin_call_write_generic">
                <field name="PinCall">setLed</field>

                <value name="PinName">
                     <block type="text">
                       <field name="TEXT">name</field>
                     </block>
                   </value>
                   <value name="Command">
              <block type="math_number">
                <field name="NUM">1</field>
              </block>
            </value>
            </block> -->
            <!--  -->
           <!--  <block type="pin_define_colorsensor">
                <value name="PinName">
                     <block type="text">
                       <field name="TEXT">I2C</field>
                     </block>
                   </value>
                <value name="SDA">
                  <block type="math_number">
                    <field name="NUM">57</field>
                  </block>
                </value>
                <value name="Clock">
                  <block type="math_number">
                    <field name="NUM">58</field>
                  </block>
                </value>

              </block> -->
              <!-- <block type="pin_define_barometer">
                <value name="PinName">
                     <block type="text">
                       <field name="TEXT">Barometer</field>
                     </block>
                   </value>
                <value name="PinNumber">
                  <block type="math_number">
                    <field name="NUM">18</field>
                  </block>
                </value>
                <value name="Clock">
                  <block type="math_number">
                    <field name="NUM">17</field>
                  </block>
                </value>

              </block>
               -->
             <!--  <block type="pin_define_barometer">
                <value name="PinName">
                     <block type="text">
                       <field name="TEXT">Barometer</field>
                     </block>
                   </value>
                <value name="SDA">
                  <block type="math_number">
                    <field name="NUM">18</field>
                  </block>
                </value>
                <value name="Clock">
                  <block type="math_number">
                    <field name="NUM">17</field>
                  </block>
                </value>

              </block>
              <block type="shape_content">
                <value name="Content">
                  <block type="variables_get">
                    <field name="VAR">Shape1</field>
                  </block>
                </value>
                <mutation shape="square"></mutation>
              <field name="Shape">square</field>
              <value name="Colour">
              <block type="colour_picker">
                <field name="COLOUR">#3333ff</field>
              </block>
            </value>

              <value name="square">
                <block type="math_number">
                  <field name="NUM">100</field>
                </block>
              </value>

              <value name="X">
                <block type="math_number">
                  <field name="NUM">0</field>
                </block>
              </value>

              <value name="Y">
                <block type="math_number">
                  <field name="NUM">0</field>
                </block>
              </value>
                </block>    -->
          </category>
    </xml>

`,



  element: `
  <xml id="toolbox" style="display: none">
  <sep></sep>
     <category id="catLogic" name="Logic" colour="210">
        <block type="controls_if"></block>
        <block type="logic_compare"></block>
        <block type="logic_operation"></block>
        <block type="logic_negate"></block>
        <block type="logic_boolean"></block>
        <block type="logic_null"></block>
        <block type="logic_ternary"></block>
      </category>
      <category id="catLoops" name="Loops" colour="120">
        <block type="controls_repeat_ext">
          <value name="TIMES">
            <shadow type="math_number">
              <field name="NUM">10</field>
            </shadow>
          </value>
        </block>
        <block type="controls_whileUntil"></block>
        <block type="controls_for">
          <value name="FROM">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
          <value name="TO">
            <shadow type="math_number">
              <field name="NUM">10</field>
            </shadow>
          </value>
          <value name="BY">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
        </block>
        <block type="controls_forEach"></block>
        <block type="controls_flow_statements"></block>
      </category>
      <category id="catMath" name="Math" colour="230">
        <block type="math_number"></block>
        <block type="math_arithmetic">
          <value name="A">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
          <value name="B">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
        </block>
        <block type="math_single">
          <value name="NUM">
            <shadow type="math_number">
              <field name="NUM">9</field>
            </shadow>
          </value>
        </block>
        <block type="math_trig">
          <value name="NUM">
            <shadow type="math_number">
              <field name="NUM">45</field>
            </shadow>
          </value>
        </block>
        <block type="math_constant"></block>
        <block type="math_number_property">
          <value name="NUMBER_TO_CHECK">
            <shadow type="math_number">
              <field name="NUM">0</field>
            </shadow>
          </value>
        </block>
        <block type="math_change">
          <value name="DELTA">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
        </block>
        <block type="math_round">
          <value name="NUM">
            <shadow type="math_number">
              <field name="NUM">3.1</field>
            </shadow>
          </value>
        </block>
        <block type="math_on_list"></block>
        <block type="math_modulo">
          <value name="DIVIDEND">
            <shadow type="math_number">
              <field name="NUM">64</field>
            </shadow>
          </value>
          <value name="DIVISOR">
            <shadow type="math_number">
              <field name="NUM">10</field>
            </shadow>
          </value>
        </block>
        <block type="math_constrain">
          <value name="VALUE">
            <shadow type="math_number">
              <field name="NUM">50</field>
            </shadow>
          </value>
          <value name="LOW">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
          <value name="HIGH">
            <shadow type="math_number">
              <field name="NUM">100</field>
            </shadow>
          </value>
        </block>
        <block type="math_random_int">
          <value name="FROM">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>
          <value name="TO">
            <shadow type="math_number">
              <field name="NUM">100</field>
            </shadow>
          </value>
        </block>
        <block type="math_random_float"></block>
      </category>
      <category id="catText" name="Text" colour="160">
        <block type="text"></block>
        <block type="text_join"></block>
        <block type="text_append">
          <value name="TEXT">
            <shadow type="text"></shadow>
          </value>
        </block>
        <block type="text_length">
          <value name="VALUE">
            <shadow type="text">
              <field name="TEXT">abc</field>
            </shadow>
          </value>
        </block>
        <block type="text_isEmpty">
          <value name="VALUE">
            <shadow type="text">
              <field name="TEXT"></field>
            </shadow>
          </value>
        </block>
        <block type="text_indexOf">
          <value name="VALUE">
            <block type="variables_get">
              <field name="VAR">text</field>
            </block>
          </value>
          <value name="FIND">
            <shadow type="text">
              <field name="TEXT">abc</field>
            </shadow>
          </value>
        </block>
        <block type="text_charAt">
          <value name="VALUE">
            <block type="variables_get">
              <field name="VAR">text</field>
            </block>
          </value>
        </block>
        <block type="text_getSubstring">
          <value name="STRING">
            <block type="variables_get">
              <field name="VAR">text</field>
            </block>
          </value>
        </block>
        <block type="text_changeCase">
          <value name="TEXT">
            <shadow type="text">
              <field name="TEXT">abc</field>
            </shadow>
          </value>
        </block>
        <block type="text_trim">
          <value name="TEXT">
            <shadow type="text">
              <field name="TEXT">abc</field>
            </shadow>
          </value>
        </block>

        <!-- <block type="text_print">
          <value name="TEXT">
            <shadow type="text">
              <field name="TEXT">abc</field>
            </shadow>
          </value>
        </block> -->
       <!--  <block type="text_prompt_ext">
          <value name="TEXT">
            <shadow type="text">
              <field name="TEXT">abc</field>
            </shadow>
          </value>
        </block> -->
      </category>
      <category id="catLists" name="Lists" colour="260">
        <block type="lists_create_with">
          <mutation items="0"></mutation>
        </block>
        <block type="lists_create_with"></block>
        <block type="lists_repeat">
          <value name="NUM">
            <shadow type="math_number">
              <field name="NUM">5</field>
            </shadow>
          </value>
        </block>
        <block type="lists_length"></block>
        <block type="lists_isEmpty"></block>
        <block type="lists_indexOf">
          <value name="VALUE">
            <block type="variables_get">
              <field name="VAR">list</field>
            </block>
          </value>
        </block>
        <block type="lists_getIndex">
          <value name="VALUE">
            <block type="variables_get">
              <field name="VAR">list</field>
            </block>
          </value>
        </block>
        <block type="lists_setIndex">
          <value name="LIST">
            <block type="variables_get">
              <field name="VAR">list</field>
            </block>
          </value>
        </block>
        <block type="lists_getSublist">
          <value name="LIST">
            <block type="variables_get">
              <field name="VAR">list</field>
            </block>
          </value>
        </block>
        <block type="lists_split">
          <value name="DELIM">
            <shadow type="text">
              <field name="TEXT">,</field>
            </shadow>
          </value>
        </block>
      </category>
      <category id="catColour" name="Colour" colour="20">
        <block type="colour_picker"></block>
        <block type="colour_random"></block>
        <block type="colour_rgb">
          <value name="RED">
            <shadow type="math_number">
              <field name="NUM">100</field>
            </shadow>
          </value>
          <value name="GREEN">
            <shadow type="math_number">
              <field name="NUM">50</field>
            </shadow>
          </value>
          <value name="BLUE">
            <shadow type="math_number">
              <field name="NUM">0</field>
            </shadow>
          </value>
        </block>
        <block type="colour_blend">
          <value name="COLOUR1">
            <shadow type="colour_picker">
              <field name="COLOUR">#ff0000</field>
            </shadow>
          </value>
          <value name="COLOUR2">
            <shadow type="colour_picker">
              <field name="COLOUR">#3333ff</field>
            </shadow>
          </value>
          <value name="RATIO">
            <shadow type="math_number">
              <field name="NUM">0.5</field>
            </shadow>
          </value>
        </block>
        <block type="hex_to_rgb">
          <value name="Hex">
            <shadow type="colour_picker">
                <field name="COLOUR">#339933</field>
            </shadow>
          </value>
        </block>
      </category>
      <category id="catVariables" name="Variables" colour="330" custom="VARIABLE"></category>
      <category id="catFunctions" name="Functions" colour="290" custom="PROCEDURE"></category>
      <sep></sep>

      <!-- Begin Custom Blocks #f5f5f5-->


      <!-- Kinoms Pins Configuration/Calling -->
      <category id="catKPins" name="Pins" colour="#005c8a">

        <!-- Pin Configuration Block -->
        <block type="pin_config">

          <value name="Pins">
            <block type="pin_define">
              <value name="PinName">
                <shadow type="text">
                  <field name="TEXT">GroundPin</field>
                </shadow>
              </value>

              <field name="PinType">Ground</field>
              <value name="PinNumber">
                <shadow type="math_number">
                  <field name="NUM">1</field>
                </shadow>
              </value>

              <next>

              <block type="pin_define">
                <value name="PinName">
                  <shadow type="text">
                    <field name="TEXT">PowerPin</field>
                  </shadow>
                </value>
                <field name="PinType">Power3.3V</field>
                <value name="PinNumber">
                  <shadow type="math_number">
                    <field name="NUM">2</field>
                  </shadow>
                </value>

              <next>
                <block type="pin_define">
                  <value name="PinName">
                    <shadow type="text">
                      <field name="TEXT">Potentiometer</field>
                    </shadow>
                  </value>
                  <field name="PinType">Analog</field>
                  <value name="PinNumber">
                    <shadow type="math_number">
                      <field name="NUM">3</field>
                    </shadow>
                  </value>
                </block>
              </next>
              </block>
              </next>
            </block>
          </value>

          <value name="PinCall">
            <block type="pin_call_read">
             <mutation type="repeat"></mutation>
              <value name="Millis">
                <block type="math_number">
                  <field name="NUM">50</field>
                </block>
              </value>
              <value name="PinName">
                <shadow type="text">
                  <field name="TEXT">Potentiometer</field>
                </shadow>
              </value>
              <value name="Result">
                <block type="variables_get">
                  <field name="VAR">Result1</field>
                </block>
              </value>
              <field name="CallType">repeating</field>
            </block>
          </value>

        </block>
        <!-- Empty Pin Configure Block -->
        <block type="pin_config">
        </block>
        <!-- Pin Definition Block -->
        <block type="pin_define">

          <value name="PinName">
             <shadow type="text">
               <field name="TEXT">LED</field>
             </shadow>
          </value>

          <field name="PinType">DigitalOut</field>

          <value name="PinNumber">
            <shadow type="math_number">
              <field name="NUM">4</field>
            </shadow>
          </value>

        </block>
        <!-- Pin Read Block -->
        <block type="pin_call_read">
        <mutation type="invoke"></mutation>
          <value name="PinName">
            <shadow type="text">
              <field name="TEXT">Potentiometer</field>
            </shadow>
          </value>

          <value name="Result">
            <block type="variables_get">
              <field name="VAR">Result2</field>
            </block>
          </value>

        </block>
        <!-- Pin Write Block -->
        <block type="pin_call_write">

          <value name="PinName">
            <shadow type="text">
              <field name="TEXT">LED</field>
            </shadow>
          </value>

          <value name="Command">
            <shadow type="math_number">
              <field name="NUM">1</field>
            </shadow>
          </value>

        </block>
        <block type="element_LED">
          <value name="Led">
                <block type="variables_get">
                  <field name="VAR">led</field>
                </block>
              </value>
            <field name="Colour">red</field>
        </block>
        <!-- Standard Trace -->
       <!--  <block type="trace">
          <value name="Tracer">
             <block type="text">
               <field name="TEXT">Hello World!</field>
             </block>
           </value>
        </block> -->
        <!-- trace(JSON.stringify(__your_JSON_object__)) -->
      <!--   <block type="trace_JSON">
          <value name="Tracer">
             <block type="text">
               <field name="TEXT">JSONData</field>
             </block>
           </value>
        </block> -->

      </category>
      <!-- Kinoma Timing Blocks Category -->
      <category id="catTime" name="Time" colour ="#FF7519">

          <block type="set_interval">
              <value name="Timer">
                <block type="variables_get">
                    <field name="VAR">Interval1</field>
                  </block>
              </value>
              <value name="Interval">
                <shadow type="math_number">
                  <field name="NUM">1000</field>
                </shadow>
              </value>
          </block>

          <block type="clear_interval">
             <value name="Timer">
                <block type="variables_get">
                    <field name="VAR">Interval1</field>
                  </block>
              </value>
          </block>

          <block type="set_timeout">
              <value name="Timer">
                <block type="variables_get">
                    <field name="VAR">Timeout1</field>
                  </block>
              </value>
              <value name="Time">
                <shadow type="math_number">
                  <field name="NUM">1000</field>
                </shadow>
              </value>
          </block>

          <block type="clear_timeout">
              <value name="Timer">
                <block type="variables_get">
                    <field name="VAR">Timeout1</field>
                  </block>
              </value>
          </block>

          <block type="init_clock">
              <value name="Clock">
                <block type="variables_get">
                  <field name="VAR">Clock1</field>
                </block>
              </value>
          </block>
          <block type="clock_time_getter">
              <field name="Object">Clock1</field>
              <field name="PropertyType">milliseconds</field>
          </block>
      </category>

      <!-- Kinoma Advanced Blocks Category -->

      <category id="catKAPins" name="Advanced" colour="#ea4335">
      <!-- Configures WebSocketServer -->
            <block type="server_config">
                <value name="CallFunctions">
                  <block type="server_function">
                    <field name="ServerFunction">do something</field>
                  </block>
                </value>
            </block>
            <!-- Makes local function remotely callable via Websocket server -->
            <block type="server_function">
              <field name="ServerFunction">do something else</field>
            </block>
            <!-- Call remote functions using Websocket call -->
            <block type="send_function_call">
              <value name="Address">
                   <shadow type="text">
                     <field name="TEXT">10.85.20.100</field>
                   </shadow>
              </value>
            </block>
          <!--  -->
            <!-- <block type="pin_call_read_generic">
               <field name="PinCall">getColor</field>
               <field name="CallType">repeat</field>
               <value name="RepeatTime">
                <block type="math_number">
                  <field name="NUM">10</field>
                </block>
              </value>
              <value name="PinName">
                   <block type="text">
                     <field name="TEXT">I2C</field>
                   </block>
                 </value>
          <value name="Result">
              <block type="variables_get">
                <field name="VAR">Color1</field>
              </block>
            </value> </block> -->
            <!--  -->
           <!--  <block type="pin_call_write_generic">
              <field name="PinCall">setLed</field>

              <value name="PinName">
                   <block type="text">
                     <field name="TEXT">name</field>
                   </block>
                 </value>
                 <value name="Command">
            <block type="math_number">
              <field name="NUM">1</field>
            </block>
          </value>
          </block> -->



        </category>
  </xml>
`
};

export class Device {
  constructor(
    public name: string,
    public imageUrl: string,
    public xml: string
  ) { }
}

//List of supported devices
export const devices = {
    create: new Device('create', './media/create-dropdown.png', deviceXml.create),
    element: new Device('element', './media/element-dropdown.png', deviceXml.element)
};
