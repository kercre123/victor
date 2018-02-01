const aes = require("./aes.js");
const os = require('os');
var noble = require('noble');
var math = require('mathjs');
const _sodium = require('libsodium-wrappers');
let sodium = null;

let pingChar;
let writeChar;
let readChar;
let secureReadChar;
let secureWriteChar;

let pairingService = "00112233445566778899887766554433";
let pingService = "ABCD";
let interfaceService = "1234";
let secureInterfaceService = "6767";

let pingCharService = "5555";
let readCharService = "4444";
let writeCharService = "3333";
let secureReadCharService = "6677";
let secureWriteCharService = "7766";

function tryConnect() {
    noble.on('discover', function(peripheral) {
        console.log("Connecting to " + peripheral.advertisement.localName + "... ");
        peripheral.once('connect', function() { onConnect(peripheral); });

        peripheral.connect();
    });

    noble.on('stateChange', function(state) {
        if(state == "poweredOn") {
            noble.startScanning([ pairingService ], false);
        }
    });
}

function onConnect(peripheral) {
    printConnectionMessage("Unsecure connection complete...", "\x1b[31m");

    discoverServices(peripheral).then(function(characteristics) {
        let pingChar = characteristics[0];
        let writeChar = characteristics[1];
        let readChar = characteristics[2];
        let secureWriteChar = characteristics[3];
        let secureReadChar = characteristics[4];

        printConnectionMessage("Finished discovering services and characteristics...", "\x1b[31m");

        secureWriteChar.on('data', function(ctext, isNotification) {
            let CRYPTO_PING = 1;
            let CRYPTO_ACCEPTED = 4;

            console.log("cipher:");
            console.log(ctext);

            let cipher = new Uint8Array(ctext);
            let nonce = new Uint8Array(decryptNonce);

            try{
                let data = sodium.crypto_aead_xchacha20poly1305_ietf_decrypt(null, cipher, null, nonce, cryptoKeys["decrypt"]);
                sodium.increment(decryptNonce);

                if(data[0] == CRYPTO_PING) {
                    let challenge = ConvertByteBufferToUInt(Buffer.from(data.slice(1)));
                    let arr = ConvertIntToByteBufferLittleEndian(challenge + 1);
                    arr.unshift(CRYPTO_PING);
                    let buf = Buffer.from(arr);
    
                    // send challenge response
                    sendMessage(buf, true);
                } else if(data[0] == CRYPTO_ACCEPTED) {
                    printConnectionMessage("Secure connection has been established...", "\x1b[32m");

                    let wifiCred = Buffer.concat([Buffer.from([5]), Buffer.from([13]), Buffer.from("vic-home-wifi"), Buffer.from([8]), Buffer.from("password")]);

                    console.log(wifiCred);
                    sendMessage(wifiCred, true);
                }
            } catch(err) {
                console.log("Failed to decrypt");
                console.log(err);
            }


        });

        writeChar.on('data', function(data, isNotification) {
            let MSG_PUBK = 4;
            let MSG_NONCE = 5;

            if(data[0] == MSG_PUBK) {
                keys = sodium.crypto_kx_keypair();

                // let buf = ConvertIntToByteBufferLittleEndian(keys.publicKey.length);
                // buf.push(2);
                let buf = [2]; // initial pairing request
                let msg = buf.concat(Array.from(keys.publicKey));

                sendMessage(Buffer.from(msg), false);

                let foreignKey = data.slice(1);
                let clientKeys = sodium.crypto_kx_client_session_keys(keys.publicKey, keys.privateKey, foreignKey);

                var readline = require('readline');

                var rl = readline.createInterface({
                    input: process.stdin,
                    output: process.stdout
                });

                rl.question("Enter pin:", function(pin) {
                    let hashedShared = sodium.crypto_generichash(32, clientKeys.sharedRx, pin);
                    cryptoKeys["decrypt"] = hashedShared;
                    cryptoKeys["encrypt"] = clientKeys.sharedTx;

                    if(receivedNonce) {
                        sendMessage(Buffer.from([1, MSG_NONCE]), false);
                    }

                    enteredPin = true;
                });
            } else if(data[0] == MSG_NONCE) { 
                decryptNonce = Buffer.from(data.slice(1));
                encryptNonce = Buffer.from(data.slice(1));

                if(enteredPin) {
                    sendMessage(Buffer.from([1, MSG_NONCE]), false);
                }

                receivedNonce = true;
            }
        });
    });
}

function discoverServices(peripheral) {
    console.log("Discovering services...");

    return new Promise(function(resolve, reject) {
        peripheral.discoverServices([pingService, interfaceService, secureInterfaceService], function(error, services) {
            console.log("Discovering characteristics...");
            let pingPromise = new Promise(function(resolve, reject) {
                services[0].discoverCharacteristics([pingCharService], function(error, characteristics) {
                    if(error != undefined) {
                        reject();
                    } else {
                        pingChar = characteristics[0];
                        console.log("Subscribe to ping...");
                        pingChar.subscribe();
                        resolve(pingChar);
                    }
                });
            });

            let streamPromise = new Promise(function(resolve, reject) {
                services[1].discoverCharacteristics([writeCharService, readCharService], function(error, characteristics) {
                    if(error != undefined) {
                        reject();
                    } else {
                        if(characteristics[0].uuid == readCharService) {
                            readChar = characteristics[0];
                            writeChar = characteristics[1];
                        } else {
                            writeChar = characteristics[0];
                            readChar = characteristics[1];
                        }

                        readChar.subscribe();
                        writeChar.subscribe();
                        
                        resolve([writeChar, readChar]);
                    }
                });
            });

            let secureStreamPromise = new Promise(function(resolve, reject) {
                services[2].discoverCharacteristics([secureWriteCharService, secureReadCharService], function(error, characteristics) {
                    if(error != undefined) {
                        reject();
                    } else {
                        if(characteristics[0].uuid == secureReadCharService) {
                            secureReadChar = characteristics[0];
                            secureWriteChar = characteristics[1];
                        } else {
                            secureWriteChar = characteristics[0];
                            secureReadChar = characteristics[1];
                        }

                        secureReadChar.subscribe();
                        secureWriteChar.subscribe();
                        
                        resolve([secureWriteChar, secureReadChar]);
                    }
                });
            });
            
            Promise.all([pingPromise, streamPromise, secureStreamPromise]).then(function(data) {
                resolve([data[0], data[1][0], data[1][1], data[2][0], data[2][1]]);
            });
        });
    });
}

