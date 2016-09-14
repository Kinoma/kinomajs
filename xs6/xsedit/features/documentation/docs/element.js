// pushd $F_HOME/xs6/xsedit/features/documentation
// $F_HOME/xs6/bin/mac/debug/xsr6 -a $F_HOME/xs6/bin/mac/debug/modules/tools.xsa docs/element.js

import * as FS from "fs";
import TOOL from "tool";

let tool = new TOOL(process.argv);
tool.currentDirectory = "./docs/element";

let docOrder = FS.readFileSync("doc-order.txt");
if (docOrder) {
	let document = "";
	let items = JSON.parse(docOrder.replace(/'/g, "\""));
	items.forEach(item => {
		trace("# " + item + "\n");
		let section = FS.readFileSync(item);
		if (section) {
			section = section.replace(/\r(\n)?/g, "\n");
			section = section.trim() + "\n\n";
			document += section;
		}
		else throw "error: " + item;
	});
	FS.writeFileSync("element.md", document);
}
else throw "error";

// END OF FILE
