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
import { Component, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';
import { TooltipModule } from 'ng2-bootstrap/ng2-bootstrap';
import { CookieService } from 'angular2-cookie/core';

@Component({
  selector: 'ip-input',
  template: `
    <li>
      <input type="text" [value]="deviceIP" placeholder="target IP" #ip (keyup)="deviceIP=ip.value; saveCookie('ip',ip.value);" maxlength="24"
      tooltipPlacement="bottom" [tooltipHtml]="iptooltipHTML" tooltipTrigger="focus"
      tooltipClass='tooltipclass' [tooltipEnable]="this.deviceIP.length==0"/>
      <template #iptooltipHTML id="ip-popup" class="tooltipclass">
        On Kinoma Create the device IP address is viewable on the <strong>Wi-Fi</strong> tile.
        <br/>
        <img src="media/create-ip-location.png" width="320" height="240" alt=""/>
      </template>
    </li>
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
  styleUrls: [ 'app/kinoma/nav/tooltips/tooltip.component.css' ]
})

export class IPTooltipComponent {
  deviceIP:string = '';
  @Output() onChange = new EventEmitter<string>();
  constructor(private _cookieService: CookieService) {
      let ip = _cookieService.get('ip');
      if( ip ) this.deviceIP = ip;
  }
  saveCookie(key:string, ip: string){
    this._cookieService.put(key, ip);
    this.onChange.emit(ip);
  }
}
