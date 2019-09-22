# Audio Input and Output

Kinoma Create has a built-in speaker and microphone, making it easy to add sound effects to projects. (You can read a thorough description of the full capabilities in the Tech Note [Playing Audio with KinomaJS](../../tech-notes/playing-audio-kinomajs/playing-audio-kinomajs.md).) This tutorial will teach you how to create a Kinoma Create application with a simple UI that uses both the microphone and the speaker. It will enable you to record audio for up to 30 seconds and then play it back.


## Configuration

As with any other sensor, you can configure the Kinoma Create's speaker and microphone using the Pins module. There is a built-in Audio module, but for this sample we create a custom BLL to help simplify the main program. 

```
var Pins = require("pins");
Pins.configure({
   sounds: {
      require: "audioBLL",
      pins: {
         microphone: { sampleRate: 8000 },
         speaker: { sampleRate: 8000 }
      }
   } 		
...
```

## Creating the Audio BLL

Add a BLL named `audioBLL.js` to your `src` folder and copy the following code into it.

```
//@module
var soundBite = undefined;
var sampleRate = 8000;
exports.pins = {
    microphone: { type: "Audio", sampleRate: sampleRate, channels: 1, 
    	direction: "input" },
    speaker: { type: "Audio", sampleRate: sampleRate, channels: 1, 
    	direction: "output" }
};

exports.configure = function () {
	this.microphone.init();
	this.speaker.init();
	this.speaker.setVolume( 1 );	
}

exports.startRecording = function(){
	this.microphone.start();
	this.speaker.stop();
}

exports.stopRecording = function(){
	this.microphone.stop();	
	soundBite = {};
	soundBite.content = this.microphone.read();
	soundBite.duration = (soundBite.content.byteLength / 2) / sampleRate;
	return( soundBite.duration );
}

exports.playRecording = function(){
	if ( soundBite.content ){
		this.speaker.write( soundBite.content );
		this.speaker.start();
		return(soundBite.duration);
	}
}

exports.stopPlay = function(){
	this.speaker.stop();
}

exports.close = function(){
	this.speaker.close();
	this.microphone.close();
}
```

You can see the built-in Audio methods being called here. The `read` method is used to store the sound data from the microphone; this data can then be written to the speaker. 

## Defining the User Interface

The UI is fairly simple. The **Play** button plays the last recorded sound clip. Using the `onTouchEnded` method, the app keeps track of the `isPlaying` state and plays or stops the speaker accordingly. The message path `/pauseForDuration` ensures that the state of the button is switched upon the clip's completion. 

```
...
onTouchEnded: function(container) {
	if (this.isPlaying) {
		Pins.invoke("/sounds/stopPlay");
		container.first.string = "Play";
	} else {
		Pins.invoke("/sounds/playRecording", function(duration) {
			new MessageWithObject("/pauseForDuration", duration*1000).invoke();
		});
		container.first.string = "Stop";
	}
	this.isPlaying = !this.isPlaying;
	container.state = this.isPlaying;
},
...
```

The **Start Recording** button switches between start and stop recording states, while invoking the appropriate module methods. 

```
...
onTouchEnded: function(container) {
	if (this.isRecording) {
		Pins.invoke("/sounds/stopRecording");
		container.first.string = "Start Recording";
	} else {
		Pins.invoke("/sounds/startRecording");
		container.first.string = "Stop Recording";
	}
	this.isRecording = !this.isRecording;
	container.state = this.isRecording;
}
...
```

## Running the App

The video in Figure 1 shows the full app running on Kinoma Create.

**Figure 1.** Running the Audio App

<iframe width="100%" height="500" src="https://www.youtube.com/embed/vZKFQ-klXYI?rel=0&amp;vq=hd1080" frameborder="0" allowfullscreen><a href="https://www.youtube.com/embed/vZKFQ-klXYI?rel=0&amp;vq=hd1080">Watch Video</a></iframe>

<br>You can download the full project <a href="../content/audio-record-player.zip" download>here</a>.