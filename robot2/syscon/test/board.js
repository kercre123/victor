const SerialPort = require("serialport");
const EventEmitter = require("events");
const crc = require("./crc");


const PAYLOAD_DATA_FRAME  = 0x6466;
const PAYLOAD_CONT_DATA   = 0x6364;
const PAYLOAD_MODE_CHANGE = 0x6d64;
const PAYLOAD_VERSION     = 0x7276;
const PAYLOAD_ACK         = 0x6b61;
const PAYLOAD_ERASE       = 0x7878;
const PAYLOAD_VALIDATE    = 0x7374;
const PAYLOAD_DFU_PACKET  = 0x6675;

class Body extends EventEmitter {
	constructor(target) {
		super();

		this.port = new SerialPort(target, { baudRate: 3000000 });
		this.payload = Buffer.alloc(0);

		this.port.on('data', (data) => this.receive(data));
		this.port.on('error', (err) => console.log(`ERROR: ${err}`));
	}

	send(id, payload = Buffer.alloc(0)) {
		const buffer = Buffer.alloc(payload.length + 12);

		payload.copy(buffer, 8);

		buffer.writeUInt32LE(0x423248AA, 0);
		buffer.writeInt16LE(id, 4);
		buffer.writeInt16LE(payload.length, 6);
		buffer.writeInt32LE(crc(payload), payload.length + 8);

		console.log("-->", id, payload);

		this.port.write(buffer);
	}

	receive (data) {
		this.payload = Buffer.concat([this.payload, data]);

		do {
			var offset = 0;

			// Find sync header
			do {
				if (this.payload.length - offset < 8) return ;
				if (this.payload.readUInt32LE(offset) == 0x483242AA) break ;
				offset++;
			} while (true);

			// Trim Payload
			this.payload = this.payload.slice(offset);
			if (offset) console.log(`Discarded ${offset} bytes`);

			// Payload settings
			const payloadID = this.payload.readUInt16LE(4);
			const bytesToFollow = this.payload.readUInt16LE(6);
			const size = bytesToFollow + 12;

			if (size > this.payload.length) return ;

			const payloadCRC = this.payload.readInt32LE(8+bytesToFollow);
			const target = this.payload.slice(8, 8+bytesToFollow);
			var expectedCRC = crc(target);


			if (payloadCRC !== expectedCRC) {
				console.error("CRC Failed");
				this.payload = this.payload.slice(4);
				continue ;
			}

			this.payload = this.payload.slice(size);
			this.emit('data', { id: payloadID, data: target } );
		} while (this.payload.length > 0);
	}
}
module.exports = Body;

Body.PAYLOAD_DATA_FRAME  = 0x6466;
Body.PAYLOAD_CONT_DATA   = 0x6364;
Body.PAYLOAD_MODE_CHANGE = 0x6d64;
Body.PAYLOAD_VERSION     = 0x7276;
Body.PAYLOAD_ACK         = 0x6b61;
Body.PAYLOAD_ERASE       = 0x7878;
Body.PAYLOAD_VALIDATE    = 0x7374;
Body.PAYLOAD_DFU_PACKET  = 0x6675;
