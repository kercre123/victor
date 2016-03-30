const EventEmitter = require('events');
const util = require('util');

function Cozmo(device, service, send, read) {
  EventEmitter.call(this);

  this._device = device;
  this._service = service;
  this._send_char = send;
  this._read_char = read;

  this._device.on('disconnect', this._disconnect.bind(this));

  // We want to read data as it comes in on this characteristic
  read.on('data', this._recv.bind(this));
  read.notify(true);
}

util.inherits(Cozmo, EventEmitter);

Cozmo.prototype._disconnect = function () {
  this.emit('disconnect');
};

Cozmo.prototype._recv = function (data) {
  this.emit('data', data);
};

Cozmo.prototype._send = function (data) {
  this._send_char.write(data);
};


module.exports = Cozmo;
