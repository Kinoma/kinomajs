# Setup for Building for Pine64, LeMaker HiKey, Odroid C2, or other 64-bit arm devices

## Overview

Building for the Pine64 device requires some one-time setup of your Linux host build enviornment. Once the host environment is configured, building a KinomaJS application is very similar as on the other platforms.

This document and build instructions are based off of the `xenial-pine64-bspkernel-20160507.img`. Information about creating the boot image and SD card for this configuration can be found [on the Pine64 forum](http://forum.pine64.org/showthread.php?pid=2827).

The main steps for setting up your Linux host build environment for Pine64 are as follows:

1. Download tools and system image (`sysroot`) for the platform to build and make the environment for cross-compiling the code.

2. Install additional libraries.

3. Set up environment variables.

### Download Tools and `sysroot`

The Pine64 device is a 64 bit arm architecture. The build tools and `sysroot` can be found on the [Linaro.org](http://releases.linaro.org/14.11/components/toolchain/binaries/aarch64-linux-gnu/) website.

Download and unpack the following packages somewhere on your host build machine.

- [aarch64-linux-gnu toolchain](http://releases.linaro.org/14.11/components/toolchain/binaries/aarch64-linux-gnu/gcc-linaro-4.9-2014.11-x86_64_aarch64-linux-gnu.tar.xz)
- [aarch64-linux-gnu sysroot](http://releases.linaro.org/14.11/components/toolchain/binaries/aarch64-linux-gnu/sysroot-linaro-eglibc-gcc4.9-2014.11-aarch64-linux-gnu.tar.xz)

```
mkdir /opt/tools/pine64
cd /opt/tools/pine64
unxz /tmp/gcc-linaro-4.9-2014.11-x86_64_aarch64-linux-gnu.tar.xz
unxz /tmp/sysroot-linaro-eglibc-gcc4.9-2014.11-aarch64-linux-gnu.tar.xz
``` 
    
### Install additional libraries

Install the Alsa libraries and headers on your Pine64 target device.

    pine64# apt-get install libasound2-dev
   
Copy the headers and libraries to your host build machine. You will need to know the IP address of your pine64 device. In this example, we'll use 192.168.1.2

```
cd /opt/tools/pine64/sysroot-linaro-eglibc-gcc4.9-2014.11-aarch64-linux-gnu/usr/lib/aarch64-linux-gnu/
scp ubuntu@192.168.1.2:/usr/lib/aarch64-linux-gnu/libasound.so.2.0.0 .
ln -s libasound.so.2.0.0 libasound.so
ln -s libasound.so.2.0.0 libasound.so.2

cd /opt/tools/pine64/sysroot-linaro-eglibc-gcc4.9-2014.11-aarch64-linux-gnu/usr/include
scp -r ubuntu@192.168.1.2:/usr/include/alsa .

```

### Set up Environment Variables

Create the following file to load the variables needed to build or set the variables in your local environment.

For Pine64:

`/etc/profile.d/pine64.sh`:

```
export PINE64_SYSROOT=/opt/tools/pine64/sysroot-linaro-eglibc-gcc4.9-2014.11-aarch64-linux-gnu
export PINE64_GNUEABI=/opt/tools/pine64/gcc-linaro-4.9-2014.11-x86_64_aarch64-linux-gnu
```

For LeMaker HiKey, use the environment variables: `HIKEY_SYSROOT` and `HIKEY_GNUEABI`.

For Odroid C2, use the environment variables: `ODROIDC2_SYSROOT` and `ODROIDC2_GNUEABI`.


The build environment is now set up. You can proceed with the KinomaJS build instructions to build an application.
