# Setup for Building for Embedded Devices

## Overview

Building for embedded Linux devices (BeagleBone, Raspberry Pi, Edison) requires some one-time setup of your Linux host build environment. Once the host environment is configured, building a KinomaJS application is very similar as on the other platforms.

The main steps for setting up your Linux host build environment for BeagleBone and Raspberry Pi are as follows (and the steps for Edison are similar):
 
1. Download tools and system images for the platform to build and make the environment for cross-compiling the code.

2. Install the tools.

3. Create the `sysroot`.

4. Build `libmraa` for access to pins.

5. Set up environment variables.

To create the `sysroot` for BeagleBone and Raspberry Pi, we use `kpartx`, a tool that reads partition tables on specified images and creates device maps over detected partition segments. We use the `-av` flag to attach an image and return the loop device to which the image is mapped. Usually this will be `loop0`, but it can vary depending on whether you have other loop devices set up. Examples in this document assume that `loop0` is being used; adjust as necessary.

## BeagleBone

### Download Tools and System Images

Download the following:

- [SD image](https://rcn-ee.com/rootfs/2016-02-11/microsd/bone-ubuntu-14.04.3-console-armhf-2016-02-11-2gb.img.xz) for the latest image as of this writing. Check [https://rcn-ee.com/rootfs/](https://rcn-ee.com/rootfs/) for a more recent SD image, as it changes often.

- [GCC tools](https://github.com/raspberrypi/tools.git) (These are the same as for Raspberry Pi.)


### Install the Tools

The required tools are:

- `git`

- `kpartx`

- `python`

- `subversion`

- `unzip`

- `xa-utils`

Install them as appropriate for your host build environment. On Ubuntu:

```
sudo apt-get install git
sudo apt-get install kpartx
sudo apt-get install python
sudo apt-get install subversion
sudo apt-get install unzip
sudo apt-get install xa-utils
```

### Install the ARM Tools

Create the location`/opt/tools` to hold the ARM tools. Then pull down the necessary tools from the repository.

`i386`:

```
mkdir -p /opt/tools

cd /opt/tools

svn checkout https://github.com/raspberrypi/tools.git/trunk/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian
```

`x86_64`:

```
mkdir /opt/tools

cd /opt/tools

svn checkout https://github.com/raspberrypi/tools.git/trunk/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64
```

### Create the sysroot

1. Extract the SD image that you downloaded earlier.

  ```
  xz -d bone-ubuntu-14.04.3-console-armhf-2016-02-11-2gb.img.xz
  ```

2. Use `kpartx` to attach the filesystem to the loop device and map its partitions.

  ```
  kpartx -av bone-ubuntu-14.04.3-console-armhf-2016-01-14-2gb.img
  ```

   This will return the loop device to which the device is attached--most likely `/dev/loop0`, but it may vary. Substitute the device loop number as returned by `kpartx` for all examples here.

3. Mount the partition and begin creating the `sysroot`. It will be located at `/opt/beaglebone`.

  ```
  mkdir /opt/beaglebone
  mount /dev/mapper/loop0p1 /mnt
  mkdir /opt/beaglebone/usr
  cp -R /mnt/usr/lib /opt/beaglebone/usr/
  cp -R /mnt/usr/include /opt/beaglebone/usr/
  ```

4. Do the following cleanup.

  ```
  umount /mnt
  dmsetup remove /dev/mapper/loop0*
  losetup -d /dev/mapper/loop0
  rm bone-ubuntu-14.04.3-console-armhf-2016-01-14-2gb.img
  ```

### Set Up Fixed Qualified Library Paths

There are symbolic links in the toolchain to point within the `sysroot`. The symlinks need to point to the new location on the Linux build host in order for the toolchain to work.

```
wget -o /tmp/fixQualifiedLibraryPaths https://raw.githubusercontent.com/shahriman/cross-compile-tools/master/fixQualifiedLibraryPaths

chmod +x /tmp/fixQualifiedLibraryPaths

/tmp/fixQualifiedLibraryPaths /opt/beaglebone /opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc
```

### Build libmraa

KinomaJS uses `libmraa` to access hardware pins on the device. It does not come pre-installed on Beaglebone, so you need to build the library.

1. Create a CMake toolchain for the platform.

  `/tmp/mraa.toolchain`:
  
  ```
  include(CMakeForceCompiler)

  set(CMAKE_SYSROOT           /opt/beaglebone)

  cmake_force_c_compiler(     /opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc GNU)
  cmake_force_cxx_compiler(   /opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++ GNU)

  set(CMAKE_C_FLAGS           "--sysroot=${CMAKE_SYSROOT}" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS         "--sysroot=${CMAKE_SYSROOT}" CACHE STRING "" FORCE)

  set(CMAKE_FIND_ROOT_PATH    ${CMAKE_SYSROOT})
  ```

2. Download the source code.

  ```
  git clone https://github.com/intel-iot-devkit/mraa /tmp/mraa
  ```

3. Build `libmraa` for the platform and install the artifacts into the `sysroot`.

  ```
  mkdir /tmp/mraa/build
  cmake -DCMAKE_TOOLCHAIN_FILE=/tmp/mraa.toolchain \
      -DBUILDSWIG=OFF \
      -DBUILDTESTS=OFF \
      -H/tmp/mraa \
      -B/tmp/mraa/build
  cmake --build /tmp/mraa/build --config Release
  cp /tmp/mraa/build/src/libmraa* /opt/beaglebone/usr/lib/
  cp -R /tmp/mraa/api/* /opt/beaglebone/usr/include/
  ```

### Set Up Environment Variables

Create the following file to load the variables needed to build or set the variables in your local environment. 

`/etc/profile.d/beaglebone.sh`:

```
export BB_SYSROOT=/opt/beaglebone
export BB_GNUEABI=/opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian
```

The build environment is now set up. You can proceed with the KinomaJS build instructions to build an application.


## Raspberry Pi

### Download Tools and System Images

Download the following:

- [SD image](http://director.downloads.raspberrypi.org/raspbian_lite/images/raspbian_lite-2016-03-18/2016-03-18-raspbian-jessie-lite.zip)

- [GCC tools](https://github.com/raspberrypi/tools.git)


### Install the Tools

The required tools are:

-  `git`

-  `kpartx`

-  `python`

-  `subversion`

-  `unzip`

Install them as appropriate for your host build environment. On Ubuntu:

```
sudo apt-get install git
sudo apt-get install kpartx
sudo apt-get install python
sudo apt-get install subversion
sudo apt-get install unzip
```

### Install the ARM Tools

The ARM tools are the same for BeagleBone and Raspberry Pi; if you have already installed the ARM tools to build for BeagleBone, you can skip this step.

Create the location `/opt/tools` to hold these tools. Then pull down the necessary tools from the repository.

`i386`:

```
mkdir -p /opt/tools

cd /opt/tools

svn checkout https://github.com/raspberrypi/tools.git/trunk/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian
```

`x86_64`:

```
mkdir /opt/tools

cd /opt/tools

svn checkout https://github.com/raspberrypi/tools.git/trunk/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64
```

### Create the sysroot

1. Extract the SD image that you downloaded earlier.

  ```
  unzip -qq 2016-03-18-raspbian-jessie-lite.zip
  ```

2. Use `kpartx` to attach the filesystem to the loop device and map its partitions.

  ```
  kpartx -av 2016-03-18-raspbian-jessie-lite.img
  ```

  This will return the loop device to which the device is attached--most likely `/dev/loop0`, but it may vary. Substitute the device loop number as returned by `kpartx` for all examples here.

3. Mount the partition and begin creating the `sysroot`. It will be located at `/opt/pi`.

  ```
  mkdir /opt/pi
  mount /dev/mapper/loop0p2 /mnt
  mkdir /opt/pi/usr
  cp -R /mnt/usr/lib /opt/pi/usr/
  cp -R /mnt/usr/include /opt/pi/usr/
  ```

4. Do the following cleanup.

  ```
  umount /mnt
  dmsetup remove /dev/mapper/loop0*
  losetup -d /dev/mapper/loop0
  rm 2016-03-18-raspbian-jessie-lite.img
  ```

### Set Up Fixed Qualified Library Paths

There are symbolic links in the toolchain to point within the `sysroot`. The symlinks need to point to the new location on the Linux build host in order for the toolchain to work.

```
wget -o /tmp/fixQualifiedLibraryPaths https://raw.githubusercontent.com/shahriman/cross-compile-tools/master/fixQualifiedLibraryPaths

chmod +x /tmp/fixQualifiedLibraryPaths

/tmp/fixQualifiedLibraryPaths /opt/pi /opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc
```

### Install libmraa

KinomaJS uses `libmraa` to access hardware pins on the device. It does not come pre-installed on Raspberry Pi, so we need to build the library.

1. Create a CMake toolchain for the platform.

  `/tmp/mraa.toolchain`:
  
  ```
  include(CMakeForceCompiler)

  set(CMAKE_SYSROOT           /opt/pi)

  cmake_force_c_compiler(     /opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc GNU)
  cmake_force_cxx_compiler(   /opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++ GNU)

  set(CMAKE_C_FLAGS           "--sysroot=${CMAKE_SYSROOT}" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS         "--sysroot=${CMAKE_SYSROOT}" CACHE STRING "" FORCE)

  set(CMAKE_FIND_ROOT_PATH    ${CMAKE_SYSROOT})
  ```

2. Download the source code.

  ```
  git clone https://github.com/intel-iot-devkit/mraa /tmp/mraa
  ```

3. Build `libmraa` for the platform and install the artifacts into the `sysroot`.

  ```
  mkdir /tmp/mraa/build
  cmake -DCMAKE_TOOLCHAIN_FILE=/tmp/mraa.toolchain \
      -DBUILDSWIG=OFF \
      -DBUILDTESTS=OFF \
      -H/tmp/mraa \
      -B/tmp/mraa/build
  cmake --build /tmp/mraa/build --config Release
  cp /tmp/mraa/build/src/libmraa* /opt/pi/usr/lib/
  cp -R /tmp/mraa/api/* /opt/pi/usr/include/
  ```

### Get libasound

Fetch the `.deb` package and extract the contents into the `sysroot`.

```
wget -o /tmp/libasound.deb http://mirrordirector.raspbian.org/raspbian/pool/main/a/alsa-lib/libasound2-dev_1.1.0-1_armhf.deb

cd /tmp/

ar -x libasound.deb data.tar.gz

tar xjvf data.tar.gz -C /opt/pi
```

### Set Up Environment Variables

Create the following file to load the variables needed to build or set the variables in your local environment.

`/etc/profile.d/pi.sh`:

```
export PI_SYSROOT=/opt/pi
export PI_GNUEABI=/opt/tools/gcc-linaro-arm-linux-gnueabihf-raspbian
```

The build environment is now set up. You can proceed with the KinomaJS build instructions to build an application.


## Edison

### Download the SDK

To download the SDK, use the link below for your Linux host build platform.

- [Edison SDK Linux 32-bit](http://downloadmirror.intel.com/25028/eng/edison-sdk-linux32-ww25.5-15.zip)

- [Edison SDK Linux 64-bit](http://downloadmirror.intel.com/25028/eng/edison-sdk-linux64-ww25.5-15.zip)

### Install the Tools

The required tools are:

- `python`

- `unzip`

Install them as appropriate for your host build environment. On Ubuntu:

```
sudo apt-get install python
sudo apt-get install unzip
```

### Install the SDK

1. Extract the SDK that you downloaded earlier. The following example shows the 64-bit SDK.

  ```
  cd /tmp

  wget -O http://downloadmirror.intel.com/25028/eng/edison-sdk-linux64-ww25.5-15.zip

  unzip -qq edison-sdk-linux64-ww25.5-15.zip
  ```

2. Install the SDK.

  ```
  sh poky-edison-* -y
  rm edison-sdk-linux64-ww25.5-15.zip
  rm poky_edison-*
  ```

### Set Up Environment Variables

Create the following file to load the variables needed to build or set the variables in your local environment.

`/etc/profile.d/edison.sh`:

```
export EDISON_SYSROOT=/opt/poky-edison/1.7.2/sysroots/core2-32-poky-linux
export EDISON_GNUEABI=/opt/poky-edison/1.7.2/x86_64-pokysdk-linux/usr/bin/i586-poky-linux
```
The build environment is now set up. You can proceed with the KinomaJS build instructions to build an application.