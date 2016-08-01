const clad = require('./clad');
const crypto = require('crypto');
const aes = require("./aes.js");
const diffie = require("./diffie.js");
const prompt = require('prompt');

const Anki = clad("clad/robotInterface/messageEngineToRobot.clad").Anki;
clad("clad/robotInterface/messageRobotToEngine.clad", Anki);

const BRYON_COZMO = "db083e35f3031e1d";

function ProcessMessage(message) {
	var length = message[0];
	var tag = message[1];
	var payload = message.slice(2, 2 + length);

	var struct = Anki.Cozmo.RobotInterface.EngineToRobot[tag];
	
	return struct.deserialize(payload);
}

const factory = require("./factory.js");

factory.on('advertised', (info) => {
	console.log(`Cozmo: ${info.id}`);

	if (info.id != BRYON_COZMO) { return ; }

	info.device.connect();
});

factory.on('connected', function(interface) {
	console.log("Connected");

	var hello = false;
	var last_msg = +new Date();

	var local_secret, remote_secret, encoded_key;

	function send(tag, payload) {
		var struct = Anki.Cozmo.RobotInterface.EngineToRobot[tag];
		var buffer = struct.serialize(payload);
	
		var data = new Buffer(buffer);

		console.log(`SEND ${struct.Name()}: ${JSON.stringify(payload)}`);

		interface.send(tag, data);
	}

	function promptForPin() {
		// Prompt for pin
		prompt.get(['pin'], function (err, result) {
			if (err) { return ; }

			interface.key = diffie(parseInt(result.pin, 16), local_secret, remote_secret, encoded_key);
			console.log("KEY*:", interface.key);

			// Rebroadcast the encrypted messages
			interface.setKey(interface.key);
		});
	}

	interface.on('encrypted', (data) => {
		console.log("ENC*:", data);

		// Send our pairing message
		local_secret = crypto.randomBytes(16);
		send(Anki.Cozmo.RobotInterface.EngineToRobot.Tag_enterPairing, { secret: local_secret });
	});

	interface.on('data', (data, hmac) => {
		try {
			var decoded = ProcessMessage(data);
			console.log("R_BF:", data);
			console.log(`RECV ${hmac} ${decoded.Name()}: ${JSON.stringify(decoded)}`);
		} catch(e) {
			console.log("Message decode failed")
		}

		// Valid messages
		if (hmac) {
			if (decoded instanceof Anki.Cozmo.EncodedAESKey) {
				remote_secret = decoded.secret;
				encoded_key = decoded.encoded_key;
			
				promptForPin();
			} else if (decoded instanceof Anki.Cozmo.HelloPhone) {
				// We got a hanshake, set the backpack LEDs
				const RED = 0x7C00;
				const GREEN = 0x03E0;
				const BLUE = 0x001F;
				
				send(Anki.Cozmo.RobotInterface.EngineToRobot.Tag_setBackpackLights, {
					lights: [
						{ onColor: 0, offColor: 0 },
						{ onColor: RED, offColor: BLUE },
						{ onColor: RED, offColor: BLUE },
						{ onColor: RED, offColor: BLUE },
						{ onColor: 0, offColor: 0 }
					]
				});
			}
		} else {

		}

		// This is a special case
		if (decoded instanceof Anki.Cozmo.HelloPhone) {
			interface.setNonce(decoded.nonce);
		} else if (!hmac) {
			promptForPin();
		}
	});

	interface.on('disconnect', (data) => {
		console.log("disconnected");
	});
});

prompt.start();
