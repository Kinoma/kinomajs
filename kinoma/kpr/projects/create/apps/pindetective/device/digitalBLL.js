//@module
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

exports.pins = {
	digital: {type: "Digital"}
}

exports.configure = function()
{
	this.digital.init();
}

exports.close = function()
{
	this.digital.close();
}

exports.read = function()
{
	return this.digital.read();
}

exports.write = function(value)
{
	return this.digital.write(value);
}

exports.setDirection = function(direction)
{
	return this.digital.direction = direction;
}