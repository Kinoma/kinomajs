//@module

exports.pins = {
	power: {type: "Power"}
}

exports.configure = function()
{
	this.power.init();
}

exports.close = function()
{
	this.power.close();
}
