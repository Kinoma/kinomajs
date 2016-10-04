#Troubleshooting Zeroconf

Zeroconf is used in KinomaJS for TCP/IP-based advertisement and discovery of networked resources. 

The Pins Sharing feature of the KinomaJS Pins Model can be used to make sensor data discoverable and easily accessible on the Wi-Fi network. It uses Zeroconf to do so. If it is not available, an exception will be thrown.

If an exception is thrown when using Pins Sharing or other Zeroconf features, it's usually because the `mDNSResponder` is not running. Methods for resolving this are different for each supported platform.

##Mac OS X

If running an app in a Kinoma simulator on a Mac and a Zeroconf-related exception is thrown, then the `mDNSResponder` used for Bonjour is likely not running. For the `mDNSResponder` to be unavailable on Mac is very unusual.  If Bonjour has been disabled for some reason, re-enable it from a Terminal window:

```
sudo launchctl load -w /System/Library/LaunchDaemons/com.apple.mDNSResponder.plist
```

##Windows
When running an app in a Kinoma simulator on Windows and a Zeroconf-related exception is thrown, it is because there is currently no `mDNSRsponder` bundled in the Kinoma simulators for Windows.

The easiest way to resolve this is to simply install [Apple iTunes](http://www.apple.com/itunes/download/).

##Linux
If running an app in a Kinoma simulator under Linux and a Zeroconf-related exception is thrown, the `mDNSResponder` is likely not running.

First, make sure the `mdnsd` is installed and executable.

###Using the Simulator from Kinoma Studio
Kinoma Studio includes preconfigured simulators for Kinoma devices like Kinoma Create. The `mDNSResponder` is preinstalled in the Simulator, but may not be running.

1. Open a terminal window
* Find the Kinoma device simulator.  
    * For the Kinoma Create simulator bundled with Kinoma Studio: 
        Go into the `plugins` directory of Kinoma Studio and navigate to 
`com.marvell.kinoma.k4.linux.1.3.49/simulators/CreateShell/lib/`
    * In this directory should be 
        * `mdnsd`
        * `dns-sd` 
        * `libdns_sd.so`
* Type `sudo chmod +x ./mdnsd` to ensure that it's executable.
* Type `./mdnsd` to start the service.

###Using `KPRConfig6` and the Open Source Release
When building an app using `kprconfig6` from the [KinomaJS open-source release](https://github.com/Kinoma/kinomajs), `kprZeroconf` must be included in the `manifest.json` under `extensions`:

1. Navigate to the directory of your app
* In its `manifest.json`, add the following line in the `extensions` block:
        
```
"kprZeroconf":"$(KPR_HOME)/extensions/zeroconf/kprZeroconf.mk", 
```
        
For example:

```
"extensions":{
    "kprZeroconf":"$(KPR_HOME)/extensions/zeroconf/kprZeroconf.mk", 
```

##Kinoma Create
It is very unusual for the `mDNSResponder` to not be running on a Kinoma Create.  If this is the case, simply shutting the device down and restarting should suffice.

Failing that, booting from a fresh OS will certainly resolve it.  The easiest way to do this is by using a MicroSD card:

1. Insert a MicroSD card of any capacity (class 4 at least, class 10 for best performance) and reboot the Kinoma Create.

* In the Kinoma Create "Settings" app, select `Setup SD Card` *Note that this will erase the SD card completely.*

* Proceed through setting up the SD card using the onscreen instructions, including those to make Kinoma create boot from it.  

* Reboot the device, using the SD card as the boot volume.