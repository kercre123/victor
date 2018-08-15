const EventEmitter = require('events');
const noble = require('noble');
const Cube = require('./cube');

var emitter = new EventEmitter();

const CUBE_SERVICE = 'c6f6c70fd219598bfb4c308e1f22f830';

noble.on('stateChange', function(state) {
  console.log(state);
  if (state === 'poweredOn') {
    noble.startScanning([CUBE_SERVICE]);  // Only cubes, allow duplicates
  } else {
    noble.stopScanning();
  }
});

noble.on('discover', function (device) {
  var data = device.advertisement.manufacturerData;

  console.log(device.address);

  emitter.emit('advertised', {
    device,
    name: device.advertisement.localName || "<noname>",
    id: data.slice(2)
  });

  // Scan for characteristics
  device.on('connect', function () {
    device.discoverServices();
  });

  device.on('servicesDiscover', function (services) {
    services.forEach(function (service) {
      // Locate cozmo service ID
      if (service.uuid !== CUBE_SERVICE) {
        return ;
      }

      service.on('characteristicsDiscover', function(characteristics) {
        emitter.emit('connected', new Cube(device, characteristics));
      });

      service.discoverCharacteristics();
    });
  });
});

module.exports = emitter;
