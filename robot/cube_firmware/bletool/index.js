const factory = require("./factory");
const fs = require('fs');

factory.on('advertised', (info) => {
  console.log(`Cozmo Cube: ${info.id}`);

  info.device.connect();
});

factory.on('connected', (cube) => {
	console.log("Connected");
	cube.on('disconnect', () => console.log('Disconnected'));

	const fw = fs.readFileSync(process.argv[2]);
	const wanted = fw.slice(0, 0x10).toString('utf8')

	console.log("Desired firmware revision:", wanted);

	cube.on('version', async version => {

		console.log("Loaded version:", version);

		if (wanted !== version) {
			console.log("Loading new image");
			const complete = await cube.upload(fw);
			console.log("Firmware upload complete");
		} else {
			// Send key frame
			cube.send(new Buffer([
				0x01, 0x00, // Command: Key Frames (0)
					0xFF, 0xFF, 10, 50,	// White (10 frame hold. 20 decay)
					0x00, 0x00, 10, 50
			]));

			cube.send(new Buffer([
				0x01, 0x01, // Command: Key Frames (1)
					0x00, 0x00, 15, 0,
					0x00, 0x00, 30, 0,
					0x00, 0x00, 45, 0,
					0x00, 0x00, 60, 0,
			]));

			cube.send(new Buffer([
				0x00, 0x00, // Command: Set frame mapping
					0x45, 0x67, // Initial frames
					0x01, 0x00, 0x11, 0x11	// Frame sequence
			]));
		}
	});
});
