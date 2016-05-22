const EventEmitter = require('events');
const util = require('util');
const aes = require("./aes.js");

function Cozmo(device, service, send, read) {
  EventEmitter.call(this);

  this._device = device;
  this._service = service;
  this._send_char = send;
  this._read_char = read;
  this._packets = [];

  this._device.on('disconnect', () => {
    clearInterval(this._interval);
    this.emit('disconnect');    
  });

  this._interval = setInterval(() => {
    if (this._packets.length == 0) { return ;}

    var packet = this._packets.shift();

    console.log("SEND:", packet);
    this._send_char.write(packet);

  }, 10);

  // We want to read data as it comes in on this characteristic
  var buffer = [];
  read.on('data', (data) => {
    var start     = data[16] & 1;
    var end       = data[16] & 2;
    var encrypted = data[16] & 4;

    if (start) { buffer = [] }

    buffer.push(data.slice(0,16));

    if (end) {
      this.emit('data', Buffer.concat(buffer), encrypted);
      buffer = [];
    }
  });

  read.notify(true);
}

util.inherits(Cozmo, EventEmitter);

Cozmo.prototype._disconnect = function () {

};

Cozmo.prototype.send = function (message, key) {
  message = message.concat();
  message.unshift(message.length);

  // Pad out the message
  while (message.length % 16) { message.push(Math.random()*0x100 | 0) }
  
  // Encrypt if nessessary
  if (key) { message = aes.encode(key, message); }
  this._packets = [];

  var packet = null;
  for (var i = 0; i < message.length; i += 16) {
    packet = Buffer.concat([
      new Buffer(message.slice(i, i+16)),
      new Buffer([i == 0 ? 1 : 0])
    ]);

    this._packets.push(packet);
  }

  // Last packet flag
  packet[16] |= 2;

  if (key) { packet[16] |= 4; } // This flag is only used on the last packet
}


module.exports = Cozmo;
