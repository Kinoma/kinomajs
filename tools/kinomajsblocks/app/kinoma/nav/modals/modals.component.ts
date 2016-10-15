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
import { Component, ViewChild } from '@angular/core';
import { CookieService } from 'angular2-cookie/core';
import { ModalDirective } from 'ng2-bootstrap/ng2-bootstrap';

@Component({
  selector: 'modal',
  template: `
    <div bsModal #lgModal="bs-modal" class="modal fade" tabindex="-1" role="dialog" aria-labelledby="mySmallModalLabel" aria-hidden="true">
      <div class="modal-dialog modal-lg">
        <div class="modal-content">
          <div class="modal-header">
            <button type="button" class="close" (click)="lgModal.hide()" aria-label="Close">
              <span aria-hidden="true">&times;</span>
            </button>
            <h4 [ngSwitch]="info_case" class="modal-title">
              <span *ngSwitchCase="'help'"> About KinomaJS Blocks </span>
              <span *ngSwitchCase="'link'"> Save Your Project </span>
            </h4>
          </div>
          <div [ngSwitch]="info_case" class="modal-body">
            <help-modal-content *ngSwitchCase="'help'"></help-modal-content>
            <link-modal-content *ngSwitchCase="'link'"></link-modal-content>
          </div>
        </div>
      </div>
    </div>
  `,
  styleUrls: ['app/kinoma/nav/modals/modals.component.css']
})

export class ModalsComponent {
  info_case:string = 'help';
  @ViewChild('lgModal') public childModal:ModalDirective;
  public showChildModal(modal) {
    this.info_case = modal;
    this.childModal.show();
  }

  public hideChildModal():void {
    this.childModal.hide();
  }

}
