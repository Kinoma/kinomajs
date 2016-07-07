# KinomaJS Open Source Build README
The build system for KinomaJS open source software supports building for Mac OS X, iOS, Android, Linux, and Windows targets. The build system runs on Mac OS X (Yosemite and later), Windows (7+) and Linux (Ubuntu 14.04). This document describes the host build environment required to build KinomaJS for each supported target platform, and the commands used to build an application.## Overview
KinomaJS applications are defined by a `manifest.json` file that contains the build options, a list of the extensions required by the application, and packaging instructions.
The `kprconfig6` tool generates build instructions from the application's `manifest.json` file. `kprconfig6` generates instructions for a command-line build for all targets, and an IDE project file for Windows, Mac OS X, and iOS.
The steps for building are:
1. Set up your host build environment.

2. Build the XS6 tools.
3. Build your KinomaJS application.
The host build environment is set up on the computer used to perform the build.

* Building KinomaJS for Mac OS X, iOS, Android, Kinoma Element (MW302), or the Kinoma Code IDE requires a Mac OS X build host.
 
* Building KinomaJS for Linux (for example, GTK desktop, Kinoma Create, or embedded Linux platforms) requires a Linux build host.
 
* Building KinomaJS for Windows requires a Windows build host.
The host build environment is set up once. Instructions for setting up the host build environment for each target platform are provided below.
The XS6 tools built in the second step interpret the `manifest.json` file that defines an application to generate build instructions. 

## kprconfig6 Tool

The XS6 tool used to build applications is `kprconfig6`.

### Using kprconfig6
To invoke `kprconfig6` to build a KinomaJS application, use:
```
kprconfig6 -x -m -p <platform> <path-to-manifest.json>```

To build a debug version of the application, use:
```
kprconfig6 -d -x -m -p <platform> <path-to-manifest.json>
```where `<platform>` identifies the target platform as indicated in the sections that follow (for example, `mac` for Mac OS X or `win` for Windows).
The package created by the build is located at `{F_HOME}/bin/<platform>/<build-type>/<app-name>.tgz` (or `.app`, `.ipa`, `.apk`, and so on, depending on the target platform).
Use `-clean` to clean the target application and platform. 
```
kprconfig6 -x -m -p <platform> <path-to-manifest.json> -clean```

