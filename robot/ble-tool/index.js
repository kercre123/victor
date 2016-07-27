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

	var local_secret;
	var remote_secret, encoded_key;

	function send(tag, payload) {
		var struct = Anki.Cozmo.RobotInterface.EngineToRobot[tag];
		var buffer = struct.serialize(payload);
	
		var data = Buffer.concat([new Buffer([tag]), buffer]);

		console.log(`SEND ${struct.Name()}: ${JSON.stringify(payload)}`);
		console.log("S_BF:", data)

		interface.send(data);
	}

	interface.on('encrypted', (data) => {
		console.log("ENC*:", data);

		// Send our pairing message
		local_secret = crypto.randomBytes(16);
		send(Anki.Cozmo.RobotInterface.EngineToRobot.Tag_enterPairing, { secret: local_secret });
	});

	interface.on('data', (data) => {
		var decoded = ProcessMessage(data);
		console.log("R_BF:", data);
		console.log(`RECV ${decoded.Name()}: ${JSON.stringify(decoded)}`);

		if (decoded instanceof Anki.Cozmo.EncodedAESKey) {
			remote_secret = decoded.secret;
			encoded_key = decoded.encoded_key;

			// Prompt for pin
			prompt.get(['pin'], function (err, result) {
				if (err) { return ; }
				
				interface.key = diffie(parseInt(result.pin, 16), local_secret, remote_secret, encoded_key);
				console.log("KEY*:", interface.key);

				// Rebroadcast the encrypted messages
				interface.setKey(interface.key);
			});
		} else if (decoded instanceof Anki.Cozmo.HelloPhone) {
			send(Anki.Cozmo.RobotInterface.EngineToRobot.Tag_helloRobotMessage, decoded)
			
			send(Anki.Cozmo.RobotInterface.EngineToRobot.Tag_setBackpackLights, {
				lights: [
					{ onColor: 0, offColor: 0 },
					{ onColor: 0, offColor: 0x001F, onFrames: 10, offFrames: 10 },
					{ onColor: 0, offColor: 0x001F << 5, onFrames: 10, offFrames: 10 },
					{ onColor: 0, offColor: 0x001F << 10, onFrames: 10, offFrames: 10 },
					{ onColor: 0, offColor: 0 }
				]
			});
		}
	});

	interface.on('disconnect', (data) => {
		console.log("disconnected");
	});
});

prompt.start();
