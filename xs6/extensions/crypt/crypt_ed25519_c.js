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
export default class Ed25519 @ "xs_ed25519_destructor" {
	constructor() @ "xs_ed25519_constructor";
	getPK(sk) @ "xs_ed25519_getPK";
	sign(msg, sk, pk) @ "xs_ed25519_sign";
	verify(msg, signature, pk) @ "xs_ed25519_verify";
};
