/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#include "cryptTypes.h"

enum des_cipher_direction {des_cipher_encryption, des_cipher_decryption};
typedef UInt32 des_subkey[32];

extern void des_keysched(const UInt8 *key, enum des_cipher_direction direction, des_subkey subkey);
extern void des_process(const UInt8 *in, UInt8 *out, des_subkey subkey);
