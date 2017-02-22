// simpleWebAudioPlayer implements HTML5 Web Audio API. API documentation can be found here:
// https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API
//
// This HTML5 Web Audio API implementation is a replacement for the original Scratch audio
// code, found in workspace_svg (see loadAudio, preloadAudio and playAudio methods there). The
// Scratch code uses the HTML5 Audio element, an older API.
//
// The HTML5 Web Audio API is used instead of the Audio element because I was
// able to get the Web Audio API working on iOS. I'm not sure if there was a path
// forward to getting the Audio element working.
//
// See also:
// https://developer.apple.com/library/prerelease/content/documentation/AudioVideo/Conceptual/Using_HTML5_Audio_Video/PlayingandSynthesizingSounds/PlayingandSynthesizingSounds.html

var simpleWebAudioPlayer = function () {    
    "use strict";

    var player = {},
        sounds = [],
        audioContext,
        masterGain;

    // Converts a base64-encoded string into an ArrayBuffer.
    //
    // From MDN : Base64 is a group of similar binary-to-text encoding schemes that represent binary
    // data in an ASCII string format by translating it into a radix-64 representation. The Uint8Array
    // typed array represents an array of 8-bit unsigned integers, and we are working with ASCII representation
    // of the data (which is also an 8-bit table)
    function _base64ToArrayBuffer(base64) {
        // First we decode the base64 string (atob)        
        var binary_string =  window.atob(base64);

        // Create new array of 8-bit unsigned integers with the same length as the decoded string
        var len = binary_string.length;
        var bytes = new Uint8Array(len);

        // Iterate over the string and populate the array with Unicode value of each character in the string
        for (var i = 0; i < len; i++)        {
            bytes[i] = binary_string.charCodeAt(i);
        }
        return bytes.buffer;
    }

    player.load = function(sound) {
        // Load the sound
        var myArrayBuffer = _base64ToArrayBuffer(sound.src);

        audioContext.decodeAudioData(myArrayBuffer, function(buffer) {
            sounds[sound.name] = sound;
            sounds[sound.name].buffer = buffer;
            // Invoke a function if a callback is specified
            if (sounds[sound.name].callback) {
                sounds[sound.name].callback();
            }
        });
    };
    
    player.play = function (name) {
        var inst = {};
        
        if (sounds[name]) { 
            // Create a new source for this sound instance

            // Create an AudioBufferSourceNode to play audio data contained within an AudioBuffer object.
            inst.source = audioContext.createBufferSource();
            inst.source.buffer = sounds[name].buffer;

            // Connect the AudioBufferSourceNode to the destination so we can hear the sound
            inst.source.connect(masterGain);
                        
            // Play the sound
            inst.source.start(0);
        }
    };
    
    // Create audio context
    if (typeof AudioContext !== "undefined") {
        audioContext = new window.AudioContext();
    } else if (typeof webkitAudioContext !== "undefined") {
        // Use WebKit AudioContext
        audioContext = new window.webkitAudioContext();
    }
    // The createGain() method of the AudioContext interface creates a GainNode, which can be used to control the overall volume of the audio graph.
    masterGain = (audioContext.createGain) ? audioContext.createGain() : audioContext.createGainNode();

    // Connect the master gain node to the context's output
    masterGain.connect(audioContext.destination);

    return player;
};