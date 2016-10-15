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
// @fileoverview This service will handle the networking between the browser and target devices
import { Injectable } from '@angular/core';
import { Headers, Http, Response, ResponseContentType } from '@angular/http';
import 'rxjs/add/operator/toPromise';
import { AlertService } from './alert.service';

declare var Main: any; // global namespace for Blockly device integration
declare var Blockly: any; // Blockly available here

@Injectable()
export class DeploymentService{
  constructor(private http: Http, private alertService: AlertService) { }
  run(title: string, ip: string){
    document.getElementById('runIcon').className += 'active'; // spinner icon
    let applicationMain = '<?xml version="1.0" encoding="utf-8"?>'
    +  `<application xmlns="http://www.kinoma.com/kpr/application/1" id="${title}" program="src/main" title="${title}">`
    +  '</application>';
    let code = Blockly.JavaScript.workspaceToCode(Main.workspace);
    let address = `http://${ip}:10000`;
    let postData = { "debug": false, "breakOnExceptions":false, "temporary":false, "application": {"id": title, "app": `applications/${title}`} };

    this.post(`${address}/disconnect`, null, null)//.then(res => console.log(res))//close device app if running
    //upload app.xml
    .then( () => this.put(`${address}/upload?path=applications/${title}/application.xml&temporary=false`, applicationMain, 'text/plain', null)/*.then(res => console.log(res))*/ )
    //upload main code
    .then( () => this.put(`${address}/upload?path=applications/${title}/src/main.js&temporary=false`, code, 'text/plain', null)/*.then(res => console.log(res))*/ )
    //Module + Media files
    .then( () => this.getFiles('module', address, title) ).then(res => /*console.log('files loaded:'+res)*/res)
    .then( () => this.getFiles('media', address, title) ).then(res => /*console.log('files loaded:'+res)*/res)
    //launch the app
    .then( () => this.post(`${address}/launch?id=${title}&file=main.js`, JSON.stringify(postData), null))//.then(res => console.log('launched')) )
    // done/alert
    .then( () => document.getElementById('runIcon').className = ' ' )
    .catch( e => {
      this.alertService.alert(e, 'danger', true);
      console.log(e);
    });
  }
  private get(address: string, contentType: string, processData: boolean, responseType:any ){
    let headers = new Headers({'Content-Type': contentType, 'Process-Data': processData });
    return this.http.get(address, { headers: headers, responseType: responseType }).toPromise();
  }
  private post(address: string, body: any, contentType: string ){
    let headers = new Headers({'Content-Type': contentType});
    return this.http.post(address, body, {headers: headers}).toPromise();
  }
  private put(address: string, data: any, contentType: string, processData: boolean){
    let headers = new Headers({'Content-Type': contentType });
    return this.http.put(address, data, {headers: headers}).toPromise();
  }
  //Get and upload extra media/modules to the app path when necessary
  private getFiles(type: string, address: string, title: string){
    if(type === 'module'){
      if(Main.require_modules.length === 0) return new Promise((resolve, reject) => { resolve('finished') });
      else {
        let getpath = `modules/${Main.require_modules[0]}.js`;
        let sendpath = `${address}/upload?path=applications/${title}/src/${Main.require_modules[0]}.js&temporary=false`;

        let content_type = 'text/plain';
        let process_data = true;

        return this.get(getpath, content_type, process_data, ResponseContentType.Text).then(res => { return res;}).then( (res) => {
        this.put(sendpath,res.text(),content_type,process_data)}).then( () => {
          Main.require_modules.splice(0,1);
          return this.getFiles('module', address, title);
        })
      }
    } else if(type === 'media'){
      if(Main.require_media.length === 0) return new Promise((resolve, reject) => { resolve('finished') });
      let getpath = `media/sounds/${Main.require_media[0]}.wav`;
      let sendpath = `${address}/upload?path=applications/${title}/src/${Main.require_media[0]}.wav&temporary=false`;
      let content_type = 'binary';
      let process_data = false;
      return this.get(getpath, content_type, process_data, ResponseContentType.Blob).then( res => {
        let data = new Blob([res.blob()], {type: 'audio/x-wav'});
        return this.put(sendpath,data,content_type,process_data)})
        .then(res => console.log(res)).then( () => {
          Main.require_media.splice(0,1);
          return this.getFiles('media', address, title);
      })
    }
  }
  private handleError(error: any){
    let errMsg = (error.message) ? error.message :
    error.status ? `${error.status} - ${error.statusText}` : 'Server error';
    console.error(errMsg); // log to console instead
    return Promise.reject(errMsg);
  }
}
