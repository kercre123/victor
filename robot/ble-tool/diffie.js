const crypto = require('crypto');
const aesjs = require('aes-js');
const BN = require('bn.js');
const pem = require('pem');
const path = require('path');

var fs = require('fs');
var key = fs.readFileSync(path.join(__dirname, "../certs/diffie.pem"));

var generator = new BN(5);
var prime;

pem.getDhparamInfo(key, function (_, key) {
	prime = new BN(key.prime.split(":").join(""), 16)
});

function encode_random(data, pin) {
	var pin_buff = new Buffer(4);
	pin_buff.writeInt32LE(pin);

	var hash = crypto.createHash('sha1');
	hash.update(pin_buff);

	var cipher = new aesjs.ModeOfOperation.ecb(hash.digest('buffer').slice(0,16));
	return new BN(cipher.encrypt(data), null, 'le');
}

function diffie(pin, local, remote, encoded) {
	local = encode_random(local, pin);
	remote = encode_random(remote, pin);

	var key = generator.toRed(BN.mont(prime)).redPow(local).redPow(remote).fromRed();
	var cipher = new aesjs.ModeOfOperation.ecb(new Buffer(key.toArray('le').slice(0, 16)));
	
	return cipher.decrypt(encoded);
}

module.exports = diffie;
