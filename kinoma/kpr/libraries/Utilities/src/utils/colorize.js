export let RED = 91;
export let GREEN = 92;
export let YELLOW = 93;
export let BLUE = 94;
export let MAGENTA = 95;
export let CYAN = 96;
export let BOLD = 1;
let END = 0;

function escape(color) {
	return "\x1b[" + color + 'm';
}

export function colored(s, color) {
	return escape(color) + s + escape(END);
}

export default {
	colored,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	BOLD
};

