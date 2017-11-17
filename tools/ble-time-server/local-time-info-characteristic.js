var util = require('util');

var bleno = require('bleno');

var moment = require('moment-timezone');

var BlenoCharacteristic = bleno.Characteristic;

var getTimezoneWithDstOffset = function () {
    var d = new Date();
    var buf = Buffer.alloc(2);

    var offset = buf.writeInt8(-1 * (d.getTimezoneOffset() / 15), 0);
    buf.writeUInt8(0, offset);
    return buf;
}

var LocalTimeInfoCharacteristic = function() {
  LocalTimeInfoCharacteristic.super_.call(this, {
    uuid: '2A0F',
    properties: ['read'],
    value: null
  });

  this._value = getTimezoneWithDstOffset();
  this._updateValueCallback = null;
};

util.inherits(LocalTimeInfoCharacteristic, BlenoCharacteristic);

LocalTimeInfoCharacteristic.prototype.onReadRequest = function(offset, callback) {
  this._value = getTimezoneWithDstOffset();
  console.log('LocalTimeInfoCharacteristic - onReadRequest: value = ' + this._value.toString('hex'));

  callback(this.RESULT_SUCCESS, this._value);
};

LocalTimeInfoCharacteristic.prototype.onSubscribe = function(maxValueSize, updateValueCallback) {
  console.log('LocalTimeInfoCharacteristic - onSubscribe');

  this._updateValueCallback = updateValueCallback;
};

LocalTimeInfoCharacteristic.prototype.onUnsubscribe = function() {
  console.log('LocalTimeInfoCharacteristic - onUnsubscribe');

  this._updateValueCallback = null;
};

module.exports = LocalTimeInfoCharacteristic;
