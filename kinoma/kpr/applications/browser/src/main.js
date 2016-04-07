//@program 
/* Skins and Styles */ 
var greySkin = new Skin ({fill: '#cccccc'}); 
var whiteSkin = new Skin ({fill: 'white'}); 
var yellowSkin = new Skin ({fill: 'yellow'}); 
var blackStyle = new Style ({ font: '22px', color: 'black', horizontal: 'center', vertical: 'middle' }); 
var whiteStyle = new Style ({ font: 'bold 22px', color: 'white', horizontal: 'center', vertical: 'middle' }); 

/* UI Templates */ 
var ButtonTemplate = Label.template(function ($) { return {
	top: 5, bottom: 5, left: 5, right: 5, skin: greySkin, style: whiteStyle, active: true, string: $.string,
	behavior: Behavior({
		onCreate: function(container, data) {
			this.data = data;
		},
		onTouchEnded: function(container, id, x, y, ticks) {
			application.distribute("onDisplayUpdate", this.data.string);
		}
	})
}})
var ButtonTemplate2 = Label.template(function ($) { return {
	top: 5, bottom: 5, left: 5, right: 5, skin: greySkin, style: whiteStyle, active: true, string: $.string,
	behavior: Behavior({
		onCreate: function(container, data) {
			this.data = data;
		},
		onTouchEnded: function(container, id, x, y, ticks) {
			//application.distribute("onDisplayUpdate", this.data.string);
			application.first.add(new OAuthContainer({}));
		}
	})
}})
/* Screen layout */ 
var mainContainer = Container.template(function ($) { return {
	left: 0, right: 0, top: 0, bottom: 0, skin: whiteSkin,
	contents: [
		Label($, {top: 0, left: 0, right: 0, height: 35, style: blackStyle,
			behavior: Behavior({
				onDisplayUpdate: function(container, string) {
					container.string = container.string + string;
				}
			})
		}),
		Container($, {top: 35, left: 0, right: 0, bottom: 0, skin: yellowSkin,
			contents: [
				Column($, {top: 0, right: 0, left: 0, bottom: 0,
					contents: [
						Line($, {top: 0, right: 0, left: 0, bottom: 0,
							contents: [
								new ButtonTemplate({string: '1'}),
								new ButtonTemplate({string: '2'}),
								new ButtonTemplate({string: '3'}),
							]
						}),
						Line($, {top: 0, right: 0, left: 0, bottom: 0,
							contents: [
								new ButtonTemplate({string: '4'}),
								new ButtonTemplate({string: '5'}),
								new ButtonTemplate({string: '6'}),
							]
						}),
						Line($, {top: 0, right: 0, left: 0, bottom: 0,
							contents: [
								new ButtonTemplate({string: '7'}),
								new ButtonTemplate({string: '8'}),
								new ButtonTemplate({string: '9'}),
							]
						}),
						Line($, {top: 0, right: 0, left: 0, bottom: 0,
							contents: [
								new ButtonTemplate({string: '*'}),
								new ButtonTemplate({string: '0'}),
								new ButtonTemplate2({string: '#'}),
							]
						}),
					]
				})
			]
		})
	]
}});

var OAuthContainer = Container.template(function($) { return {
	left: 0, right: 0, top: 0, bottom: 150, skin: greySkin, active: true,
	contents: [
		new Label({top: 0, string: "CLOSE", style: blackStyle}),
		new Browser({top: 50, left: 0, right: 0, bottom: 0}, "http://kinoma.com")
	],
	behavior: Behavior({
		onTouchEnded: function(label) {
			label.container.last.visible = false;
			label.container.remove(label.container.last);
			label.container.container.remove(label.container);
			//application.first.remove(application.first.last);
		}
	})
}})

/* Application definition */ 
application.behavior = {
	onLaunch: function() {
        var data = {};
		application.add(new mainContainer(data));
	}
}
