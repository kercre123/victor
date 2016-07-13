const factory = require("./factory.js");
const aes = require("./aes.js");

var BRYON_COZMO = "db083e35f3031e1d";

factory.on('advertised', (info) => {
	console.log(`Cozmo: ${info.id} (${info.revision})`);

	if (info.id != BRYON_COZMO) { return ; }

	info.device.connect();
});

factory.on('connected', function(interface) {
	console.log("Connected");

	interface.on('data', (data, encrypted) => {
		console.log(encrypted ? "ENC*:" : "RECV:", data.length, data) 
	});

	interface.on('disconnect', (data) => {
		console.log("disconnected");
	});
});