Use `-cleanall` to clean all applications and all platforms.
```
kprconfig6 -cleanall
```
### kprconfig6 Options
#### General Options
- `-a`:  Application name
- `-p`:  Platform name
- `-v`:  Verbose
- `-clean`:  Clean the build for a particular application/platform
- `-cleanall`:  Clean the build for all platforms/applications
#### Debug Options
- `-c`:  Debug or release configuration
- `-d`:  Debug build (same as `-c debug`)
- `-i`:  Enable instrumentation
- `-l`:  Debug memory leaks
- `-X`:  Support xsdebug
#### Build Options
- `-I`:  Use the CMake-generated IDE (sets `-x`)
- `-g`:  Sets CMake generator (build tool used)
- `-m`:  Build the application after generating the make files
- `-x`:  Use CMake build
#### iOS Options
- `-csi`:  Code signing identity to use on iOS
- `-pp`:  Provisioning profile for iOS
#### Output Options
- `-b`:  `bin` directory
- `-o`:  Output directory to place default `bin` and `tmp` directories
- `-t`:  `tmp` directory
## Mac OS X Target
![icon](http://kinoma.com/img/github/osx.png)
Building KinomaJS for Mac OS X uses a computer running Mac OS X as the build host.
On Mac OS X, the KinomaJS build uses Homebrew to manage installation and maintenance of CMake.
### Set Up Your Host Build Environment
1. Install the current version of Xcode from Apple's App Store. (As of the writing of this document, the current version is 7.3.) After installing, run Xcode to accept the license agreement and install any required additional components.
2. Install [Homebrew](http://brew.sh/).
  ```
  $ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"  $ brew install caskroom/cask/brew-cask
  ```

3. Install CMake.
  ```
  $ brew install cmake
  ```
4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```
5. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6  ```

### Build the XS6 Tools
1. Build the XS6 tools as follows:

  ```
  $ cd ${XS6}  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  $ export PATH=${PATH}:${XS6}/bin/mac/Release
  ```
3. *(Optional)* Build `xsbug`, a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  $ kprconfig6 -p mac -x -m ${XS6}/xsbug/manifest.json
  ```
	The `xsbug` application is located at `${F_HOME}/bin/mac/Release/xsbug.app`.
### Build Your KinomaJS Application
Build the `balls` sample application that is included in the KinomaJS repository.
```
$ cd ${F_HOME}$ kprconfig6 -x -m -p mac ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
```

The `balls` application is located at `${F_HOME}/bin/mac/Release/Balls.app`.

## Linux/GTK Target
![icon](http://kinoma.com/img/github/linux.png)
Building KinomaJS for Linux/GTK requires a Linux build host. We use Ubuntu 14.04 on VirtualBox (on Mac OS X); other similar distributions have also been successfully used.
### Set Up Your Host Build Environment
1. Install the developer tools and CMake.
  ```
  $ sudo apt-get install build-essential checkinstall  $ sudo apt-get install libgtk-3-dev libasound2-dev zlib1g-dev  $ sudo apt-get install cmake
  ```
2. If you are using a 64-bit Linux distribution as a build host, install 32-bit libraries.
  ```
  $ sudo apt-get install zlib1g:i386 lib32stdc++6
  ```
  
3. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```
4. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6
  ```
  
### Build the XS6 Tools
1. Build the XS6 tools as follows:

  ```
  $ cd ${XS6}  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.

  ```
  $ export PATH=${PATH}:${XS6}/bin/linux/$(uname -m)/Release
  ```    
3. *(Optional)* Build `xsbug`, a graphical debugger used as an alternative to the Kinoma Studio debugger.

  ```
  $ kprconfig6 -p linux/gtk -x -m ${XS6}/xsbug/manifest.json
  ```
	The `xsbug` application is located at `${F_HOME}/bin/linux/gtk/Release/xsbug/xsbug`.
### Build Your KinomaJS Application
Build the `balls` sample application that is included in the KinomaJS repository.
```
$ cd ${F_HOME}
$ kprconfig6 -x -m -p linux/gtk ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
```The `balls` application is located at `${F_HOME}/bin/linux/gtk/Release/Balls/Balls`.
## Windows Target
![icon](http://kinoma.com/img/github/windows.png)
Building KinomaJS for Windows builds uses a computer running Windows as the build host.
Building KinomaJS for Windows requires a machine running Windows 7 or greater. It is built with the Visual Studio development tools. We have used Visual Studio 2013 Express and Visual Studio 2015 Express.
### Set Up Your Host Build Environment1. Install the developer tools (Visual Studio Express):

	1. Download Visual Studio Express from the [Visual Studio Community download page](https://www.visualstudio.com/products/visual-studio-community-vs).
	2. Select the **Custom** install method to install the following packages:
       - Common Tools for Visual C++ 2015
	   - Windows XP Support for C++
	   - Git for Windows

2. Install CMake: on the [CMake download page](http://www.cmake.org/download/), click the "Windows (Win32 Installer)" link.
3. Update the `PATH` environment variable to include the paths for Git and CMake:

	- Windows 32-bit -- `C:\Program Files\Git\cmd;C:\Program Files\CMake\bin`
	- Windows 64-bit -- `C:\Program Files (x86)\Git\cmd;C:\Program Files (x86)\CMake\bin`
4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  > git clone https://github.com/kinoma/kinomajs.git
  ```
5. Set up two environment variables to point to the source tree. Use

  **Start** > **Control Panel** > **System** > **Advanced system settings** > **Advanced** > **Environment Variables**
   
	to set the following:
	 - `F_HOME=\path\to\kinomajs` (example: `F_HOME=c:\Users\me\kinomajs`)

	 - `XS6=\path\to\kinomajs\xs6` (example: `XS6=c:\Users\me\kinomajs\xs6`)

### Build the XS6 Tools
1. Launch a Visual Studio Command Prompt.
	**Start** > **All Programs** > **Visual Studio Command Prompt (2013)**
2. Build the XS6 tools.
  ```
  > cd %XS6%  > mkdir tmp  > cd tmp  > cmake %XS6%  > cmake --build . --config Release
  ```
3. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  > set PATH=%PATH%;%XS6%\bin\win\Release
  ```       
4. *(Optional)* Build `xsbug`, a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  > kprconfig6 -p win -x -m %XS6%\xsbug\manifest.json
  ```
	The `xsbug` application is located at `%F_HOME%\bin\win\Release\xsbug\xsbug.exe`.
### Build Your KinomaJS Application
Build the `balls` sample application that is included in the KinomaJS repository.
```
$ cd %F_HOME%$ kprconfig6 -x -m -p win %F_HOME%\kinoma\kpr\applications\balls\manifest.json
```
The `balls` application is located at `%F_HOME%\bin\win\Release\balls\balls.exe`.

## iOS Target
![icon](http://kinoma.com/img/github/iOS.png)
iOS applications are built on a Mac OS X build host. If you have already set up your Mac OS X build host and XS6 tools, skip to the [Build Your KinomaJS Application](#ios-build) section.
On Mac OS X, the KinomaJS build uses Homebrew to manage installation and maintenance of CMake.
### Set Up Your Mac OS X Host Build Environment
1. Install the current version of Xcode from Apple's App Store. (As of the writing of this document, the current version is 7.3.) After installing, run Xcode to accept the license agreement and install any required additional components.
2. Install [Homebrew](http://brew.sh/).
  ```
  $ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"  $ brew install caskroom/cask/brew-cask
  ```
3. Install CMake.
  ```
  $ brew install cmake
  ```
4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```
5. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6
  ```

### Build the XS6 Tools
1. Build the XS6 tools as follows:

  ```
  $ cd ${XS6}  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  $ export PATH=${PATH}:${XS6}/bin/mac/Release
 ```
3. *(Optional)* Build `xsbug`, a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  $ kprconfig6 -p mac -x -m ${XS6}/xsbug/manifest.json
  ```
  
	The `xsbug` application is located at `${F_HOME}/bin/mac/Release/xsbug.app`.

<a id="ios-build"></a>
### Build Your KinomaJS Application
Build the `balls` sample application that is included in the KinomaJS repository.

```
$ cd ${F_HOME}$ kprconfig6 -x -m -p ios ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
```
The `balls` application is located at `${F_HOME}/bin/ios/Release/Balls.ipa`.
If you have multiple developer accounts, you will be warned to include a codesign switch on the command line. To list the certificates you have installed, use:
```
$ security find-identity -v -p codesigning
```
Then add the switch `-csi` to your build line with the ID of the certificate to use.
```
$ kprconfig6 -x -m -p ios -csi <ID> ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
```
## Android Target
![icon](http://kinoma.com/img/github/android.png)
Building KinomaJS for Android uses Mac OS X as the build host.
In addition to the Mac OS X build host setup, KinomaJS for Android requires that you install the Android NDK and SDK.
On Mac OS X, the KinomaJS build uses Homebrew to manage installation and maintenance of CMake, Java, the Android SDK, and the Android NDK.

### Set Up Your Mac OS X Host Build Environment and Android Tools
1. Install the current version of Xcode from Apple's App Store. (As of the writing of this document, the current version is 7.3.) After installing, run Xcode to  accept the license agreement and install any required additional components.
2. Install [Homebrew](http://brew.sh/).
  ```
  $ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"  $ brew install caskroom/cask/brew-cask
  ```
3. Install CMake.
  ```
  $ brew install cmake
  ```
4. Install Java and the Android SDK and NDK.
  ```
  $ brew cask install java  $ brew install android-sdk android-ndk
  ```      
5. Set up two environment variables to point to the Android SDK and Android NDK install locations.
  ```
  $ export ANDROID_SDK=/usr/local/opt/android-sdk  $ export ANDROID_NDK=/usr/local/opt/android-ndk
  ```
6. Update the Android tools.
  ```
  $ android update sdk -u -t platform-tools  $ android update sdk -u -t android-17  $ android update sdk -u -t $(android list sdk -e | grep build-tools | sed -ne 's/.*"\(.*\)".*/\1/p' | head -1)
  ```
7. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```
8. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6
  ```
### Build the XS6 Tools
1. Build the XS6 tools as follows:

  ```
  $ cd ${XS6}  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  $ export PATH=${PATH}:${XS6}/bin/mac/Release
  ```
3. *(Optional)* Build `xsbug`, a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  $ kprconfig6 -p mac -x -m ${XS6}/xsbug/manifest.json
  ```
	The `xsbug` application is located at `${F_HOME}/bin/mac/Release/xsbug.app`.

### Build Your KinomaJS Application
1. Build the `balls` sample application that is included in the KinomaJS repository.
  ```
  $ cd ${F_HOME}  $ kprconfig6 -x -m -p android ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
  ```
	The `balls` application `.apk` file is located at `$F_HOME/bin/android/Balls/Release/balls/balls.app`.
2. Transfer to your Android device and run.
  ```
  $ adb -d install -r ${F_HOME}/bin/android/Release/Balls/Balls.apk
  ```

## Kinoma Create Target (Linux/Aspen)
![icon](http://kinoma.com/img/github/create.png)
Building KinomaJS for Linux/Aspen uses a Linux build host.
The build system uses `linux/aspen` as the target platform to build Kinoma Create.
### Set Up Your Host Build Environment
1. Install the developer tools and CMake.
  ```
  $ sudo apt-get install build-essential checkinstall  $ sudo apt-get install libgtk-3-dev libasound2-dev zlib1g-dev  $ sudo apt-get install cmake
  ```      
2. For a 64-bit Linux, install 32-bit libraries.
  ```
  $ sudo apt-get install zlib1g:i386 lib32stdc++6
  ```
  
3. Install the `linux/aspen` toolchain and `sysroot`.
  ```
  $ mkdir /your/buildtools/path  $ cd /your/buildtools/path  $ wget http://downloads.kinoma.com/aspen/sysroot.tbz  $ tar -jxvf sysroot.tbz  $ wget http://downloads.kinoma.com/aspen/toolchain.tbz  $ tar -jxvf toolchain.tbz
  ```
4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```
5. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6
  ```
6. Set up two environment variables to the linux/aspen cross-compiler and `sysroot`.
  ```
  $ export ARM_MARVELL_LINUX_GNUEABI=/your/buildtools/path/arm-marvell-linux-gnueabi  $ export FSK_SYSROOT_LIB=/your/buildtools/path/arm-bin
  ```

### Build the XS6 Tools
1. Build the XS tools as follows:

  ```
  $ cd ${XS6}  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  $ export PATH=${PATH}:${XS6}/bin/linux/$(uname -m)/Release
  ```
3. *(Optional)* Build the `xsbug` debugger: `xsbug` is a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  $ kprconfig6 -p linux/gtk -x -m ${XS6}/xsbug/manifest.json
  ```
	The `xsbug` application is located at `${F_HOME}/bin/linux/gtk/Release/xsbug/xsbug`.
### Build Your KinomaJS Application
1. Build the `balls` sample application that is included in the KinomaJS repository.
  ```
  $ cd ${F_HOME}  $ kprconfig6 -x -m -p linux/aspen ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
  ```
	The `balls` application is located at `${F_HOME}/bin/linux/aspen/Release/Balls.tgz`.
2. Transfer the `.tgz` file to your device and decompress.
## Using Xcode
![icon](http://kinoma.com/img/github/xcode.png)
When building for Mac OS X or iOS build targets, the Xcode IDE can be useful during development, particularly for debugging. The XS6 build tools can generate an Xcode project for Mac OS X and iOS targets.
1. Add `-I` to the build line to create an Xcode project. 
  ```
  $ kprconfig6 -I -x -p ios ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
  ```
2. From Xcode, choose the application and the simulator or iOS device.
    ![icon](http://kinoma.com/img/github/xcode-screen.png)
3. Click the **Play** button. The iPhone simulator will launch with the `balls` app.
This also works with the `mac` target.
```
$ kprconfig6 -I -x -p mac ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
```
## Kinoma Element (MW302) Target
![icon](http://kinoma.com/img/github/element.png)
Building KinomaJS for Element uses a Mac OS X build host. Applications are built on a Mac OS X build host. If you have already set up your Mac OS X build host and XS6 tools, skip to the [Build Your KinomaJS Application](#mc-build) section.
The build system uses `mc` as the target platform to build for Kinoma Element.
On Mac OS X, the KinomaJS build uses Homebrew to manage installation and maintenance of CMake.
### Set Up Your Host Build Environment
1. Install the current version of Xcode from Apple's App Store. (As of the writing of this document, the current version is 7.3.) After installing, run Xcode to accept the license agreement and install any required additional components.
2. Install [Homebrew](http://brew.sh/).
  ```
  $ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"  $ brew install caskroom/cask/brew-cask
  ```
3. Install CMake.
  ```
  $ brew install cmake
  ```
4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```5. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6
 ```       
### Build the XS6 Tools
Building KinomaJS for Element currently requires a non-parallel build and setting the `MAKEFLAGS` environment variable. 

1. Build the XS tools as follows:
  ```
  $ export MAKEFLAGS=-j1

  $ cd ${XS6}
  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  $ cmake --build . --target mc
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  $ export PATH=${PATH}:${XS6}/bin/mac/Release
  ```
3. *(Optional)* Build `xsbug`, a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  $ kprconfig6 -p mac -x -m ${XS6}/xsbug/manifest.json
  ```
	The `xsbug` application is located at `${F_HOME}/bin/mac/Release/xsbug.app`.

<a id="mc-build"></a>
### Build Your KinomaJS Application
Build the Kinoma Element simulator that is included in the KinomaJS repository.
```
$ cd ${F_HOME}$ kprconfig6 -d -x -m ${F_HOME}/kinoma/kpr/projects/element/manifest.json```
The simulator application is located at `${F_HOME}/bin/mac/Debug/ElementShell.app`.

<a id="code-build"></a>
## Kinoma Code Target
![icon](http://kinoma.com/img/icons/kinoma-code-icon.png)

Building KinomaJS for the Kinoma Code IDE uses a Mac OS X build host. Applications are built on a Mac OS X build host. If you have already set up your Mac OS X build host and XS6 tools, skip to the [Build Your Kinoma Code Application](#kcode-build) section.

On Mac OS X, the KinomaJS build uses Homebrew to manage installation and maintenance of CMake.

### Set Up Your Host Build Environment

1. Install the current version of Xcode from Apple's App Store. (As of the writing of this document, the current version is 7.3.) After installing, run Xcode to accept the license agreement and install any required additional components.

2. Install [Homebrew](http://brew.sh/).

  ```
  $ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  $ brew install caskroom/cask/brew-cask
  ```

3. Install CMake.

  ```
  $ brew install cmake
  ```

4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:

  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```

5. Set up two environment variables to point to the source tree.

  ```
  $ export F_HOME=/path/to/kinomajs
  $ export XS6=${F_HOME}/xs6
  ```
        
### Build the XS6 Tools

1. Build the XS6 tools as follows:

  ```
  $ cd ${XS6}
  $ mkdir tmp
  $ cd tmp
  $ cmake ${XS6}
  $ cmake --build . --config Release
  ```

2. Update the `PATH` environment variable to include the path to the XS6 tools.

  ```
  $ export PATH=${PATH}:${XS6}/bin/mac/Release
  ```


<a id="kcode-build"></a>
### Build Your KinomaJS Application

Build the `Kinoma Code` application that is included in the KinomaJS repository.

  ```
  $ cd ${F_HOME}
  $ kprconfig6 -x -m ${XS6}/xsedit/manifest.json
  ```

The `Kinoma Code` application is located at `${F_HOME}/bin/mac/Release/Kinoma Code.app`. You can move the application somewhere else, or run it directly with:

  ``` 
  $ open ${F_HOME}/bin/mac/Release/Kinoma\ Code.app
  ```

## Embedded Linux Targets (BeagleBone, Edison, Raspberry Pi, Pine64, Linkit7688, LeMaker Guitar, LeMaker HiKey, Odroid C2))![icon](http://kinoma.com/img/github/linux.png)

Building KinomaJS for embedded Linux uses a Linux build host.
The build system uses `linux/beaglebone`, `linux/edison`, `linux/pi_gl`, `linux/pi`, `linux/pine64`, `linux/linkit7688`, `linux/guitar`, `linux/hikey`, or `linux/odroidc2` as the target platform to build for the respective platform. The examples in this section use `linux/beaglebone`.



### Set Up Your Host Build Environment

1. Install the developer tools and CMake.
  ```
  $ sudo apt-get install build-essential checkinstall  $ sudo apt-get install libgtk-3-dev libasound2-dev zlib1g-dev  $ sudo apt-get install cmake
  ```
2. For a 64-bit Linux, install 32-bit libraries.
  ```
  $ sudo apt-get install zlib1g:i386 lib32stdc++6
  ```
3. Install the toolchain and `sysroot`.	To set up the cross-compilation environment and libraries for your build target:
	
	- For BeagleBone, Raspberry Pi or Edison, see the document [Setup for Building for Embedded Devices](https://github.com/Kinoma/kinomajs/blob/master/doc/EmbeddedSetup.md)  
	
	- For Pine64, LeMaker HiKey, Odroid C2, or other 64-bit arm devices, see the document [Setup for Building for Pine64](https://github.com/Kinoma/kinomajs/blob/master/doc/Pine64Setup.md)
	
	- For Linkit7688, see the document [Setup for Building for Linkit7688](https://github.com/Kinoma/kinomajs/blob/master/doc/Linkit7688Setup.md)
	
	- For LeMaker Guitar, see the document [Setup for Building for LeMaker Guitar](https://github.com/Kinoma/kinomajs/blob/master/doc/LeMakerGuitarSetup.md)
4. Get a copy of the KinomaJS source code. It can be downloaded using your web browser from the [KinomaJS repository on GitHub](http://github.org/kinoma/kinomajs) or using the `git` command-line tool, as follows:
  ```
  $ git clone https://github.com/kinoma/kinomajs.git
  ```5. Set up two environment variables to point to the source tree.
  ```
  $ export F_HOME=/path/to/kinomajs  $ export XS6=${F_HOME}/xs6
  ```### Build the XS6 Tools
1. Build the XS tools as follows:

  ```
  $ cd ${XS6}  $ mkdir tmp  $ cd tmp  $ cmake ${XS6}  $ cmake --build . --config Release
  ```
2. Update the `PATH` environment variable to include the path to the XS6 tools.
  ```
  $ export PATH=${PATH}:${XS6}/bin/linux/$(uname -m)/Release
  ```
3. *(Optional)* Build the `xsbug` debugger: `xsbug` is a graphical debugger used as an alternative to the Kinoma Studio debugger.
  ```
  $ kprconfig6 -p linux/gtk -x -m ${XS6}/xsbug/manifest.json
  ```
	The `xsbug` application is located at `${F_HOME}/bin/linux/gtk/Release/xsbug/xsbug`.

### Build Your KinomaJS Application

The target platform specifiers are as follows:

  - `linux/pi` - Raspberry Pi
  - `linux/pi_gl` - Raspberry Pi, using OpenGL
  - `linux/beaglebone` - BeagleBone
  - `linux/edison` - Intel Edison
  - `linux/pine64` - Pine64
  - `linux/linkit7688` - Linkit7688
  - `linux/guitar` - LeMaker Guitar
  - `linux/hikey` - LeMaker HiKey
  - `linux/odroidc2` - Odroid C2
  
1. Build the `balls` sample application that is included in the KinomaJS repository.
  ```
  $ cd ${F_HOME}  $ kprconfig6 -x -m -p linux/beaglebone ${F_HOME}/kinoma/kpr/applications/balls/manifest.json
  ```
	The `balls` application is located at `${F_HOME}/bin/linux/beaglebone/Release/Balls.tgz`.
2. Transfer the `.tgz` file to your device and decompress.

#### To allow connection to your device from Kinoma Code, build the EmbedShell application

1. Build the `EmbedShell` application

  ```
  $ cd $(F_HOME)
  $ kprconfig6 -x -m -p linux/pine64 $(F_HOME)/kinoma/kpr/projects/embed/manifest.json
  ```

	The `EmbedShell` application is located at `${F_HOME}/bin/linux/pine64/Release/EmbedShell `.2. Transfer the `EmbedShell.tgz` file to your device and decompress.

3. Run the `mdnsd` if Avahi or other mdns is not running on your target. This will allow EmbedShell to advertise itself to your local network.

   ```
	./EmbedShell/lib/mdnsd
   ```

4. Run the EmbedShell

   ```
   ./EmbedShell/EmbedShell
   ```
>**Note**: Depending on your user permissions, you may have to run mdnsd and EmbedShell as root using the sudo command.
>
> ```
> sudo ./EmbedShell/EmbedShell
> ```

