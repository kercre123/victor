const factory = require("./factory.js");
const aes = require("./aes.js");
const keys = require("./keys.js");

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

	interface.on('data', (data, encrypted) => {
		console.log(encrypted ? "ENC*:" : "RECV:", data.length, data);

		console.log("Clock:", (+new Date() - last_msg) / 1000);
		last_msg = +new Date();

		if (!hello) {
			interface.send([ENTER_PAIRING_MESSAGE, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16]);
			hello = true;
		}
	});

	interface.on('disconnect', (data) => {
		console.log("disconnected");
	});
});
