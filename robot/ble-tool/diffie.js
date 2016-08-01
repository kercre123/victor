const crypto = require('crypto');
const aesjs = require('aes-js');
const BN = require('bn.js');
const pem = require('pem');
const path = require('path');

var fs = require('fs');
var key = fs.readFileSync(path.join(__dirname, "../certs/diffie.pem"));

// pem doesn't support pulling the generator out of a Dhparam info file
var base;

pem.getDhparamInfo(key, function (_, key) {
	const prime = new BN(key.prime.split(":").join(""), 16)
	const generator = new BN(5);

	base = generator.toRed(BN.mont(prime));
});

function encode_random(data, pin) {
	var pin_buff = new Buffer(4);
	pin_buff.writeUInt32LE(pin);

	var hash = crypto
		.createHash('sha1')
		.update(pin_buff)
		.digest('buffer');

	var cipher = new aesjs.ModeOfOperation.ecb(hash.slice(0,16));
	console.log(cipher.encrypt(data))
	return new BN(cipher.encrypt(data), null, 'le');
}

function diffie(pin, local, remote, encoded) {
	local = encode_random(local, pin);
	remote = encode_random(remote, pin);

	var key = base.redPow(local).redPow(remote).fromRed();
	var cipher = new aesjs.ModeOfOperation.ecb(new Buffer(key.toArray('le').slice(0, 16)));
	
	return cipher.decrypt(encoded);
}

module.exports = diffie;
