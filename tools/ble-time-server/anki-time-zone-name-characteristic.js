var util = require('util');

var bleno = require('bleno');

var moment = require('moment-timezone');

var BlenoCharacteristic = bleno.Characteristic;

var getTimeZoneName = function () {
    var tz = moment.tz.guess();
    var buf = Buffer.from(tz);

    return buf;
}

var AnkiTimeZoneNameCharacteristic = function() {
  AnkiTimeZoneNameCharacteristic.super_.call(this, {
    uuid: 'f95096be90144c2aab0c2efe6c2076cc',
    properties: ['read'],
    value: null
  });

  this._value = getTimeZoneName();
  this._intervalHandle = null;
  this._updateValueCallback = null;
};

util.inherits(AnkiTimeZoneNameCharacteristic, BlenoCharacteristic);

AnkiTimeZoneNameCharacteristic.prototype.onReadRequest = function(offset, callback) {
  this._value = getTimeZoneName();
  console.log('AnkiTimeZoneNameCharacteristic - onReadRequest: value = ' + this._value.toString('hex'));

  callback(this.RESULT_SUCCESS, this._value);
};

module.exports = AnkiTimeZoneNameCharacteristic;
