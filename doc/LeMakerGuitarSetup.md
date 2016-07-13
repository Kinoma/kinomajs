# Setup for Building for LeMaker Guitar, NanoPi M1

## Overview

Building for the LeMaker Guitar or NanoPi M1 devices requires some one-time setup of your Linux host build enviornment. Once the host environment is configured, building a KinomaJS application is very similar as on the other platforms.

This document and build instructions assume `Lemuntu for Guitar` for the LeMaker Guitar or `NanoPi M1 Debian` for the NanoPi M1 but should be similar for other OS versions.

The main steps for setting up your Linux host build environment for LeMaker Guitar are as follows:

1. Download tools and system image (`sysroot`) for the platform to build and make the environment for cross-compiling the code.

2. Install additional libraries.

3. Set up environment variables.

### Download Tools and `sysroot`

The LeMaker Guitar device is a quad-core 32 bit arm architecture. The build tools and `sysroot` can be found on the [Linaro.org](http://releases.linaro.org/15.05/components/toolchain/binaries/arm-linux-gnueabihf/) website.

Download and unpack the following packages somewhere on your host build machine.

- [gcc-linaro-4.9-2015.05 toolchain](http://releases.linaro.org/15.05/components/toolchain/binaries/arm-linux-gnueabihf/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf.tar.xz)
- [gcc-linaro-4.9-2015.05 sysroot](http://releases.linaro.org/15.05/components/toolchain/binaries/arm-linux-gnueabihf/sysroot-linaro-glibc-gcc4.9-2015.05-arm-linux-gnueabihf.tar.xz)

```
mkdir /opt/tools/guitar
cd /opt/tools/guitar
unxz /tmp/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf.tar.xz
unxz /tmp/sysroot-linaro-glibc-gcc4.9-2015.05-arm-linux-gnueabihf.tar.xz
``` 
    
### Install additional libraries

Install the Alsa libraries and headers on your Guitar target device.

    guitar# apt-get install libasound2-dev
   
Copy the headers and libraries to your host build machine. You will need to know the IP address of your Guitar device. In this example, we'll use 192.168.1.2

```
cd /opt/tools/guitar/sysroot-linaro-glibc-gcc4.9-2015.05-arm-linux-gnueabihf/usr/lib
scp lemuntu@192.168.1.2:/usr/lib/arm-linux-gneabihf/libasound.so.2.0.0 .
ln -s libasound.so.2.0.0 libasound.so
ln -s libasound.so.2.0.0 libasound.so.2

cd /opt/tools/guitar/sysroot-linaro-glibc-gcc4.9-2015.05-arm-linux-gnueabihf/usr/include
scp -r lemuntu@192.168.1.2:/usr/include/alsa .

```

### Set up Environment Variables

Create the following file to load the variables needed to build or set the variables in your local environment.

`/etc/profile.d/guitar.sh`:

```
export GUITAR_SYSROOT=/opt/tools/guitar/sysroot-linaro-glibc-gcc5.3-2016.02-arm-linux-gnueabihf
export GUITAR_GNUEABI=/opt/tools/guitar/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf
```

The build environment is now set up. You can proceed with the KinomaJS build instructions to build an application.
