const clad = require('./clad');
const crypto = require('crypto');
const aes = require("./aes.js");
const diffie = require("./diffie.js");
const prompt = require('prompt');

const Anki = clad("clad/robotInterface/messageEngineToRobot.clad").Anki;
clad("clad/robotInterface/messageRobotToEngine.clad", Anki);


function ProcessMessage(message) {
	var length = message[0];
	var tag = message[1];
	var payload = message.slice(2, 2 + length);

	var struct = Anki.Cozmo.RobotInterface.EngineToRobot[tag];
	
	return struct.deserialize(payload);
}

var decoded = ProcessMessage(new Buffer([0x0e, 0x28, 0x43, 0x5a, 0x4d, 0x30, 0x9e, 0x93, 0x9b, 0x36, 0x4b, 0xa0, 0x14, 0xd8, 0x52, 0x7b]));
console.log(JSON.stringify(decoded))
console.log(decoded instanceof Anki.Cozmo.HelloPhone)


/*
const factory = require("./factory.js");

var BRYON_COZMO = "db083e35f3031e1d";

const ENTER_PAIRING_MESSAGE = 0x27

factory.on('advertised', (info) => {
	console.log(`Cozmo: ${info.id} (${info.revision})`);

	if (info.id != BRYON_COZMO) { return ; }

	info.device.connect();
});

factory.on('connected', function(interface) {
	console.log("Connected");

	var hello = false;
	var last_msg = +new Date();

	var local_secret = crypto.randomBytes(16);
	var remote_secret, encoded_key;

	var encrypted_messages = [];

	interface.on('encrypted', (data) => {
		console.log("ENC*:", data);

		// Send our pairing message
		var pairing_message = [ENTER_PAIRING_MESSAGE, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
		for (var i = 0; i < 16; i++) {
			pairing_message[i+1] = local_secret[i];
		}

		interface.send(pairing_message);
	});

	interface.on('data', (data) => {
		console.log("RECV:", data);

		// TODO: USE CLAD HERE

		// This is the pairing received message
		if (data[1] == 0x29) {
			remote_secret = data.slice(2,18);
			encoded_key = data.slice(18, 34);

			// Prompt for pin
			prompt.get(['pin'], function (err, result) {
				if (err) { return ; }
				
				interface.key = diffie(parseInt(result.pin, 16), local_secret, remote_secret, encoded_key);
				console.log("KEY*:", interface.key);

				// Rebroadcast the encrypted messages
				interface.setKey(interface.key);
				encrypted_messages.map((data) => aes.decode(interface.key, data)).forEach((d) => interface.emit('data', d));
			});
		}
	});

	interface.on('disconnect', (data) => {
		console.log("disconnected");
	});
});

prompt.start();
*/