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
// @fileoverview This service will handle the alert messages for various functions
import { Injectable }    from '@angular/core';
import { ReplaySubject } from 'rxjs/ReplaySubject';

/**
 * Present a text message to the user.
 * @param {string} message Text to alert.
 */
 @Injectable()
 export class AlertService{
  private _alertSource = new ReplaySubject<Object>();
  alertInstance$ = this._alertSource.asObservable();
  alert(message,type,close) {
    this._alertSource.next({msg: message, type: type, closable: close});
  };

}
