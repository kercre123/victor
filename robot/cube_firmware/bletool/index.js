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

	let uploading = false;
	let timeout = 0;

	async function upload() {
		// Block erasure message causing double upload
		if (uploading) return ;

		console.log("Desired firmware revision:", wanted);
		console.log("Loading new image");
		uploading = true;
		await cube.upload(fw);
		uploading = false;
		console.log("Firmware upload complete");

		timeout = setTimeout(upload, 1000);
	}

	function booted() {
		console.log("Booted");

		const pass = new Buffer([0x00, 0x00, 0x04, 0x04, 0x04, 0x04]);
		const fail = new Buffer([0x00, 0x00, 0x00, 0x01, 0x02, 0x03]);


		// Setup frames for test
		cube.send(new Buffer([
			0x01, 0x00, // Command: Key Frames (0)

			0xFF, 0x00, 0x00, 0x20, 0x08, 0x01, // Red
			0x00, 0xFF, 0x00, 0x20, 0x08, 0x02, // Green
			0x00, 0x00, 0xFF, 0x20, 0x08, 0x03 	// Blue
		]));

		cube.send(new Buffer([
			0x01, 0x03, // Command: Key Frames (0)

			0xFF, 0xFF, 0x00, 0x20, 0x08, 0x00, // Yellow
			0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x04
		]));

		var last = null;

		cube.on('data', data => {
			const ONEG = 0x2000;
			const ACCEL_THRESH = 0.10; // Much less than a G

			const x = data.readInt16LE(2) / ONEG;
			const y = data.readInt16LE(4) / ONEG;
			const z = data.readInt16LE(6) / ONEG;

			const test = Math.abs(x) < ACCEL_THRESH 
	            && Math.abs(y) < ACCEL_THRESH 
	            && Math.abs(z-1) < ACCEL_THRESH;

	        if (last !== test) {
	        	console.log(test);
				cube.send(test ? pass : fail);
	        	last = test;
	        }

	        console.log(data);
		});
	}

	cube.on('version', version => {
		console.log("Loaded version:", version);

		clearTimeout(timeout);

		if (wanted !== version) {
			upload();
		} else {
			booted();
		}
	});
});
