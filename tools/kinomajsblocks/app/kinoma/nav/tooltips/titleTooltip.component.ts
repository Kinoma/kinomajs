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
import { ChangeDetectionStrategy, Component, Output, EventEmitter } from '@angular/core';
import { TooltipModule } from 'ng2-bootstrap/ng2-bootstrap';
import { CookieService } from 'angular2-cookie/core';

@Component({
  selector: 'title-input',
  template: `
    <li>
      <li><input id="title" class="hasPopup" type="text" [value]="appTitle" placeholder="app name" #title
      (keyup)="appTitle=title.value; saveCookie('title',title.value);" maxlength="30"
      tooltipPlacement="bottom" [tooltipHtml]="titletooltipHTML" tooltipTrigger="focus"
      tooltipClass='tooltipclass' [tooltipEnable]="this.appTitle=='BlocksApp'"/>
      <template #titletooltipHTML id="title-popup" class="tooltipclass">
        This will be the name of the app when installed on your Kinoma Create after running the app with the orange <strong>RUN</strong> button.
        <br/>
        <img src="media/app-tile.png" width="320" height="154" alt=""/>
      </template>
    </li>
  `,
  changeDetection: ChangeDetectionStrategy.OnPush,
  styleUrls: [ 'app/kinoma/nav/tooltips/tooltip.component.css' ]
})

export class TitleTooltipComponent {
  appTitle:string = 'BlocksApp';
  @Output() onChange = new EventEmitter<string>();
  constructor(private _cookieService: CookieService) {
      let title = _cookieService.get('title');
      if( title ) this.appTitle = title;
  }
  saveCookie(key:string, title: string){
    this._cookieService.put(key, title);
    this.onChange.emit(title);
  }
}
