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
import { Subscription } from 'rxjs/Subscription';
import { StorageService } from '../../../blocklycomponent/storage.service';

@Component({
  selector: 'link-modal-content',
  template: `
    <div class="link-modal">
      Copy the link below to re-open this project at current state.
      <br/>
      <input id='link'type="text" [value]="'http://kinomablockly.appspot.com/static/index.html#'+hashUrl" readonly>
      <br/>
      Or you can bookmark the link.
      <br/><br/>
      <a id="bookmark-this" href="#" title="Bookmark">Bookmark</a>
     </div>
    `,
    styleUrls: ['app/kinoma/nav/modals/modals.component.css']
})

export class LinkModalComponent {
  subscription:Subscription;
  private hashUrl = '#';
  constructor(private _storageService: StorageService){
    this.subscription = this._storageService.saveInstance$.subscribe(res => this.hashUrl = res.toString());
  }
}
