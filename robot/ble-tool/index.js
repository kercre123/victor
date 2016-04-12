const factory = require("./factory.js");
const aes = require("./aes.js");

const BLUE = 0x001F;
const GREEN = 0x03E0;
const RED = 0x7C00;

const PALETTE = {
	red: RED,
	green: GREEN,
	yellow: RED | GREEN,
	blue: BLUE,
	cyan: BLUE | GREEN,
	magenta: BLUE | RED,
	white: BLUE | RED | GREEN,
};


var KEYS = Object.keys(PALETTE);
const key = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
var a = 0, b = 0;

factory.on('discovered', function(info) {

	var color_a = KEYS[a];
	var color_b = KEYS[b];

	if (++a >= KEYS.length) b++, a = 0;

	console.log("Located cozmo:", info.name, `(id: ${info.id}, git: ${info.revision})`, color_a, color_b, color_a)

	color_a = PALETTE[color_a];
	color_b = PALETTE[color_b];

	var message = [
		0x03, 
		0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 
		
		color_a & 0xFF, color_a >> 8, color_a & 0xFF, color_a >> 8, 0x20, 0x20, 0x00, 0x00, 
		color_b & 0xFF, color_b >> 8, color_b & 0xFF, color_b >> 8, 0x20, 0x20, 0x00, 0x00, 
		color_a & 0xFF, color_a >> 8, color_a & 0xFF, color_a >> 8, 0x20, 0x20, 0x00, 0x00, 
		
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	];

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
