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

export default {
	timeSyncURL: "http://service.cloud.kinoma.com/includes/time-stamp.inc",
	pk8Password: "pk8 password",	// set the password to encrypt private keys in pkcs8
	envKey: "0123456789012345",	// to encrypt sensitive data such as wifi password on flash
	telnet: true,		// enable telnet
	tftp: true,		// enable tftp
	kinomaStudio: true,	// enable KinomaStudio
	setup: true,		// enable the setup service
	mfiPins: undefined,	// set I2C pins if the MFi chip is present, like {scl: 19, sda: 18}
};
