const EventEmitter = require('events');
const noble = require('noble');

const Cozmo = require("./cozmo.js");

var emitter = new EventEmitter();

noble.on('stateChange', function(state) {
  if (state === 'poweredOn') {
    noble.startScanning();
  } else {
    noble.stopScanning();
  }
});

noble.on('discover', function (device) {
  var data = device.advertisement.manufacturerData;

  // Check manufacturer ID and data bundle to verify this is a cozmo
  if (!data || data.length < 2 || data.readUInt16BE(0) != 0xBEEF) { return ; }

  // THIS CONTAINS SPECIAL MANUFACTURER CODE
  if (data.length != 10) {
    return ;
  }

  function pad(v) { return (v+0x100000000).toString(16).substr(1) }

  // Display info we found out about cozmo
  var deviceID = pad(data.readUInt32LE(2)) + pad(data.readUInt32LE(6));

  emitter.emit('advertised', { 
    device,
    name: device.advertisement.localName || "<noname>", 
    id: deviceID
  });

  // Scan for characteristics
  device.on('connect', function () { 
    device.discoverServices(); 
  });

  device.on('servicesDiscover', function (services) {
    services.forEach(function (service) {
      // Locate cozmo service ID
      if (service.uuid !== '763dbeef5df1405e8aac51572be5bab3') {
        return ;
      }

      service.on('characteristicsDiscover', function(characteristics) {
        var receive, send;

        characteristics.forEach(function (char) {
          switch (char.uuid) {
            case '763dbee05df1405e8aac51572be5bab3': // TO PHONE
              receive = char;
              break;
            case '763dbee15df1405e8aac51572be5bab3': // TO COZMO
              char.notify(true);
              send = char;
              break ;
          }
        });

        if (!send || !receive) {
          device.disconnect();
          return ;
        }

        emitter.emit('connected', new Cozmo(device, service, send, receive));
      });

      service.discoverCharacteristics();
    });
  });
});

module.exports = emitter;
