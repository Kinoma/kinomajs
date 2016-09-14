<!-- Version: 160418-CR / Primary author: Brian Friedkin / Last reviewed: December 2015

This document summarizes and illustrates the KinomaJS Files API, which enables applications to access the host file system to read and write files, iterate through directories, rename files, and more.
-->

#Accessing Files from KinomaJS

The KinomaJS Files API provides easy, platform-independent access to the host file system, enabling applications to read and write files, iterate through directories, rename files, and more. This document summarizes and illustrates the functions and properties used in performing these operations. They are all accessed from the global `Files` object.

>**Note:** The [`files-buffers`](https://github.com/Kinoma/KPR-examples/tree/master/files-buffers) sample app also demonstrates these operations.

##Background

###Pathnames

Pathnames in KinomaJS use the [file URI scheme](https://en.wikipedia.org/wiki/File_URI_scheme) for both files and directories. 

Pathnames corresponding to directories must include a trailing slash, as in `file://path/to/directory/`. If a pathname does not include a trailing slash, it corresponds to a file, as in `file://path/to/file.txt`.

###Predefined Directories

The `Files` object enables access to predefined directories on the host file system. These directories contain specific types of files and are accessible through the following properties:

* `documentsDirectory` stores general-purpose document files that persist across application launches.

* `picturesDirectory` corresponds to the host directory where picture files are stored.

* `preferencesDirectory` stores application preference files that persist across application launches.

* `temporaryDirectory` stores temporary files, which do not persist across application launches. This directory on Kinoma Create lives on a RAM disk that is erased on power cycles and reboots.

The KinomaJS global `mergeURI` function is used to construct a URI from a base URL and a partial URL. The partial URL is merged with the base URL to arrive at the full URI. In the example below, we use `mergeURI` to construct full URIs from predefined directories.

```
// Write the my_app_prefs.json file to the host preferences directory.
var uri = mergeURI(Files.preferencesDirectory, "my_app_prefs.json");
Files.writeJSON(uri, {color: "green"});

// Write the string "Hello world" into the hello.txt file contained in
// the host documents directory.
var uri = mergeURI(Files.documentsDirectory, "hello.txt");
var text = "Hello world";
Files.writeText(uri, text);
```

##Reading and Writing Files

The functions available through the `Files` object can be used to read and write a variety of data types, often eliminating the need for the application to convert the data after reading or before writing. Unless indicated otherwise, the functions that write will overwrite any existing file.

> **Note:** These functions throw exceptions upon failure, so you are advised to bracket them with `try`/`catch` blocks when there is the potential for errors.

#####`readText`, `writeText`

Use `readText` and `writeText` to read and write `String` data.

```
var uri = mergeURI(application.url, "assets/license.txt");
var licenseText = Files.readText(uri);
var textContainer = new Text( {left: 0, right: 0, string: licenseText });

var text = container.field.string;
var uri = mergeURI(Files.preferencesDirectory, "input.txt");
Files.writeText(uri, text);
```

#####`appendText`

Use `appendText` to append `String` data to an existing file.

```
var uri = mergeURI(Files.temporaryDirectory, "hello.txt");
Files.writeText(uri, "Hi");
Files.appendText(uri, " There");  // File contains the text "Hi There"
```

#####`readBuffer`, `writeBuffer`

Use `readBuffer` and `writeBuffer` to read and write `ArrayBuffer` data.

```
// Validate PNG file by looking for "PNG" chars in header bytes 1-3.
var uri = mergeURI(application.url, "assets/balls.png");
var buffer = Files.readBuffer(uri);
var data = new Uint8Array(buffer);
if (data[1] != 0x50 || data[2] != 0x4E || data[3] != 0x47)
	throw("Invalid PNG file!");

// Write the photo.jpg asset file to the host pictures directory.
var uri = mergeURI(application.url, "assets/photo.jpg");
var buffer = Files.readBuffer(uri);
var uri = mergeURI(Files.picturesDirectory, "photo.jpg");
Files.writeBuffer(uri, buffer);
```

#####`appendBuffer`

Use `appendBuffer` to append `ArrayBuffer` data to an existing file.

```
// Write buffer1 followed by buffer2 into the data/results file in 
// the host documents directory.
var uri = mergeURI(Files.temporaryDirectory, "test_results_1");
var buffer1 = Files.readBuffer(uri);
var uri = mergeURI(Files.temporaryDirectory, "test_results_2");
var buffer2 = Files.readBuffer(uri);
var uri = mergeURI(Files.documentsDirectory, "data/results");
Files.writeBuffer(uri, buffer1);
Files.appendBuffer(uri, buffer2);
```

#####`readJSON`, `writeJSON`

Use `readJSON` and `writeJSON` to read and write JavaScript objects.

```
var preferences = { name: "Brian", city: "Del Mar", food: "tacos" };
var filename = "user.json";
var uri = mergeURI(Files.preferencesDirectory, application.di + "." + filename);
Files.writeJSON(uri, preferences);
var user = Files.readJSON(uri);
trace(user.name + " likes to eat " + user.food + "!");
```

#####`readXML`, `writeXML`

Use `readXML` and `writeXML` to read and write XML DOM data.

```
var uri = mergeURI(application.url, "./assets/person.xml");
var document = Files.readXML(uri);
var items = document.getElementsByTagName("person");
if (items.length > 0) {
	var node = items.item(0);
	var name = node.getAttribute("name");
	if (!name || (name != "Brian"))
		throw("Unexpected person!");
}

var uri = mergeURI(Files.documentsDirectory, "empty.html");
var document = DOM.implementation.createDocument("", "html");
var html = document.documentElement;
html.setAttribute("lang", "en");
var head = document.createElement("head");
html.appendChild(head);
var meta = document.createElement("meta");
head.appendChild(meta);
var title = document.createElement("title");
head.appendChild(title);
var body = document.createElement("body");
html.appendChild(body);
Files.writeXML(uri, document);
```

##Directory and File Utilities

The `Files` object also provides a number of common file and directory utility functions.

#####`deleteFile`, `deleteDirectory`

Use `deleteFile` and `deleteDirectory` to delete files and directories. The `deleteDirectory` function requires the directory to be empty. To recursively delete a directory tree and its contained files, set `deleteDirectory`'s optional second parameter.

```
// Delete the "download.txt" file from the host temporary directory.
var uri = mergeURI(Files.temporaryDirectory, "download.txt");
Files.deleteFile(uri);

// Delete the empty "kinoma" directory from the host preferences directory.
var uri = mergeURI(Files.preferencesDirectory, "kinoma/");
Files.deleteDirectory(uri);

// Delete the "kinoma" directory tree from the host documents directory.
var uri = mergeURI(Files.documentsDirectory, "kinoma/");
Files.deleteDirectory(uri, true);
```

#####`ensureDirectory`

Use `ensureDirectory` to ensure that a directory exists. The directory (tree) is created if it does not already exist.

```
// Ensure there's a kinoma/apps directory in the host preferences directory.
var uri = mergeURI(Files.preferencesDirectory, "kinoma/apps/");
Files.ensureDirectory(uri);
```

#####`exists`

The `exists` function tests whether a file or directory exists.

```
if (Files.exists("file:///log.txt"))
	trace("The log.txt file is in the root directory");

// Clear out the "destination" directory.
if (Files.exists(destination)) {
	Files.deleteDirectory(destination, true);
	Files.ensureDirectory(destination);
}
```

#####`renameFile`, `renameDirectory`

Use `renameFile` and `renameDirectory` to rename files and directories. Note that these functions are never used to move a file or directory to another volume or directory.

```
// Rename the "log.txt" file in the kinoma/data directory to "data.txt".
var uri = mergeURI(Files.documentsDirectory, "kinoma/data/log.txt");
Files.renameFile(uri, "data.txt");

// Rename the "data" directory in the "kinoma" directory to "results".
var uri = mergeURI(Files.documentsDirectory, "kinoma/data/");
Files.renameDirectory(uri, "results");
```

#####`getInfo`

The `getInfo` function provides information about a file or directory. The `info` object returned contains the following properties:

*	`type` -- `Files.directoryType` for directories, `Files.fileType` for files	
* 	`created` -- File creation date in milliseconds from the epoch

* 	`date` -- File modification date in milliseconds from the epoch (when available)

* 	`size` -- File size in bytes

```
var uri = mergeURI(Files.picturesDirectory, "photo.jpg");
var info = Files.getInfo(uri);
if (info.type != Files.fileType)
	throw("photo.jpg is not a file!");
trace("photo.jpg file size is " + info.size + " bytes and was created on " + new Date(info.created));
```

##Directory Iterator

Finally, the `Files` object provides functions for iterating through directories. 

Create the directory iterator using the `Files.Iterator` constructor, and then call `iterator.getNext` repeatedly to iterate through the contents of the directory. The `info` object returned contains a `path` property corresponding to the name of the current iterated item.

```
// Recursively iterate through the kinoma/apps directory.
var iterateDirectory = function(path) {
	var info, iterator = new Files.Iterator(path);
	while (info = iterator.getNext()) {
		trace(path + info.path + "\n");
		if (Files.directoryType == info.type)
			iterateDirectory(path + info.path + "/");
	}
}

var path = mergeURI(Files.preferencesDirectory, "kinoma/apps/");
iterateDirectory(path);
```