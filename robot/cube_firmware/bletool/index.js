const factory = require("./factory");
const fs = require('fs');

factory.on('advertised', (info) => {
  console.log(`Cozmo Cube: ${info.id}`);

  info.device.connect();
});

factory.on('connected', (cube) => {
	cube.upload(fs.readFileSync(process.argv[2]));
	setTimeout(() => {
		cube.send(new Buffer([0x00, 0xFF,0xFF,0xFF, 50, 100,0x00,0x00,0x00, 50, 100]));
		cube.send(new Buffer([0x01, 0xFF,0xFF,0xFF, 50, 100,0x00,0x00,0x00, 50, 100]));
		cube.send(new Buffer([0x02, 0xFF,0xFF,0xFF, 50, 100,0x00,0x00,0x00, 50, 100]));
		cube.send(new Buffer([0x0F, 0xFF,0xFF,0xFF, 50, 100,0x00,0x00,0x00, 50, 100]));
	}, 1000);

	cube.on('disconnect', () => console.log('Disconnected'));
});
