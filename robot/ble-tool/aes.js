const aesjs = require('aes-js');
const crypto = require('crypto');

function encode(key, data) {	
	// Setup IV
	var fb = crypto.randomBytes(16);
	var cipher = new aesjs.ModeOfOperation.cfb(key, fb, 16);

	return Buffer.concat([fb, cipher.encrypt(data)]);
}

function decode(key, data) {
	var iv = data.slice(0,16);
	var cipher = new aesjs.ModeOfOperation.cfb(key, iv, 16);

	return cipher.decrypt(data.slice(16));
}

module.exports = {
	encode, decode
};
