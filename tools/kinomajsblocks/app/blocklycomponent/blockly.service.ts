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
// @fileoverview This service will handle the interfacing with the blockly library and component
import { Injectable }    from '@angular/core';
import { devices } from '../kinoma/device';
import { StorageService } from './storage.service'

declare const Blockly: any; // make Blockly available
declare const Main: any; // global namespace for special Blockly device integration
declare const prettyPrintOne: any; // Pretty print the generated code
declare const goog: any;

@Injectable()
export class BlocklyService {

  private sockets: any;
  private devices;
  constructor(private storageService: StorageService) { this.devices = devices;}

  changeDevice(device: string){
    if(Main.workspace){
      this.storageService.backupBlocks_(Main.workspace);
      Main.workspace.toolbox_.dispose();
      Main.workspace.dispose();
      this.loadToolbox(device);
    } else this.loadToolbox(device);

  }

  loadToolbox(device: string){
    let toolbox: any = {
      grid: {
        spacing: 25,
        length: 3,
        colour: '#ccc',
        snap: true },
      media: 'media/',
      toolbox: this.devices[device].xml,
      zoom: {
        controls: true,
        wheel: true,
        scaleSpeed: 1.05 },
      };

    Main.workspace = Blockly.inject('content_blocks', toolbox);
    Main.workspace.addChangeListener(e => this.onWorkspaceChange(e));
    this.onWorkspaceChange('resize');
    this.storageService.loadBlocks();
    Main.device = device;
  }

  onWorkspaceChange(e:any){
    if(e === 'resize') window.dispatchEvent(new Event('resize'));
  }

  jsClick(contentVisibility: boolean){
    if (contentVisibility === false) {
      //hide block panel
      Main.workspace.setVisible(false);
      //show js
      this.renderJavascript(true);
      //timermodule = false;
    }
    else{
      //show block panel
      Main.workspace.setVisible(true);
      //hide js
      this.renderJavascript(false);
      //trigger resize of workspace
      this.onWorkspaceChange('resize');
    }
  }

  renderJavascript(render: boolean){
    let content = document.getElementById('content_javascript');
    let code = Blockly.JavaScript.workspaceToCode(Main.workspace);
    let blocks = document.getElementById('content_blocks');
    if(render === true){
      content.innerHTML = code;
      // toggle visibilities
      content.setAttribute('style','visibility:visible');
      blocks.setAttribute('style','display:none');
    } else {
      // toggle visibilities
      content.setAttribute('style','visibility:hidden');
      blocks.setAttribute('style','display:block');
    }
    if (typeof prettyPrintOne == 'function') {
      code = content.innerHTML;
      code = prettyPrintOne(code, 'js');
      content.innerHTML = code;
    }
  }
  finishJavascript(){
    //overwrite Blockly finish for custom code finishing touches
    Blockly.JavaScript.addReservedWords('code, timeouts, checkTimeout');
    Blockly.JavaScript.finish = function(code) {
      // Convert the definitions dictionary into a list.
      let definitions = [];
      for (let name in Blockly.JavaScript.definitions_) {
        definitions.push(Blockly.JavaScript.definitions_[name]);
      }

      if(Main.device === 'create'){
        //ADDIN Create global socket connection variables.
        if(Main.socketArray.length > 0){
          for(let i=0; i<Main.socketArray.length; i++){
            definitions.push('const connection'+i+' = new WebSocket(\'ws://'+Main.socketArray[i]+':9300\');');
          }
        }
        // ADDIN require kinoma timers module
        if(Main.timermodule === true){
         definitions.push('import { setInterval, setTimeout, clearInterval, clearTimeout } from \'timers\';');
       }
        // Clean up temporary data.
        delete Blockly.JavaScript.definitions_;
        delete Blockly.JavaScript.functionNames_;
        Blockly.JavaScript.variableDB_.reset();
        return definitions.join('\n\n') + '\n\n' + code;
      }

      if(Main.device === 'element'){
        // special imports, and setups
        if(Main.socketArray.length > 0){
          for(let i=0; i < Main.socketArray.length; i++){
            definitions.push(`const connection${i} = new WebSocketClient('ws://${Main.socketArray[i]}:9300');\n
            this.connection${i} = connection${i};`);
          }
        }

        definitions[0] = `    ${definitions[0]}`; //pre defs with space visual fix
        code = definitions.join('\n\n') + '\n\n\n' + code;
        delete Blockly.JavaScript.definitions_;
        delete Blockly.JavaScript.functionNames_;
        Blockly.JavaScript.variableDB_.reset();

        if(Main.elementLED === true){
          const led_code = 'import LED from "board_led";\n';
          code = led_code.concat(code);
        }
        if(Main.wsServer === true || Main.wsClient === true){ //if need ws, include outside
          const ws_code = `import { ${Main.wsServer ? 'WebSocketServer,' : ''}) ${(Main.wsClient ? ' WebSocketClient' : '') } ' } from 'websocket';\n`;
          return ws_code.concat(code);
        } else {
          //wrap element code inside of element application framework
          let pre = '//Kinoma element application framework\nconst main = {\n'
          + '  onLaunch(){\n\n';
          code = code.replace(/(?:\r\n|\r|\n)/g, '\n    '); // add spaces to visually fix
          code = code.concat('\n  }\n}\n\nexport default main;\n' );
          return pre.concat(code);
        }
      }
    };
  }
}

export const blockly = Blockly;
export const main = Main;
export const pretty = prettyPrintOne;
