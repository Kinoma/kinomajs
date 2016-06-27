# manifest.json
Standalone KinomaJS applications are described to the build system in a file named `manifest.json`.  

## Overview

The manifest is a JSON document consisting of entries that describe the builds for all target platforms. There is a global entry that applies to all target platforms, and separate entries to describe unique build instructions for specific platforms.
The manifest contains information for the build, environment variables, extensions to include, debugging information to enable, where to find fonts, and how to package the application. It is used by the build tool `kprconfig6` to set up the build.
Each application has a `manifest.json` file. It is usually located in the top-level directory of an application. To build an application for KinomaJS, you supply the location of the manifest file to `kprconfig6`. For example:
```kprconfig6 -x -m -p android $F_HOME/myapplication/manifest.json
```

The manifest includes the following entries. Features for specific target platforms override or add to the global entry.
*   `xsdebug` and `xsprofile` -- features to enable debug and profiling code
*   `build` -- variables used during the build

*   `environment` -- runtime variables

*   `extensions` -- extensions to include in the build

*   `fonts` -- the name and location of included fonts
*   `info` -- permissions and feature definitions for Android and iOS builds
*   `resources` -- files to copy into the target package

*   `platforms` -- additional variables, extensions, and resources for specific platforms

*   `variants` -- additional variables, extensions, and resources

*   `instrument` -- what instrumentation, if any, is enabled for instrumented builds
## Descriptions of Entries
The examples in this section break down the manifest file
`kinoma/kpr/applications/balls/manifest.json`.

##### `xsdebug`, `xsprofile`
The `xsdebug` and `xsprofile` entries shown below cause debug and profiling code to be included in the application.
```
"xsdebug": {
    "enabled": true,
},
"xsprofile": {
    "enabled": true,
},
```

##### `build`
The `build` entry defines build variables referred to later in the `manifest.json` file. Paths are set here to simplify references to files and are referenced using the `$(variable)` construct.
```
"build": {
    "F_EXTENSIONS": "$(F_HOME)/extensions",
    "F_KINOMA": "$(F_HOME)/kinoma",
    "F_MEDIA_READER": "$(F_KINOMA)/mediareader",
    "KDT_HOME": "$(F_KINOMA)/kpr/projects/kdt",
    "KPL_HOME": "$(F_HOME)/build/linux/kpl",
    "KPR_HOME": "$(F_KINOMA)/kpr",
    "APP_HOME": "$(KPR_HOME)/applications/balls",
},
```
>**Note**: `$(F_HOME)` is predefined and is taken from the host system's environment variable `F_HOME`.
##### `environment`
The `environment` entry contains runtime variables. They are made available to the application, from both C and JavaScript.
In C code, they can be accessed with `FskEnvironmentGet`.
```
const char *value = FskEnvironmentGet("variableName");
```

`FskEnvironmentGet` returns a reference to value, not a copy. The storage is disposed of by `FskEnvironment` and so must not be disposed of by the caller.
From ECMAScript, use the global `getEnvironmentVariable` function.
```
getEnvironmentVariable("variableName");
```

Environment variables can contain other environment variables if they are enclosed in square brackets (`[]`).

```
char *msg = FskEnvironmentApply("my platform is [platform]");
```
The preceding line returns `"my platform is android"` when the target platform is Android. `FskEnvironmentApply` allocates and returns a new memory block. The caller of `FskEnvironmentApply` is responsible for disposing of the block using `FskMemPtrDispose`.
Predefined variables include:
* `[applicationPath]`
* `[arch]`

* `[platform]`
```
"environment": {
    "VERSION": "1.0",
    "NAME": "Balls",
    "NAMESPACE": "com.marvell.kinoma.balls",
    "CA_list": "ca-bundle.crt",
    "httpServerPort": "10000",
    "useSSDP": "1",
    "ssdpExpire": "1800",
    "ssdpSearchAll": "0",
    "ssdpTTL": "4",
    "ssdpByebyeRepeat": "3",
    "hardwarepinsSimulator": "true",
    "modulePath": "[applicationPath]modules/",
    "screenScale": "1",
    "httpPoolSize": "5",
    "httpCacheSize": "197",
    "httpCookiesSize": "197",
    "httpKeychainSize": "197",
    "httpLocalStorageSize": "197",
    "textureCacheSize": "2500000",
    "displayFPS": "0",
    "rotateLeftAccel": "[",
    "rotateRightAccel": "]",
    "useGL": "1",
    "windowStyle": "0",
},
```
Note:
- The `modulePath` field in the example above defines where to find the KinomaJS modules in the target application.

- `applicationPath` is set by KinomaJS to be the full path to the executable file. Multiple paths can be specified, separated by a semicolon. For example:
  ```
  "modulePath": "[applicationPath]modules/;[applicationPath]program/src/",
  ```
  
