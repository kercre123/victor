const factory = require("./factory.js");

factory.on('discovered', function(info) {
	console.log("Located cozmo:", info.name, "(" + info.revision + ")")

	/*
	info.interface.send(new Buffer(
		0x0E, 0x28, 
		0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x03
	));

	info.interface.on('data', (data) => console.log("recv:", data) );
	info.interface.on('disconnect', (data) => console.log("disconnected", info.name) );
	*/
});
