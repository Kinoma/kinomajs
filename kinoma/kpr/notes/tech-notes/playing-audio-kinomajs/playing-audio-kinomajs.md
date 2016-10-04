<!-- Version: 160711-CR / Last reviewed: October 2015

There are three different ways to play audio in KinomaJS, each appropriate for a different situation. This Tech Note explains the characteristics of the three ways (including using the audio output hardware pin for synthesizing audio in real time) and provides guidance on how to select the right method for your application.
-->

<img alt="" src="img/playing-audio-kinomajs_icon.png" class="technoteIllus" >

#Playing Audio with KinomaJS

**Peter Hoddie**  
April 3, 2015

There are three different ways to play audio in KinomaJS, each appropriate for a different situation. This Tech Note explains the characteristics of the three ways (including using the audio output hardware pin for synthesizing audio in real time) and provides guidance on how to select the right method for your application.

Kinoma Create has a built-in speaker, making it easy to add sound effects to projects. In a recent hackathon, over a third of the projects incorporated audio to deliver a more complete user experience. By providing three ways to play audio, KinomaJS accommodates the many different ways that audio can be used in an application. 

##Choosing an Audio Playback API

KinomaJS offers several different APIs to play back audio, so developers are sometimes unsure which API to select for a specific application. The following points summarize the characteristics of each audio API.

To play the following

* Short sounds in response to user events
* Occasional sounds that are independent from one another
* Audio contained in local files
* One sound at a time

