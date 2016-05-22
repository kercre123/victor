const aesjs = require('aes-js');

// This is not crypto safe, but who cares!
function iv() {
	var iv = new Buffer(16);

	for (var i = 0; i < 16; i++) {
		iv[i] = Math.random()*0x100 | 0;
	}
	
	return iv;
}

function encode(key, data) {	
	// Setup IV
	var fb = iv();
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
