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
import { Component, OnInit, OnDestroy  } from '@angular/core';
import { BlocklyService, blockly, main }        from './blockly.service';
import {Subscription} from 'rxjs/Subscription';
import { CookieService } from 'angular2-cookie/core';
import { StorageService } from './storage.service';
import { AlertService } from '../kinoma/alert.service';
import { AlertModule } from 'ng2-bootstrap/ng2-bootstrap';

@Component({
  selector: 'blockly',
  providers: [ BlocklyService ],
  template: `
    <div id="content_blocks" class="content" >
      <!-- Alerts float at top of block content -->
      <alert id="alert" *ngFor="let alert of alerts;let i = index" [type]="alert.type" dismissible="true" (close)="closeAlert(i)">
        {{ alert.msg }}
      </alert>
    </div>
    <pre id="content_javascript" class="content prettyprint" ></pre>
  `,
  styleUrls: [ 'app/blocklycomponent/blockly.component.css' ]
})

export class BlocklyComponent implements OnInit, OnDestroy {
  public alerts:Array<Object> = [];
  subscription:Subscription;

  private contentVisibility: boolean = false;
  dirty: boolean = false;
  generatedCode: string = '// generated code will appear here';

  constructor(private _blocklyService: BlocklyService, private _cookieService: CookieService,
    private _storageService: StorageService, private alertService: AlertService ){ }

  ngOnInit():void {
    this._blocklyService.finishJavascript();//addins to generated code
    this.subscription = this.alertService.alertInstance$.subscribe(res => this.addAlert(res));
  }
  ngOnDestroy():void {
    this.subscription.unsubscribe();
  }
  jsClick():void {
    this._blocklyService.jsClick(this.contentVisibility);
    this.contentVisibility = !this.contentVisibility;
  }
  ngAfterViewInit():void { // INIT toolbox
    let savedDevice = this._cookieService.get('device')
    if( savedDevice !== main.device && savedDevice !== undefined){//if not default
      main.device = savedDevice;
      this._blocklyService.loadToolbox(savedDevice);
    } else this._blocklyService.loadToolbox(main.device);
  }
  public closeAlert(i:number):void{
    this.alerts.splice(i, 1);
    setTimeout(()=>this._blocklyService.onWorkspaceChange('resize'),0);//strange visual hack
  }
  public addAlert(alert):void{
    this.alerts.push(alert);
    setTimeout(()=>this._blocklyService.onWorkspaceChange('resize'),0);//strange visual hack
  }
}
