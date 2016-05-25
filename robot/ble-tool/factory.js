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
  
  // Scan for characteristics
  device.on('connect', function () { device.discoverServices(); });

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

        function pad(v) { while (v.length < 8) v = "0" + v; return v }

        // Display info we found out about cozmo
        var deviceVersion = data.readUInt32LE(2);
        var deviceID0 = pad(data.readUInt32LE(6).toString(16));
        var deviceID1 = pad(data.readUInt32LE(10).toString(16));

        emitter.emit('discovered', {
          name: device.advertisement.localName || "<noname>", 
          revision: deviceVersion.toString(16),
          id: deviceID0 + deviceID1,
          interface: new Cozmo(device, service, send, receive)
        });
      });

      service.discoverCharacteristics();
    });
  });

  device.connect();
});

module.exports = emitter;
