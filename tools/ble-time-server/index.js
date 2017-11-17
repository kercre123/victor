var bleno = require('bleno');

var BlenoPrimaryService = bleno.PrimaryService;

// var CurrentTimeCharacteristic = require('./current-time-characteristic');
// var LocalTimeInfoCharacteristic = require('./local-time-info-characteristic');
var AnkiCurrentTimeCharacteristic = require('./anki-current-time-characteristic');
var AnkiTimeZoneNameCharacteristic = require('./anki-time-zone-name-characteristic');

console.log('ble time server');

bleno.on('stateChange', function(state) {
  console.log('on -> stateChange: ' + state);

  if (state === 'poweredOn') {
    bleno.startAdvertising('anki-time', ['5c41d0eb30fd4b2293a3faccd10e987e']);
  } else {
    bleno.stopAdvertising();
  }
});

bleno.on('advertisingStart', function(error) {
  console.log('on -> advertisingStart: ' + (error ? 'error ' + error : 'success'));

  if (!error) {
    bleno.setServices([
      // new BlenoPrimaryService({
      //   uuid: '1805',
      //   characteristics: [
      //       new CurrentTimeCharacteristic(),
      //       new LocalTimeInfoCharacteristic()
      //   ]
      // }),
      new BlenoPrimaryService({
	  uuid: '5c41d0eb30fd4b2293a3faccd10e987e',
	  characteristics: [
	      new AnkiCurrentTimeCharacteristic(),
	      new AnkiTimeZoneNameCharacteristic()
	  ]
      })
    ]);
  }
});


bleno.on('accept', function(clientAddress) {
    console.log('accepted connection from ' + clientAddress);
});

bleno.on('disconnect', function(clientAddress) {
    console.log('disconnected from ' + clientAddress);
});


