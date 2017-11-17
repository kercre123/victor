var util = require('util');

var bleno = require('bleno');

var BlenoCharacteristic = bleno.Characteristic;

var getExactTime256 = function () {
    var d = new Date();
    var buf = Buffer.alloc(10);

    var offset = buf.writeUInt16LE(d.getFullYear(), 0);
    offset = buf.writeUInt8(d.getMonth() + 1, offset);
    offset = buf.writeUInt8(d.getDate(), offset);
    offset = buf.writeUInt8(d.getHours(), offset);
    offset = buf.writeUInt8(d.getMinutes(), offset);
    offset = buf.writeUInt8(d.getSeconds(), offset);
    offset = buf.writeUInt8(d.getDay() + 1, offset);
    offset = buf.writeUInt8(Math.round(d.getMilliseconds() * 0.255), offset);
    offset = buf.writeUInt8(0, offset); // no update reason
    return buf;
}

var CurrentTimeCharacteristic = function() {
  CurrentTimeCharacteristic.super_.call(this, {
    uuid: '2A2B',
    properties: ['read', 'notify'],
    value: null
  });

  this._value = getExactTime256();
  this._intervalHandle = null;
  this._updateValueCallback = null;
};

util.inherits(CurrentTimeCharacteristic, BlenoCharacteristic);

CurrentTimeCharacteristic.prototype.onReadRequest = function(offset, callback) {
  this._value = getExactTime256();
  console.log('CurrentTimeCharacteristic - onReadRequest: value = ' + this._value.toString('hex'));

  callback(this.RESULT_SUCCESS, this._value);
};

CurrentTimeCharacteristic.prototype.onSubscribe = function(maxValueSize, updateValueCallback) {
  console.log('CurrentTimeCharacteristic - onSubscribe');
  var updateTimeFunc = function () {
      this._value = getExactTime256();
      if (this._updateValueCallback) {
	  this._updateValueCallback(this._value);
      }
  }.bind(this);
  this._intervalHandle = setInterval(updateTimeFunc, 5 * 1000);
  this._updateValueCallback = updateValueCallback;
};

CurrentTimeCharacteristic.prototype.onUnsubscribe = function() {
  console.log('CurrentTimeCharacteristic - onUnsubscribe');
  clearInterval(this._intervalHandle);
  this._intervalHandle = null;
  this._updateValueCallback = null;
};

module.exports = CurrentTimeCharacteristic;
