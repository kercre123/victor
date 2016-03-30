const EventEmitter = require('events');
const util = require('util');

function Cozmo(device, service, send, read) {
  EventEmitter.call(this);

  this._device = device;
  this._service = service;
  this._send_char = send;
  this._read_char = read;
  this._packets = [];

  this._device.on('disconnect', this._disconnect.bind(this));

  this._interval = setInterval(this._dequeue.bind(this), 10);

  // We want to read data as it comes in on this characteristic
  read.on('data', this._recv.bind(this));
  read.notify(true);
}

util.inherits(Cozmo, EventEmitter);

Cozmo.prototype._disconnect = function () {
  clearInterval(this._interval);
  this.emit('disconnect');
};

Cozmo.prototype._recv = function (data) {
  this.emit('data', data);
};

Cozmo.prototype._dequeue = function () {
  if (this._packets.length == 0) { return ;}

  var packet = this._packets.shift();

  console.log("SEND:", packet);
  this._send_char.write(packet);
};

Cozmo.prototype.send = function (message) {
  message = message.concat();
  message.unshift(message.length);

  while (message.length % 16) { message.push(0xCD) }

  this._packets = []

  var packet = null;
  for (var i = 0; i < message.length; i += 16) {
    packet = new Buffer(message.slice(i, i+16).concat(i == 0 ? 1 : 0));
    this._packets.push(packet);
  }

  // Last packet flag
  packet[16] |= 2;
}


module.exports = Cozmo;
