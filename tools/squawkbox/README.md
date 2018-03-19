# Squawkbox

### What it is:
Squawkbox is an audio interface that allows for directional testing. It is a large, relatively sound proof box, that has 12 speakers that can be triggered individually. The goal is to automate testing Victors ability to know where a sound is coming from, and whether or not it understood it as a command ("Hey Victor, what time is it?")

For now, the only thing available is the ability to play audio from 1 of 12 speakers. The next steps would be to connect to Victor, and confirm if the correct directions and intentions were triggered.

As of now, there is one way to use the Audio interface.

**Automatic Mode:** Sound is played through 1 of the 12 speakers. Sound is pulled from an audio folder, where any .wav audio files will be marked as playable. In order to play the sounds, you would need to create/use a json file that will 

1. Tell which sounds to play
2. How many times to play
3. Pause in between each sound
4. Which speaker to play from

Here is the format of the json file.

```
{
  "play" : [  
    {
      "Speaker": 3,
      "Sound": "alarm",
      "Play_Amount": 1,
      "Delay": 0
    },
    {
      "Speaker": 3, 
      "Sound": "unknown_intent",
      "Play_Amount": 3,
      "Delay": 0
    }
  ]
}
```
**Important** as of now, the name of the `Sound` is based on the name of any .wav file in the `audio_folder` *without* the `.wav` extension. 

