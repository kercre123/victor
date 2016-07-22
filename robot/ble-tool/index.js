const crypto = require('crypto');
const factory = require("./factory.js");
const aes = require("./aes.js");
const diffie = require("./diffie.js");
const prompt = require('prompt');

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

	interface.on('data', (data, encrypted) => {
		console.log(encrypted ? "ENC*:" : "RECV:", data);

		// Buffer encrypted messages
		if (encrypted) {
			encrypted_messages.push(data);

			if (interface.pair) {
				return ;
			}

			interface.pair = true;

			// Send our pairing message
			var pairing_message = [ENTER_PAIRING_MESSAGE, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
			for (var i = 0; i < 16; i++) {
				pairing_message[i+1] = local_secret[i];
			}

			interface.send(pairing_message);

			// Prompt for pin
			prompt.get(['pin'], function (err, result) {
				if (err) { return ; }
				
				interface.key = diffie(parseInt(result.pin, 16), local_secret, remote_secret, encoded_key);
				console.log(interface.key);

				// Rebroadcast the encrypted messages
				encrypted_messages.map((data) => aes.decode(interface.key, data)).forEach((d) => interface.emit('data', d));
			});
		
			return ;
		}

		// This 
		if (data[1] == 0x29) {
			remote_secret = data.slice(2,18);
			encoded_key = data.slice(18, 34);
		}
	});

	interface.on('disconnect', (data) => {
		console.log("disconnected");
	});
});

prompt.start();
