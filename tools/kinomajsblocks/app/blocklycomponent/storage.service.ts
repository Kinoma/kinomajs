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
// This File is a derivitive of https://github.com/google/blockly/blob/master/appengine/storage.js
// Original License:
/**
 * @license
 * Visual Blocks Editor
 *
 * Copyright 2012 Google Inc.
 * https://developers.google.com/blockly/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 // Original Author: q.neutron@gmail.com (Quynh Neutron)
import { Injectable }    from '@angular/core';
import { devices } from '../kinoma/device';
import { ReplaySubject } from 'rxjs/ReplaySubject';
import { Headers, Http, Response } from '@angular/http';
import 'rxjs/add/operator/toPromise';
import { AlertService } from '../kinoma/alert.service';

declare const Blockly: any; // make Blockly available
declare const Main: any; // global namespace for special Blockly device integration
/* This Storage Service is intended to work with Google Blockly's default appengine setup.
 * Modify if other Storage implmenetation is added.
 */
@Injectable()
export class StorageService {
  constructor(private http:Http, private alertService: AlertService){}
  backupBlocks_(workspace) {
    if ('localStorage' in window) {
      let xml = Blockly.Xml.workspaceToDom(workspace);
      // Gets the current URL, not including the hash.
      let url = window.location.href.split('#')[0];
      window.localStorage.setItem('BlocklyStorage', Blockly.Xml.domToText(xml));
    }
  }

  /**
   * Bind the localStorage backup function to the unload event.
   * @param {Blockly.WorkspaceSvg=} opt_workspace Workspace.
   */
  backupOnUnload(opt_workspace) {
    var workspace = opt_workspace || Blockly.getMainWorkspace();
    window.addEventListener('unload', () => this.backupBlocks_(workspace), false);
  }

  /**
   * Restore code blocks from localStorage.
   * @param {Blockly.WorkspaceSvg=} opt_workspace Workspace.
   */
  restoreBlocks(opt_workspace) {
    if ('localStorage' in window && window.localStorage['BlocklyStorage']) {
      var workspace = opt_workspace || Blockly.getMainWorkspace();
      var xml = Blockly.Xml.textToDom(window.localStorage['BlocklyStorage']);
      Blockly.Xml.domToWorkspace(xml, workspace);
    }
  }

  /**
   * Save blocks to database and return a link containing key to XML.
   * @param {Blockly.WorkspaceSvg=} opt_workspace Workspace.
   */
  link(opt_workspace) {
    var workspace = opt_workspace || Blockly.getMainWorkspace();
    var xml = Blockly.Xml.workspaceToDom(workspace);
    var data = Blockly.Xml.domToText(xml);
    this.makeRequest_('/storage', 'xml', data, workspace);
  }

  /**
   * Retrieve XML text from database using given key.
   * @param {string} key Key to XML, obtained from href.
   * @param {Blockly.WorkspaceSvg=} opt_workspace Workspace.
   */
  retrieveXml(key, opt_workspace) {
    var workspace = opt_workspace || Blockly.getMainWorkspace();
    this.makeRequest_('/storage', 'key', key, workspace);
  }

  /**
   * Fire a new Post request, and modal if successful save
   */
  private _saveSource = new ReplaySubject<Object>();
  saveInstance$ = this._saveSource.asObservable();
  makeRequest_(url, name, content, workspace) {
    let headers = new Headers({'Content-Type': 'application/x-www-form-urlencoded'});
    this.http.post(url, name + '=' + encodeURIComponent(content), {headers: headers}).toPromise()
    .catch(e => this.alertService.alert('Storage request failed, is cloud storage implemented?','danger',true) )
    .then(res => {
      if(name === 'xml'){
        window.location.hash = res.text().trim();//TD trigger link modal with hash url
        this._saveSource.next(res.text().trim());
      }
      else if(name === 'key') {
        if (!content.length) {
          this.alertService.alert('hash error','danger',true);
        } else {
          this.loadXml_(res.text().trim(), workspace);
        }
      }
      this.monitorChanges_(workspace);
    }).catch(e => console.log(e));
  }

  /**
   * Start monitoring the workspace.  If a change is made that changes the XML,
   * clear the key from the URL.  Stop monitoring the workspace once such a
   * change is detected.
   * @param {!Blockly.WorkspaceSvg} workspace Workspace.
   * @private
   */
  monitorChanges_(workspace) {
    var startXmlDom = Blockly.Xml.workspaceToDom(workspace);
    var startXmlText = Blockly.Xml.domToText(startXmlDom);
    function change() {
      var xmlDom = Blockly.Xml.workspaceToDom(workspace);
      var xmlText = Blockly.Xml.domToText(xmlDom);
      if (startXmlText != xmlText) {
        window.location.hash = '';
        workspace.removeChangeListener(bindData);
      }
    }
    var bindData = workspace.addChangeListener(change);
  }

  /**
   * Load blocks from XML.
   * @param {string} xml Text representation of XML.
   * @param {!Blockly.WorkspaceSvg} workspace Workspace.
   * @private
   */
  loadXml_(xml, workspace) {
    try {
      xml = Blockly.Xml.textToDom(xml);

    } catch (e) {
      this.alertService.alert(e + '\nXML: ' + xml, 'danger', true);
      return;
    }
    // Clear the workspace to avoid merge.
    workspace.clear();

    Blockly.Xml.domToWorkspace(xml, workspace);
  }
  loadBlocks(){
    this.backupOnUnload(null);
    if (window.location.hash.length > 1) {
      // An href with #key trigers an AJAX call to retrieve saved blocks.
      this.retrieveXml(window.location.hash.substring(1), null);
    } else if ('BlocklyStorage' in window.localStorage) {
      // Restore saved blocks in a separate thread so that subsequent
      // initialization is not affected from a failed load.
      window.setTimeout(this.restoreBlocks, 0);
    }
  }

}
