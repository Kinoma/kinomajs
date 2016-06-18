# Instrumentation

KinomaJS has an instrumentation facility to assist in the development and debugging of the KPL, Fsk, and KPR layers. 

Instrumentation is enabled by the build-line option `-i` for debug builds.

```
kprconfig6 -x -m -d -i .../manifest.json
```

## Enabling the Instrumented Types

Individual instrumented types are enabled in the `instrument` entry of the application's `manifest.json` file. There are dozens of instrumented types in KinomaJS; a partial list is provided in the next section.

```
"instrument":  {
    "log": "",
    "syslog": "",
    "trace": true,
    "threads": true,
    "times": true,
    "androidlog": true,
    "kinds": [
    	{ "type": "KprLayer", "messages": "debug" },
    	{ "type": "KprText", "messages": "debug" },
    	{ "type": "KprStyle", "messages": "debug" },
    	{ "type": "freetype", "messages": "debug" },
    ]
}
```

The `instrument` entry includes the following properties:

* `log`
* `syslog`
* `trace`
* `threads`
* `times`
* `androidlog`
* `kinds`

##### `kinds`

```
"kinds": [
	{ "type": "KprLayer", "messages": "debug" },
]
```

The `kinds` property enables the output of a specific instrumented type (see the next section) and the level of messages reported. Message levels, in order of increasingly detailed output, are:

- `minimal`
- `normal`
- `verbose`
- `debug`

##### `log`

`"log": "/fullpath/logfile.txt"` writes all instrumentation output to the specified file.

##### `syslog`

`"syslog": "ipaddr:port"` writes instrumentation log lines via UDP to the [system log](https://en.wikipedia.org/wiki/Syslog) at the specified IP address and port--for example, `"10.0.10.23:514"`.

##### `trace`

`"trace": true` emits the instrumentation output to `stderr`.

##### `threads`

`"threads": true` adds to the log line a prefix indicating the thread that generated the output.

##### `times`

`"times": true` adds to the log line a prefix indicating the time that the log line is generated.

##### `androidlog`

`"androidlog": true` emits instrumentation output for the Android platform in the system's [`logcat`](http://developer.android.com/tools/help/logcat.html) log. It is available for the Android platform only; it is ignored on other platforms.

## List of Instrumented Types

Below is a partial list of the instrumented types in KinomaJS and its extensions.


```
audiodecompress
audiofilter
audioinnative
audionative
audioout
bitmap
blit
condition
directoryiterator
error
event
extensions
file
filemapping
freetype
hardware
httpclient
httpclientauth
httpclientrequest
httpserver
httpserverrequest
httpsystem
imagedecompress
mediaplayer
mediareader
mediatranscoder
memory
mutex
netinterfacenotifier
opengl
phoneproperty
port
resolver
rtsp
rtspreader
runloop
semaphore
sndchannel
socket
textedit
thread
timecallback
vm
window
xdbMedia
xs

kplfiles
linuxfiles

androidenergy
androidevent
androidfiles
androidframebuffer
androidfsnotifier
androidglue
androidjni
androidmainblock
androidphonestate
androidsurface
androidte
androidtouch
androidwindow
FskAndroidJavaVideoEncoder
fskStagefrightOMXCodec
fskStagefrightOMXCodecExtension

kinomaaacdecfh
kinomaaacdecipp
kinomaaacdecpv
kinomaamrnbdecpv
kinomaavcdecipp
kinomaavcdecpv
kinomagifdecipp
kinomaiomxdec
kinomaiomxdecextension
kinomajpegdecipp
kinomajpegencipp
kinomamp3decpv
kinomamp4ipp
kinomapngdecipp
kinomaqtdec
kinomasbcdec
kinomavmetadec
kinomavmetadecbg2
kinomayuv420dec

FskJpegIdct
FskMediaReaderFLAC
FskMediaReaderFLV
FskMediaReaderYUV420
FskPngDecode
FskRectBlit

KprApplication
KprBitmap
KprCanvas
KprColumn
KprContainer
KprContent
KprEffect
KprHTTPClient
KprHTTPConnection
KprHorizontalTable
KprHost
KprImage
KprImageEntry
KprImageTarget
KprLabel
KprLayer
KprLine
KprMedia
KprMessage
KprSSDP
KprSSDPDevice
KprSSDPDiscovery
KprSSDPInterface
KprSSDPPacket
KprSSDPPacketMessage
KprScriptBehavior
KprScroller
KprShell
KprSkin
KprStyle
KprText
KprTextLink
KprTexture
KprThumbnail
KprTransition
KprUPnP
KprUPnPDevice
KprUPnPService
KprUPnPSubscription
KprVerticalTable
kprMediaWriter
```
