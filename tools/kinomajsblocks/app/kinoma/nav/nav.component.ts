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
import { ChangeDetectionStrategy, Component, Output, EventEmitter, OnInit } from '@angular/core';
import { DeploymentService } from '../deployment.service';
import { StorageService } from '../../blocklycomponent/storage.service';
import { Subscription } from 'rxjs/Subscription';
import { CookieService } from 'angular2-cookie/core';
import { ModalsComponent } from './modals/modals.component';

@Component({
  selector: 'primary-nav',
  providers: [ DeploymentService, ModalsComponent ],
  template: `
    <nav id='primary_nav_wrap'>
      <ul class="input-list clearfix">
        <li id="runButton" (click)="deviceRun()"><a href="#" id="runIcon" class=""><span class="spinner"><i class="fa fa-spinner fa-pulse"></i></span> <span class="runText">RUN <i class="fa fa-chevron-right"></i></span></a></li>
        <li><device-dropdown #dropdown></device-dropdown></li>
        <ip-input (onChange)="ipChanged($event)"></ip-input>
        <title-input (onChange)="titleChanged($event)"></title-input>
        <li id="linkContainer"> <a id="linker" class="hasPopup" href="#" (click)="saveWorkspace(); modal.showChildModal('link')"><i class="fa fa-link"></i></a>
        <li class="switch">
          <input (click)="jsClick(); dropdown.toggleHidden()" id="cmn-toggle-7" class="cmn-toggle cmn-toggle-yes-no" type="checkbox">
          <label for="cmn-toggle-7" data-on="View Blocks" data-off="View JavaScript Source"></label>
        </li>
        <li><a id="help" class="hasPopup" href="#" (click)="modal.showChildModal('help')"><i class="fa fa-question-circle fa-2x" style="color: white; font-size: 140%;"></i></a></li>
      </ul>
    </nav>
    <modal #modal></modal>

  `,
  styleUrls: [ 'app/kinoma/nav/nav.component.css' ]
})

export class PrimaryNavComponent {
  @Output() jsClicked = new EventEmitter();
  jsClick(){
    this.jsClicked.emit(null);
  }
  subscription:Subscription;
  deviceIP: string = '';
  appTitle: string = 'blocksApp';
  public iptooltipHTML:string = 'I\'ve been made <b>bold</b>!';
  constructor(private _deploymentService: DeploymentService, private _cookieService: CookieService, private _storageService: StorageService,
  private ModalsComponent: ModalsComponent ){
    let title = _cookieService.get('title');
    let ip = _cookieService.get('ip');
    if( title ) this.appTitle = title;
    if( ip ) this.deviceIP = ip;
    // this.subscription = this._storageService.saveInstance$.subscribe(res => this.ModalsComponent.showChildModal(res));
  }
  deviceRun(){
    //TODO if valid..
    this._deploymentService.run(this.appTitle, this.deviceIP);
  }
  ipChanged(ip){
    this.deviceIP = ip;
  }
  titleChanged(title){
    this.appTitle = title;
  }
  saveWorkspace(){
    this._storageService.link(null);
  }
}
