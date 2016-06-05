# Setup for Building for Linkit Smart 7688

## Overview

Building for the Linkiit Smart 7688 device requires some one-time setup of your Linux host build enviornment. Once the host environment is configured, building a KinomaJS application is very similar as on the other platforms.

The Linkit Smart 7688 is a MIPS architecture device running the OpenWrt version of Linux. The SDK includes [mraa](http://iotdk.intel.com/docs/master/mraa/) to access the pins on the device.

This document and build instructions are based off of the version 0.9.3 firmware and SDK. Information about creating the boot image and SD card for this configuration can be found [on the Linkit Smart 7688 site](https://labs.mediatek.com/site/global/developer_tools/mediatek_linkit_smart_7688/sdt_intro/index.gsp).

The main steps for setting up your Linux host build environment for the Linkit Smart 7688 are as follows:

1. Download the SDK and toolchain for the platform to build and make the environment for cross-compiling the code.

2. Install additional libraries

3. Set up environment variables.

### Download SDK

The Linkit Smart 7688 SDK includes the build tools and `sysroot` and can be found on the [Linkit Smart 7688](https://labs.mediatek.com/site/global/developer_tools/mediatek_linkit_smart_7688/sdt_intro/index.gsp) website.

Download and unpack the following package somewhere on your host build machine.

- [Linux Toolchain](http://download.labs.mediatek.com/MediaTek_LinkIt_Smart_7688_Openwrt_toolchain_Linux)

```
mkdir /opt/tools/linkit7688
cd /opt/tools/linkit7688
tar -jxvf /tmp/OpenWrt-Toolchain-ramips-mt7688_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64.tar.bz2
``` 

To reduce the size of the paths to the toolchain and `sysroot`, make a link to it.

```
cd /opt/tools/linkit7688
ln -s OpenWrt-Toolchain-ramips-mt7688_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2 linkitToolchain
```
   
### Install additional libraries

To access the pins on the device with KinomaJS, install mraa.

(from the [MediaTek forum](http://labs.mediatek.com/forums/posts/list/0/4226.page#p8790))

1. Fetch the libraries and headers

   ```
   cd /opt/tools/linkit7688
   wget -O api.tar.gz https://www.dropbox.com/s/qmx73eyrwwh654v/api.tar.gz?dl=0
   tar -zxvf api.tar.gz
   cd api
	wget -O libmraa.a https://www.dropbox.com/s/ihw50mzq63knwil/libmraa.a?dl=0
   ```
 
2. Copy the headers and libraries into place and link library versions

	```
	cp -r mraa* /opt/tools/linkit7688/linkitToolchain/usr/include
	cp libmraa* /opt/tools/linkit7688/linkitToolchain/usr/lib
	cd /opt/tools/linkit7688/linkitToolchain/usr/lib
	ln -s libmraa.so.0.8.0 libmraa.so
	ln -s libmraa.so.0.8.0 libmraa.so.0
	```
 
### Set up Environment Variables

Create the following file to load the variables needed to build or set the variables in your local environment.

`/etc/profile.d/linkit7688.sh`:

```
export LINKIT7688_SYSROOT=/opt/tools/linkit7688/linkitToolchain
export LINKIT7688_GNUEABI=/opt/tools/linkit7688/linkitToolchain
```

The build environment is now set up. You can proceed with the KinomaJS build instructions to build an application.