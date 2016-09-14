/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */
export default class Poly1305 @ "xs_poly1305_destructor" {
	constructor(key) @ "xs_poly1305_constructor";
	update(m) @ "xs_poly1305_update";
	close() @ "xs_poly1305_close";
	get keySize() @ "xs_poly1305_keySize";
	get outputSize() @ "xs_poly1305_outputSize";
};
