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
import { Component } from '@angular/core';

@Component({
  selector: 'help-modal-content',
  template: `
    <div id="help-popup" class="blockly-modal">
       KinomaJS Blocks is a visual programming web app for building apps on Kinoma Hardware with KinomaJS.
       For more information/help visit the link below.
       <br/>

       <a class='doc_button'href="http://kinoma.com/develop/documentation/blockly/" target="_blank" >View Documentation</a>
       <br/>
       <a class='doc_button'href="http://kinoma.com/develop/blockly/examples.php" target="_blank" >View Examples</a>
       <br/>
       <br/>
       Found a bug? Need more help?
       <br/>
       <a class='doc_button'href=" http://forum.kinoma.com/categories/kinomajs-blocks" target="_blank" >KinomaJS Blocks Forum</a>

     </div>
    `,
    styleUrls: ['app/kinoma/nav/modals/modals.component.css']
})

export class HelpModalComponent {
  constructor(){ }
}
