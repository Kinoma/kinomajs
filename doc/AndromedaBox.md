# KinomaJS on Andromeda Box

Get "invited" to Google Brillo (see link below - use your marvell.com email).

[Get Brillo SDK](https://developers.google.com/brillo/guides/get-started/downloads?authuser=1)

Build a sample Weave application ([LED Flasher Brillo device](https://codelabs.developers.google.com/codelabs/brillo-weave-leds/?authuser=1#0)) so that you can use the Weave provisioning app to set the WiFi access point. Follow the example on the website.

Get Weave companion app on an Android device (need the Google invitation).

Using the Weave app, connect to LED Flasher "device" and provision the WiFi.

Build KinomaJS EmbedShell for **linux/andro**

```
export ANDRO_GNUEABI=$BDK_HOME/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9
export ANDRO_SYSROOT=$BDK_HOME/prebuilts/ndk/current/platforms/android-23/arch-arm

kprconfig6 -x -m -p linux/andro kinoma/kpr/projects/embed/manifest.json
```

adb push EmbedShell to device (no tgz on device, so ungzip it to EmbedShell.tar)

```
sudo adb root
sudo adb -d push EmbedShell.tar /data/local
sudo adb shell
cd /data/local
tar -xvf EmbedShell.tar
```

On the device, open some ports to allow access to KinomaJS's http server:

```
COAP - 5683 (udp)
SSDP response server - 10000:10010(?) (tcp)
Element - 4433 (tcp/https)
xmpp - 5222

```

```
iptables -A INPUT -p tcp --dport 10000:10010 -j ACCEPT
iptables -A INPUT -p udp --dport 5683 -j ACCEPT
iptables -A INPUT -p tcp --dport 4433 -j ACCEPT
iptables -A INPUT -p tcp --dport 5222 -j ACCEPT
```

Let the environment know about where to find the libdns_sd.so file:

```
export LD_LIBRARY_PATH=/data/local/EmbedShell/lib
```

Run EmbedShell

```
/data/local/EmbedShell/EmbedShell
```

In Kinoma Code, turn on discovery for the Andromeda Box.

Preferences->Devices->Discovery->Andromeda Box

 

----
References:

[Brillo page - Request Invite](https://developers.google.com/brillo/?authuser=0])

[Brillo page - after invitation received](https://developers.google.com/brillo/guides/overview/what-is-brillo?authuser=1)

[What is Weave](https://developers.google.com/weave/guides/overview/what-is-weave?authuser=1)

[Andromeda Box Edge - Getting Started](https://developers.google.com/brillo/guides/get-started/aboxedge-marvell?authuser=1)


