const aesjs = require('aes-js');
const crypto = require('crypto');

function encode(nonce, key, data) {	
	// Append hmac
	data = Buffer.concat([data, crypto.createHmac('md5', nonce).update(data).digest()])

	// Setup IV
	var fb = crypto.randomBytes(16);
	var cipher = new aesjs.ModeOfOperation.cfb(key, fb, 16);

	return Buffer.concat([fb, cipher.encrypt(data)]);
}

function decode(nonce, key, data) {
	var iv = data.slice(0,16);
	var cipher = new aesjs.ModeOfOperation.cfb(key, iv, 16);

	// Decrypt
	data = cipher.decrypt(data.slice(16))

	var hmac = data.slice(data.length - 16);
	data = data.slice(0, data.length - 16);

	const test = hmac.compare(crypto.createHmac('md5', nonce).update(data).digest()) == 0;

	return { data, test };
}

module.exports = {
	encode, decode
};