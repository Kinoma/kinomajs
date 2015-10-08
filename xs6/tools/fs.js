/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
export function closeSync(fd) @ "fs_closeSync";
export function copyFileSync(from, to) @ "fs_copyFileSync";
export function deleteDirectory(path) @ "fs_deleteDirectory";
export function deleteFile(path) @ "fs_deleteFile";
export function existsSync(path) @ "fs_existsSync";
export function mkdirSync(path) @ "fs_mkdirSync";
export function readDirSync(path) @ "fs_readDirSync";
export function readFileSync(path) @ "fs_readFileSync";
export function openSync(path) @ "fs_openSync";
export function writeFileSync(path, string) @ "fs_writeFileSync";
export function writeSync(fd, string) @ "fs_writeSync";
