const fs = require("fs");
const path = require("path");
const Body = require("./board");

const body = new Body(process.argv[2]);

function* send() {
	// Get current version
	body.send(Body.PAYLOAD_VERSION);
	yield null;

	// Erase our application
	body.send(Body.PAYLOAD_ERASE);
	yield null;

	const bin = fs.readFileSync(path.join(__dirname, "../build/syscon.bin")).slice(0x10);
	var payload = Buffer.alloc(0x404);

	// Write our image
	for (var i = 0; i < bin.length; i += (payload.length - 4)) {
		const bytes = bin.length - i;

		payload.writeUInt16LE(i, 0);
		payload.writeUInt16LE(Math.min(0x400, bytes) / 4, 2);
		bin.copy(payload, 4, i);

		body.send(Body.PAYLOAD_DFU_PACKET, payload);
		yield null;
	}

	// Validate the image
	body.send(Body.PAYLOAD_VALIDATE, payload);
	yield null;

	// Start running the application
	body.send(Body.PAYLOAD_MODE_CHANGE);
	yield null;
}

const flow = send();
flow.next();

body.on('data', (info) => {
	switch (info.id) {
	case Body.PAYLOAD_ACK:
		{
			const value = info.data.readInt32LE(0);
			console.log(value < 0 ? "NACK" : "ACK", value);
		}
		break ;
	default:
		console.log("<--", info.id, info.data);
		break ;
	}
	flow.next();
});
