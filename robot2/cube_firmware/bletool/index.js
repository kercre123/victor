const factory = require("./factory");
const fs = require('fs');

factory.on('advertised', (info) => {
  console.log(`Cozmo Cube: ${info.id}`);

  info.device.connect();
});

factory.on('connected', (cube) => {
	cube.upload(fs.readFileSync(process.argv[2]));
	cube.send(new Buffer([1,2,3,4]));

	cube.on('disconnect', () => console.log('Disconnected'));
})
