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
import { Component, Output, EventEmitter } from '@angular/core';
import { Device, devices } from '../../device';
import { NgFor } from '@angular/common';
import { BlocklyService } from '../../../blocklycomponent/blockly.service';
import { CookieService } from 'angular2-cookie/core';
import { DropdownModule } from 'ng2-bootstrap/ng2-bootstrap';

@Component({
  selector: 'device-dropdown',
  providers: [ BlocklyService ],
  template: `
      <div id="deviceSelect" [hidden]=jsRendered>
        <!-- Simple dropdown -->
        <span dropdown (onToggle)="toggled($event)">
          <a href dropdownToggle>
            <img [src]="getDeviceUrl(this.selectedDevice)"/>
          </a>
          <ul class="dropdown-menu" dropdownMenu aria-labelledby="simple-dropdown">
            <li *ngFor="let choice of items">
              <a class="dropdown-item" href="#" (click)="selectDevice(choice.name)" ><img [src]="getDeviceUrl(choice.name)"/>Kinoma {{choice.name}}</a>
            </li>
          </ul>
        </span>
      </div>
  `,
  styleUrls: [ 'app/kinoma/nav/dropdown/dropdown.component.css' ]
})

export class DropdownComponent {
  jsRendered = false;
  devices;
  selectedDevice;
  public disabled:boolean = false;
  public status:{isopen:boolean} = {isopen: false};
  public items:Array<any>;

  constructor(private blocklyService: BlocklyService, private _cookieService:CookieService){
    this.devices = devices;
    this.selectedDevice = _cookieService.get('device') || 'create';
    this.items = Object.keys(devices).map(key => devices[key])
  }
  getDeviceUrl(device:string){
    return this.devices[device].imageUrl;
  }
  selectDevice(device: string){

    this.blocklyService.changeDevice(device);
    this.selectedDevice = device;
    this._cookieService.put('device', device);
  }
  public toggled(open:boolean):void {
   //nothing useful here...
  }
  //hide device selection on jsRendered avoids unecessary rendering conflicts
  public toggleHidden(){
    this.jsRendered = !this.jsRendered;
  }

  public toggleDropdown($event:MouseEvent):void {
    $event.preventDefault();
    $event.stopPropagation();
    this.status.isopen = !this.status.isopen;
  }
}
