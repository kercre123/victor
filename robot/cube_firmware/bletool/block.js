const EventEmitter = require('events');
const noble = require('noble');

export default class BlockFactory extends EventEmitter {
	constructor() {

	}
}

noble.on('stateChange', function(state) {
  if (state === 'poweredOn') {
    noble.startScanning();
  } else {
    noble.stopScanning();
  }
});

noble.on('discover', function (device) {
	console.log(device);
});
