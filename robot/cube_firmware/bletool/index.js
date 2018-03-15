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
			// Setup frames for test
			cube.send(new Buffer([
				0x01, 0x00, // Command: Key Frames (0)
				0x00, 0xF8, 0x01, 0x00, // Red
				0xE0, 0x07, 0x00, 0x00, // Green
			]));

		}
	});

	cube.on('data', data => {
		const ONEG = 0x2000;
		const ACCEL_THRESH = 0.10; // Much less than a G

		const x = data.readInt16LE(2) / ONEG;
		const y = data.readInt16LE(4) / ONEG
		const z = data.readInt16LE(6) / ONEG;

		const test = Math.abs(x) < ACCEL_THRESH 
            && Math.abs(y) < ACCEL_THRESH 
            && Math.abs(z-1) < ACCEL_THRESH;

		cube.send(new Buffer(test ? [0x00, 0x00, 0x11, 0x11, 0x11] : [0x00, 0x00]));

		console.log(data);
	});
});
