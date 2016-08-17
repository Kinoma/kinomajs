export default class extends Behavior {
	onCreate() {
		super.onCreate(shell, data);
		
		this.clipboard = "";
		
		this.cursorShape = system.cursors.arrow;
		this.hoverContent = null;
		this.hoverFlag = true;
		this.hoverX = -1;
		this.hoverY = -1;
	}
	
/* CLIPBOARD */
	getClipboard() {
		return this.clipboard;
	}
	hasClipboard() {
		return this.clipboard ? true : false;
	}
	setClipboard(text) {
		this.clipboard = text;
	}
	onActivated(shell, activateIt) {
		if (activateIt)
			this.clipboard = system.getClipboardText();
		else
			system.setClipboardText(this.clipboard);
	}
	
/* HOVER */
	onHover() {
		this.onTouchMoved(shell, 0, this.hoverX, this.hoverY, 0);
	}
	onTouchBegan(shell, id, x, y, ticks) {
		this.onTouchMoved(shell, id, x, y, ticks);
		this.hoverFlag = false;
	}
	onTouchEnded(shell, id, x, y, ticks) {
		this.hoverFlag = true;
		this.onTouchMoved(shell, id, x, y, ticks);
	}
	onTouchMoved(shell, id, x, y, ticks) {
		if (this.hoverFlag) {
			var content = shell.hit(x, y);
			if (this.hoverContent != content) {
				this.cursorShape = 0;
				if (this.hoverContent)
					this.hoverContent.bubble("onMouseExited", x, y);
				this.hoverContent = content;
				if (this.hoverContent)
					this.hoverContent.bubble("onMouseEntered", x, y);
				shell.changeCursor(this.cursorShape);
			}
			else if (this.hoverContent)
				this.hoverContent.bubble("onMouseMoved", x, y);
			this.hoverX = x;
			this.hoverY = y;
		}
	}
	onTouchScrolled(shell, touched, dx, dy, ticks) {
		if (Math.abs(dx) > Math.abs(dy))
			dy = 0;
		else
			dx = 0;
		var content = this.hoverContent;
		while (content) {
			if (content instanceof Scroller)
				content.delegate("onTouchScrolled", touched, dx, dy, ticks);
			content = content.container;
		}
	}
};

Files.toPath = function(url) @ "KPR_Files_toPath";
Files.toURI = function(path) @ "KPR_Files_toURI";

shell.changeCursor = function(shape) @ "KPR_shell_changeCursor",
(function() @ "KPR_Shell_patch")();

system.gotoFront = function() @ "KPR_system_gotoFront";

system.getClipboardText = function() @ "KPR_system_getClipboardText",
system.setClipboardText = function(it) @ "KPR_system_setClipboardText",

let buttons;
system.alert = function(dictionary, callback) @ "KPR_system_alert",
system.openDirectory = function(dictionary, callback) @ "KPR_system_openDirectory",
system.openFile = function(dictionary, callback) @ "KPR_system_openFile",
system.saveDirectory = function(dictionary, callback) @ "KPR_system_saveDirectory",
system.saveFile = function(dictionary, callback) @ "KPR_system_saveFile",

system.beginModal = function() @ "KPR_system_beginModal",
system.endModal = function() @ "KPR_system_endModal",

