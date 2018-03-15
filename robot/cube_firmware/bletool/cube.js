const EventEmitter = require('events');

const OTA_TARGET_UUID = '9590ba9c514092b518445f9d681557a4';
const APP_VERSION_UUID = '450aa1758d8516a69148d50e2eb7b79e';
const APP_WRITE_UUID = '0ea752906759a58d7948598c4e02d94a';
const APP_READ_UUID = '43ef14af5fb17b8136472a9477824cab';

const MAX_BYTES_PER_PACKET = 20;

class Cube extends EventEmitter {
	constructor(device, characteristics) {
		super();

		this._device = device;

    characteristics.forEach((char) => {
    if (char.properties.indexOf('notify')) {
      char.subscribe();
    }

  	switch (char.uuid) {
    	case OTA_TARGET_UUID:
      	this._ota_target = char;
      	break ;
    	case APP_VERSION_UUID:
    		char.on('data', (data) =>  this.emit('version', data.toString('utf8')));
    		char.read();
      	break ;
    	case APP_WRITE_UUID:
      	this._app_write = char;
      	break ;
    	case APP_READ_UUID:
      	char.on('data', (data) => this.emit('data', data));
      	break ;
			}
    });

    this._device.on('disconnect', () => {
      this.emit('disconnect');
    });
	}

	send(packet) {
		this._app_write.write(packet);
	}

  send_ota(packet) {
    return new Promise((accept, reject) => {
      console.log(packet);
      this._ota_target.write(packet, false, accept);
    });
  }

  async upload(data) {
		let offset = 0x10;

		while (offset < data.length) {
      const packet = data.slice(offset, offset + MAX_BYTES_PER_PACKET);
      await this.send_ota(packet);
      offset += packet.length;
		}
  }
}

module.exports = Cube;
