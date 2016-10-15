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
import { NgModule }      from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { FormsModule }   from '@angular/forms';
import { HttpModule }     from '@angular/http';
import "rxjs/Rx";
//import { NgbModule } from '@ng-bootstrap/ng-bootstrap';//bootstrap

import { AppComponent }  from './app.component';

import { BlocklyComponent } from './blocklycomponent/blockly.component';

import { PrimaryNavComponent } from './kinoma/nav/nav.component';
import { DropdownComponent } from './kinoma/nav/dropdown/dropdown.component';
import { ModalsComponent } from './kinoma/nav/modals/modals.component';
import { HelpModalComponent } from './kinoma/nav/modals/help.component';
import { LinkModalComponent } from './kinoma/nav/modals/link.component';
import { IPTooltipComponent } from './kinoma/nav/tooltips/ipTooltip.component';
import { TitleTooltipComponent } from './kinoma/nav/tooltips/titleTooltip.component';

// Cookies
import { CookieService } from 'angular2-cookie/services/cookies.service';
// Custom Alerts
import { AlertService } from './kinoma/alert.service';
// Local / Cloud Storage Handling
import { StorageService } from './blocklycomponent/storage.service';
// Bootstrap Component Library
import { Ng2BootstrapModule } from 'ng2-bootstrap/ng2-bootstrap';


@NgModule({
  imports:      [ BrowserModule, FormsModule, HttpModule, Ng2BootstrapModule ],
  declarations: [ AppComponent, BlocklyComponent, PrimaryNavComponent,
    DropdownComponent, ModalsComponent, HelpModalComponent, LinkModalComponent,
    IPTooltipComponent, TitleTooltipComponent ],
  bootstrap:    [ AppComponent ],
  providers: [
    CookieService,
    StorageService,
    AlertService
    //{ provide: XHRBackend, useClass: InMemoryBackendService }, // in-mem server
    //{ provide: SEED_DATA,  useClass: InMemoryDataService }     // in-mem server data
  ]
})
export class AppModule { }