##### `extensions`
The `extensions` entry defines the extensions to include in the application, enabling KinomaJS to build and link only the minimum necessary functionality for a given application and platform. Extensions are given a name and path that specifies the extension's build instructions (`.mk` file).
```
"extensions": {
    "fsZip": "$(F_EXTENSIONS)/fsZip/fsZip.mk",
    "Crypt": "$(F_EXTENSIONS)/crypt/Crypt.mk",
    "FskSSLAll": "$(F_EXTENSIONS)/ssl/FskSSLAll.mk",
    "FskJPEGDecode": "$(F_EXTENSIONS)/FskJPEGDecode/FskJPEGDecode.mk",
    "FskPNGDecode": "$(F_EXTENSIONS)/FskPNGDecode/FskPNGDecode.mk",
    "mediareader": "$(F_KINOMA)/mediareader/mediareader.mk",
    "kpr": "$(KPR_HOME)/kpr.mk",
},
```

##### `resources`
The `resources` entry defines the destination location in the target build platform for files to copy into the application binary package.
```
"resources": {
    ".": [
        "$(F_HOME)/data/sslcert/ca-bundle.crt",
    ],
    "~": [
    ],
    "fonts": [
	 	"$(F_HOME)/data/fonts/FiraSans-Regular.ttf",
    	"$(F_HOME)/data/fonts/FiraSans-Bold.ttf",
    	"$(F_HOME)/data/fonts/FiraMono-Regular.ttf"
    ]
},
```
`"."` is the destination directory for the target files, relative to the application executable or `[applicationPath]`. In the example above, it copies the file from `$(F_HOME)/data/sslcert/ca-bundle.crt` to the target binary directory. The file name is unchanged.
`"~"` is an exclusion directive: the files or paths it specifies are excluded from the copy. 
`"fonts"` creates a destination directory named `fonts` and copies the specified TrueType files into that directory. Note that this copy directive is different from the `fonts` entry (described next); `"fonts"` here (in the `resources` entry) is the name of the directory to create.
##### `fonts`
The `fonts` entry specifies the default font and the path to the additional `fonts` directory. 
The Android, Mac OS X, iOS, and Windows platforms allow the default font to be selected from the installed system fonts.
Linux and its variants use FreeType to render fonts. FreeType requires the application to specify the font files and their location; there are no pre-installed system fonts. Specify which files to copy in the `resources` entry (described above).
```
"fonts": {
    "default": "Fira Sans",
    "path": "[applicationPath]fonts/"
},
```

##### `info`
The `info` entry is used for Android and iOS to specify platform permissions and feature definitions. It is usually only found in the `platforms` entry for Android and iOS.
```
  "info": {
    "permissions": [
      "ACCESS_WIFI_STATE",
      "READ_PHONE_STATE"
    ]
  },
```
##### `platforms`
The `platforms` entry extends and overrides the global entry for a particular platform. `build`, `resources`, `extensions`, `environment`, and `info` directives can be included, excluded, or overridden.
```
"ios": {
  "build": {
    "DEVICE": true
  },
  "environment": {
    "debugger": "192.168.0.4"
  },
  "info": {
    "CFBundleDisplayName": "Basic-dialog"
  }
},
```
##### `variants`
Similar to the `platforms` entry, the `variants` entry allows specification and modification of other entries. The selection is based on a build variable rather than a particular platform, so the `variants` entry is useful for grouping specification and modification common to a variety of platforms.
```
"variants": {
    "$(DEVICE)": {
        "environment": {
            "shellPath": "[applicationPath]program/deviceShell.xsb",
        },
        "resources": {
            "program": [
                "$(BALLS_HOME)/deviceShell.xml",
                "$(BALLS_HOME)/src",
            ],
        },
    },
    "$(SIMULATOR)": {
        "environment": {
            "shellPath": "[applicationPath]program/mockupShell.xsb",
        },
        "resources": {
            "modules": [
                "$(KPR_HOME)/simulator/modules/fingerprint.png",
                "$(KPR_HOME)/simulator/modules/mockup",
            ]
            "program": [
                "$(KPR_HOME)/simulator/devices",
                "$(BALLS_HOME)/mockupShell.xml",
                "$(BALLS_HOME)/src",
            ],
        },
    },
```
##### `instrument`
KinomaJS has an instrumentation facility to assist in the development and debugging of the KPL, Fsk, and KPR layers. Instrumentation is enabled by the build-line option `-i` for debug builds.
```
kprconfig6 -x -m -d -i .../manifest.json
```
Individual instrumentation types are enabled in the `instrument` entry of an application's `manifest.json` file.
```
"instrument": {
    "log": "",
    "syslog": "",
    "trace": true,
    "threads": true,
    "times": true,
    "androidlog": true,
    "kinds": [
    	{ "type": "KprLayer", "messages": "debug" },
    	{ "type": "KprText", "messages": "normal" },
    	{ "type": "KprStyle", "messages": "verbose" },
    	{ "type": "freetype", "messages": "minimal" },
    ]
}
```

See the [Instrumentation](https://github.com/Kinoma/kinomajs/blob/master/doc/Instrumentation.md) document for details.
