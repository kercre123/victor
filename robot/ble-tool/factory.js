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
  if (!data || data.readUInt16BE(0) != 0xBEEF) { return ; }
  
  var deviceType = data.toString('utf-8', 2, 4);  
  if (deviceType != 'CZ') return ;

  // Display info we found out about cozmo
  var deviceVersion = data.readUInt32LE(4);

  // Scan for characteristics
  device.on('connect', function () { device.discoverServices(); });

  device.on('servicesDiscover', function (services) {
    services.forEach(function (service) {
      // Locate cozmo service ID
      if (service.uuid !== '763dbeef5df1405e8aac51572be5bab3') return ;

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

        emitter.emit('discovered', {
          name: device.advertisement.localName || "<noname>", 
          revision: deviceVersion.toString(16),
          interface: new Cozmo(device, service, send, receive)
        });
      });

      service.discoverCharacteristics();
    });
  });

  device.connect();
});

module.exports = emitter;