function printConnectionMessage(message, color) {
    console.log(color + '%s',        "========================================================================")
    console.log(message);
    console.log(color + '%s\x1b[0m', "========================================================================")
}

async function loadSodium() {
    await _sodium.ready;
    sodium = _sodium;
}

let keys = null;
let cryptoKeys = {};
let verHash = [];
let enteredPin = false;
let receivedNonce = false;

loadSodium().then(function() {
    tryConnect();
});

function testEncryption() {
    let serverKeys = sodium.crypto_kx_keypair();
    let clientKeys = sodium.crypto_kx_keypair();

    let clientSessionKeys = sodium.crypto_kx_client_session_keys(clientKeys.publicKey, clientKeys.privateKey, serverKeys.publicKey);
    let serverSessionKeys = sodium.crypto_kx_server_session_keys(serverKeys.publicKey, serverKeys.privateKey, clientKeys.publicKey);

    let nonce = sodium.randombytes_buf(sodium.crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);

    let msg0 = "Hello, world!";

    let ciphertext = sodium.crypto_aead_xchacha20poly1305_ietf_encrypt(msg0, null, null, nonce, serverSessionKeys.sharedTx);
    let msg1 = Buffer.from(sodium.crypto_aead_xchacha20poly1305_ietf_decrypt(null, ciphertext, null, nonce, clientSessionKeys.sharedRx)).toString('utf8');

    console.log("Encryption test: " + (msg0==msg1));
    console.log(ciphertext);
    console.log(nonce);
    console.log(clientSessionKeys.sharedRx);
    console.log("Nonce length: " + nonce.length);
    console.log("Key length: " + clientSessionKeys.sharedRx.length);
    console.log("");
}

let encryptNonce;
let decryptNonce;
function sendMessage(msg, encrypt) {
    let buffer = msg;

    if(encrypt) {
        let nonce = new Uint8Array(encryptNonce);
        let msgBuffer = Buffer.from(msg);
        let data = new Uint8Array(msgBuffer);

        encrypted = sodium.crypto_aead_xchacha20poly1305_ietf_encrypt(data, null, null, nonce, cryptoKeys["encrypt"]);

        sodium.increment(encryptNonce);

        buffer = Buffer.from(encrypted);
    }

    if(encrypt) {
        secureReadChar.write(buffer, true);
    } else {
        readChar.write(buffer, true);
    }
}

function onDisconnect(event) {
    console.log("device disconnected: " + event.target.id);
}

function onPing(event) {
	var byteBuffer = new Uint8Array( event.target.value["buffer"] );
}

function onRead(event) {
    // outgoing traffic to peripheral
	var byteBuffer = new Uint8Array( event.target.value["buffer"] );
}

function onWrite(event) {
    // incoming traffic from peripheral
	var byteBuffer = new Uint8Array( event.target.value["buffer"] );
}

function ping(event) {
	var byteBuffer = new Uint8Array( event.target.value["buffer"] );
}

function ConvertFloatToByteBufferLittleEndian(val) {
    let floatArray = new Float32Array(1);
    floatArray[0] = val;
    return new Uint8Array(floatArray.buffer);
}

function ConvertByteBufferToShort(buffer) {
    var buf = new ArrayBuffer(2);
    var view = new DataView(buf);

    buffer.forEach(function (b, i) {
        view.setUint8(i, b);
    });

    return view.getInt16(0);
}

function ConvertByteBufferToUInt(buffer) {
    var buf = new ArrayBuffer(4);
    var view = new DataView(buf);

    console.log("Endianness: " + os.endianness());

    buffer.forEach(function (b, i) {
        view.setUint8(i, b);
    });

    return view.getUint32(0, true);
}


function ConvertByteBufferToDouble(buffer) {
    // Create a buffer
    var buf = new ArrayBuffer(4);
    // Create a data view of it
    var view = new DataView(buf);

    // set bytes
    buffer.forEach(function (b, i) {
        view.setUint8(i, b);
    });

    // Read the bits as a float; note that by doing this, we're implicitly
    // converting it from a 32-bit float into JavaScript's native 64-bit double
    return view.getFloat32(0);
}

function ConvertShortToByteBufferLittleEndian(val) {
    let buffer = new Array(2);
    buffer[0] = val & 0x00FF;
    buffer[1] = val >> 8;
    return buffer;
}

function ConvertIntToByteBufferLittleEndian(val) {
    let buffer = new Array(4);
    buffer[0] = val & 0x000000FF;
    buffer[1] = (val >> 8) & 0x0000FF;
    buffer[2] = (val >> 16) & 0x00FF;
    buffer[3] = val >> 24;
    return buffer;
}