const factory = require("./factory.js");
const aes = require("./aes.js");

var message = [
	0x03, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
	0x80, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x80, 
	0x80, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0x80, 0x80
];

const key = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];

factory.on('discovered', function(info) {
	console.log("Located cozmo:", info.name, "(" + info.revision + ")")

	//info.interface.send(message, key);
	info.interface.send(message);

	info.interface.on('data', (data, encrypted) => {
		if (encrypted) {
			console.log("ENC*:", data.length, data) 
			data = aes.decode(key, data);
		}

		console.log("RECV:", data.length, data) 
	});
	info.interface.on('disconnect', (data) => console.log("disconnected", info.name) );
});
