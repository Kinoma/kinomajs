// pushd $F_HOME/xs6/xsedit/features/documentation
// $F_HOME/xs6/bin/mac/debug/xsr6 -a $F_HOME/xs6/bin/mac/debug/modules/tools.xsa docs/index.js

import * as FS from "fs";
import TOOL from "tool";

let directory = "./docs";
let tool = new TOOL(process.argv);
tool.currentDirectory = directory;

function clean(input, output) {
	trace(input + " " + output + "\n");
	let document = FS.readFileSync(input);
	if (document) {
		document = document.replace(/\r(\n)?/g, "\n").trim() + "\n";
		FS.writeFileSync(output || input, document);
	}
	else throw "error: " + input;
}

let files = [];
let names = FS.readDirSync(".");
names.forEach(name => {
	if (!name.startsWith(".")) {
		if (name.endsWith(".js")) {
			// continue
		}
		else if (name === "index.md") {
			files.unshift(`${directory}/${name}`);
			clean(`${name}`);
		}
		else if (name === "technotes") {
			var technotes = FS.readDirSync(name);
			technotes.forEach(technote => {
				if (!technote.startsWith(".")) {
					files.push(`${directory}/${name}/${technote}/index.md`);
					clean(`${name}/${technote}/${technote}.md`, `${name}/${technote}/index.md`);
				}
			});
		}
		else {
			if (name === "files-api") {
				files.push(`${directory}/${name}/${name}.md`);
				clean(`${name}/filesapi.md`, `${name}/${name}.md`);
			}
			else {
				files.push(`${directory}/${name}/${name}.md`);
				clean(`${name}/${name}.md`);
			}
		}
	}
});
trace(JSON.stringify({ files, force:false }, null, "\t") + "\n");

// END OF FILE
