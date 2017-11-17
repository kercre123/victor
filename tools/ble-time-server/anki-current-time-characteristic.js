var util = require('util');

var bleno = require('bleno');

var BlenoCharacteristic = bleno.Characteristic;

var getUTCMilliseconds = function () {
    var millis = Date.now().toString();
    var buf = Buffer.from(millis);

    return buf;
}

var AnkiCurrentTimeCharacteristic = function() {
  AnkiCurrentTimeCharacteristic.super_.call(this, {
    uuid: 'a5f1be0223c341088f8be4b17216ab35',
    properties: ['read', 'notify'],
    value: null
  });

  this._value = getUTCMilliseconds();
  this._intervalHandle = null;
  this._updateValueCallback = null;
};

util.inherits(AnkiCurrentTimeCharacteristic, BlenoCharacteristic);

AnkiCurrentTimeCharacteristic.prototype.onReadRequest = function(offset, callback) {
  this._value = getUTCMilliseconds();
  console.log('AnkiCurrentTimeCharacteristic - onReadRequest: value = ' + this._value.toString('hex'));

  callback(this.RESULT_SUCCESS, this._value);
};

AnkiCurrentTimeCharacteristic.prototype.onSubscribe = function(maxValueSize, updateValueCallback) {
  console.log('AnkiCurrentTimeCharacteristic - onSubscribe');
  var updateTimeFunc = function () {
      this._value = getUTCMilliseconds();
      if (this._updateValueCallback) {
	  this._updateValueCallback(this._value);
      }
  }.bind(this);
  this._intervalHandle = setInterval(updateTimeFunc, 5 * 1000);
  this._updateValueCallback = updateValueCallback;
};

AnkiCurrentTimeCharacteristic.prototype.onUnsubscribe = function() {
  console.log('AnkiCurrentTimeCharacteristic - onUnsubscribe');
  clearInterval(this._intervalHandle);
  this._intervalHandle = null;
  this._updateValueCallback = null;
};

module.exports = AnkiCurrentTimeCharacteristic;
