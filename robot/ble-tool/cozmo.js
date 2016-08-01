const EventEmitter = require('events');
const util = require('util');
const aes = require("./aes.js");
const crypto = require('crypto');

class Cozmo extends EventEmitter {
  constructor(device, service, send, read) {
    super();

    this._device = device;
    this._service = service;
    this._send_char = send;
    this._read_char = read;
    this._packets = [];
    this._encoded = [];
    
    this.nonce = new Buffer([]);

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
        var packet = Buffer.concat(buffer);

        if (encrypted) {
          if (!this.key) {
            this._encoded.push(packet);
            this.emit('encrypted', packet);
            return ;
          }

          this._processEncrypted(packet);
          return ;
        }

        this.emit('data', packet, true);

        buffer = [];
      }
    });

    read.notify(true);
  }

  _processEncrypted (packet) {
    var decoded = aes.decode(this.nonce, this.key, packet);

    setTimeout(() => this.emit('data', decoded.data, decoded.test), 0);

    return decoded.test;
  }

  setNonce (nonce) {
    this.nonce = new Buffer(nonce);
    this._encoded = this._encoded.filter((d) => !this._processEncrypted(d));
  }

  setKey (key) {
    this.key = new Buffer(key);
    this._encoded = this._encoded.filter((d) => !this._processEncrypted(d));
  }

  send (tag, buffer) {
    // Frame our message
    buffer = Buffer.concat([new Buffer([buffer.length, tag]), buffer]);
    var message = crypto.randomBytes((buffer.length + 15) & ~0xF);
    buffer.copy(message);

    // Encrypt if nessessary
    if (this.key) {
      message = aes.encode(this.nonce, this.key, message);
    }

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

    if (this.key) { packet[16] |= 4; } // This flag is only used on the last packet
  }
}

module.exports = Cozmo;
