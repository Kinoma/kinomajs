//@module

exports.pins = {
	ground: {type: "Ground"}
}

exports.configure = function()
{
	this.ground.init();
}

exports.close = function()
{
	this.ground.close();
}