use the [`sound` object](#sound-object), the simplest audio API.

To play the following

* Audio contained in local files or streaming sources
* Long sounds
* Looping audio
* Multiple audio content at the same time

use the [`media` object](#media-object), the most complete audio API.

To play the following

* Audio in response to many different real-time events
* The lowest-latency audio
* Audio with the most control

use the [audio output hardware pin](#audio-pin), a very low-level audio API that allows you to implement your own audio synthesis algorithms.

<a id="sound-object"></a>
##Sound Object

The simplest playback method is the KinomaJS `sound` object, which is designed to play short sounds in response to user interactions. The constructor of the `sound` object takes the URL of a sound file. The sound file can be in any format supported by KinomaJS on the host device, which includes MP3, M4A, and WAVE on Kinoma Create. Once the `sound` object has been created, you can play it.

```
var sound = new Sound("file:///tmp/beep.wav");
sound.play();
```
	
Typically an application stores its sounds inside its directory, so each sound file is accessed using the relative URL to the absolute URL of the file. Use `mergeURI` to resolve the relative path to the file.

```
var sound = new Sound(mergeURI(application.url, "assets/beep.mp3"));
```

An application often creates the `sound` objects it needs when it launches. The objects are assigned to a global variable or the application’s model, so they can be played as needed. Creating many `sound` objects at launch is fast because the file containing the sound is not loaded into memory until the first time the sound is played. For larger sound files, this can result in a brief delay the first time the sound is played. Once the sound is played, it remains in memory until the `sound` instance is garbage-collected.

Because the data for sounds is stored completely in memory, sounds are limited by available memory. This means that `sound` objects should typically reference only small files. There are two essential ways to keep sound files small: make them short in duration or use audio compression. Keeping sounds short in duration is easy to do in any audio editor. Using audio compression means that the audio is stored in a compressed format such as MP3 or M4A. Many tools are available for compressing audio, and most audio editors write output in a variety of different formats.

Compressed sounds have a size advantage, but they require more CPU power to play. KinomaJS `sound` objects decompress the audio at the time it is played. Decompressing the audio takes some CPU power; although typically under 25% of the available CPU, it is enough that it may slow down some animations or data processing.

The `sound` object expects the audio it plays to be short. For efficiency, it provides for playback of only a single sound at a time. While a `sound` object is playing, if a second `sound` object attempts to play, the first sound is stopped. This behavior is reasonable for user interface audio feedback, but it makes `sound` objects impractical for playing chords or sequences of notes.

The volume of all `sound` objects can be changed using the `Sound.volume` global property. The volume level is a number from 0 to 1.0.

```
Sound.volume = 1.0;                // full volume
Sound.volume = 0.05;               // very quiet
Sound.volume = Sound.volume / 2;   // reduce volume by half
```

The [`sound`](https://github.com/Kinoma/KPR-examples/tree/master/sound) sample application uses the KinomaJS `sound` object to play the shutter sound for a camera. The [`kprSound.c`](https://github.com/Kinoma/kinomajs/blob/master/kinoma/kpr/sources/kprSound.c) source code that implements the KinomaJS `sound` object is part of the KinomaJS repository.

<a id="media-object"></a>
##Media Object

The `media` object is the media player in KinomaJS. It plays video and audio, both from a file and streaming from the network. The `media` object is designed to play long-form content, such as songs and movies. A `media` object is always contained within the KinomaJS user interface containment hierarchy (as described in the [*KinomaJS Overview*](../../../../../xs6/xsedit/features/documentation/docs/overview/overview.md) document), which determines how it renders. 

>**Note:** The details of KinomaJS content is outside the scope of this Tech Note.

The `media` object supports playback of a wide variety of file and streaming formats. On Kinoma Create, it supports MP3, M4A, MPV, M4V, MP4, MOV, and FLAC. Most of these file formats can be streamed over HTTP. Kinoma Create also supports streaming MP3 audio.

There are several ways to instantiate a `media` object. The easiest is calling the constructor with a dictionary.

```
var video = new Media({url: "file://tmp/video.mp4", aspect: "fill",
  top: 0, left: 0, width: 160, height: 120});
application.add(video);

var song = new Media({url: "http://example.com/song.mp3",
  width: 0, height: 0});
application.add(song);

var backgroundMusic = new Media({width: 0, height: 0,
  mergeURI(application.url, "assets/backgroundMusic.m4a"});
application.add(backgroundMusic);
```
	
The `media` instance loads the media asynchronously, so it may not be ready to play immediately after the constructor returns, especially for network sources. The `media` instance invokes the `onLoaded` function on its behavior when the media is ready to play. The application can call `play` on the media anytime after that.

```
function onLoaded(media) {
	media.start();
}
```

When the `media` instance reaches the end, the `onFinished` function on the behavior is invoked. The media can be stopped then.

```
function onFinished(media) {
	media.stop();
}
```

An application may want to play continuous background audio. To implement looping audio, use the `onFinished` function to rewind and restart the media.

```
function onFinished(media) {
	media.stop();
	media.time = 0;
	media.start();
}
```

More than one `media` object can be instantiated at a time. Several `media` objects can be playing simultaneously; typically, however, only one `media` object is played at a time, to minimize the CPU load. The `media` object implements playback using streaming, even for local files, so only a small portion of a typical file or stream is loaded in memory at any moment.

The `media` object API contains a significant number of functions, all of which are described in detail in the [*KinomaJS JavaScript Reference*](../../../../../xs6/xsedit/features/documentation/docs/javascript/javascript.md) document. See also the following sample applications:

- The [`media-player`](https://github.com/Kinoma/KPR-examples/tree/master/media-player) sample streams video over HTTP into a simple player user interface. 

- The [`somafm-player`](https://github.com/Kinoma/KPR-examples/tree/master/somafm-player) sample implements a radio tuner user interface for listening to streaming radio from SomaFM. 

The [`kprMedia.c`](https://github.com/Kinoma/kinomajs/blob/master/kinoma/kpr/sources/kprMedia.c) source code that implements the KinomaJS `media` object is part of the KinomaJS repository.

<a id="audio-pin"></a>
##Audio Output Hardware Pin

Playing audio with the audio output hardware pin is the most flexible and challenging way to play sound in KinomaJS. This pin enables applications to play uncompressed audio samples in real time. The application uses an algorithm of its choosing to determine the content of the audio samples. There may be no external audio asset files at all. Generating audio samples in real time, synthesizing audio, is a specialized field requiring knowledge of audio algorithms and code optimization. Performance is a critical concern when audio is synthesized in real time, as there will be gaps and pops in the audio if the synthesis is too slow. 

>**Note:** This Tech Note explains the process of playing synthesized audio in KinomaJS but does not attempt to teach audio synthesis and its optimization.

The audio output hardware pin builds on KinomaJS Hardware Pins, an API to access sensors and actuators connected to Kinoma Create (see the document [*Programming with Hardware Pins for Kinoma Create*](../../../../../xs6/xsedit/features/documentation/docs/pins/pins.md)). The speaker and microphone in Kinoma Create are an actuator and sensor, so KinomaJS provides access to audio output and input through the Hardware Pins API. Using hardware pins has the advantage of running the audio generation code in the hardware pins thread, which is separate from the main KinomaJS application thread. This minimizes the chance for the application to block audio synthesis, which would lead to gaps and pops.

When working with hardware pins, the application communicates with the pins using messages addressed with a URL. The audio generation code is contained in a BLL module. This Tech Note shows how to use the `synthOut.js` BLL in the [`simplesynth`](https://github.com/Kinoma/KPR-examples/tree/master/simplesynth) sample, but the same techniques apply to any BLL that generates audio.

First, the application configures the `synthOut` BLL, which causes KinomaJS to load the module and invoke its `configure` function.

```
application.invoke(new MessageWithObject("pins:configure", {
	audio: {
		require: "synthOut",
		pins: {
			speaker: {sampleRate: 8000}
		}
	}
}));
```

The key pieces of information here are `synthOut`, which is the name of the BLL file (without the `.js` source code file extension), and `sampleRate`, which indicates the number of audio samples the synthesizer will generate per second. Higher sample rates provide the potential for better quality but require more CPU power to calculate. The basic speaker in Kinoma Create 8000 provides adequate quality and enables full synthesis of the audio in this example in JavaScript.

The application then sends two more messages to the BLL. The first message tells the BLL to call its `synthesize` function whenever the audio output pin needs more audio samples. If that function is unable to provide the samples, the application's `/getSamples` handler is invoked.

```
application.invoke(new MessageWithObject("pins:/audio/synthesize?repeat=on&timer=speaker&callback=/getSamples"));
```

Finally, the application sends a message to the BLL to begin synthesizing audio.

```
application.invoke(new MessageWithObject("pins:/audio/start"));
```
	
Once the BLL is running, the application sends messages to the BLL to control the audio being played. The synthesizer can play up to three simultaneous notes:

```
application.invoke(new MessageWithObject("pins:/audio/setFrequencies", [440]));

application.invoke(new MessageWithObject("pins:/audio/setFrequencies", [523, 660]));

application.invoke(new MessageWithObject("pins:/audio/setFrequencies", [523, 660, 763]));
```
	
Send the `silence` message to turn off all synthesis.

```
application.invoke(new MessageWithObject("pins:/audio/silence"));
```
	
To play a sequence of predefined frequencies, use the `setSequence` message.

```
application.invoke(new MessageWithObject("pins:/audio/setSequence", {
	sequence: [
		{samples: 200, frequencies: [523,660]},
		{samples: 200, frequencies: [660,763]},
		{samples: 200, frequencies: [763,880]}
	]
}));
```

Note that the `setFrequencies`, `setSequence`, and `silence` messages all execute immediately. Any audio that is playing is immediately stopped and replaced with the audio in the most recent message.

Inside the BLL, each time the audio hardware is ready for more audio samples to play, the `synthesize` function of the BLL is invoked. The `synthesize` function generates more audio. There are many different algorithms to synthesize audio. The `synthOut` BLL uses a simple sine wave generator. The audio is generated as 16-bit signed-integer samples and stored into a chunk. The chunk is then provided to the audio hardware pin.

```
this.speaker.write(this.chunk);
```

The complete `synthOut` sample is contained in the [`simplesynth`](https://github.com/Kinoma/KPR-examples/tree/master/simplesynth) sample application. All audio synthesis is contained in the BLL `synthOut.js`. The synthesizer implementation is reasonably well optimized, so it may be a little tricky to understand initially.

Hardware pins, including the audio pins, are documented in [*Programming with Hardware Pins for Kinoma Create*](../../../../../xs6/xsedit/features/documentation/docs/pins/pins.md). The audio pins, both input and output, have very straightforward native implementations in [`audioIn.c`](https://github.com/Kinoma/kinomajs/blob/master/kinoma/kpr/extensions/pins/sources/audioIn.c).