const aes = require("./aes.js");
var noble = require('noble');
var math = require('mathjs');
const _sodium = require('libsodium-wrappers');
let sodium = null;

let pingChar;
let writeChar;
let readChar;

let pairingService = "00112233445566778899887766554433";
let pingService = "ABCD";
let interfaceService = "1234";
let pingCharService = "5555";
let readCharService = "4444";
let writeCharService = "3333";

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

        printConnectionMessage("Finished discovering services and characteristics...", "\x1b[31m");

        writeChar.on('data', function(data, isNotification) {
            if(data[4] == 2) {
                keys = sodium.crypto_kx_keypair();

                let buf = ConvertIntToByteBufferLittleEndian(keys.publicKey.length);
                buf.push(2);
                let msg = buf.concat(Array.from(keys.publicKey));

                sendMessage(Buffer.from(msg), false);

                let foreignKey = data.slice(5);

                let clientKeys = sodium.crypto_kx_client_session_keys(keys.publicKey, keys.privateKey, foreignKey);

                    //sharedRx
                    //sharedTx

                var readline = require('readline');

                var rl = readline.createInterface({
                    input: process.stdin,
                    output: process.stdout
                });

                rl.question("Enter pin:", function(pin) {
                    let hashedShared = sodium.crypto_generichash(32, clientKeys.sharedRx, pin);
                    cryptoKeys["decrypt"] = hashedShared;
                    cryptoKeys["encrypt"] = clientKeys.sharedTx;
                    console.log("Decrypt key:");
                    console.log(Buffer.from(hashedShared));
                    console.log("Encrypt key:");
                    console.log(Buffer.from(clientKeys.sharedTx));

                    let verBuf = ConvertIntToByteBufferLittleEndian(32);
                    verBuf.push(3);
                    verHash = sodium.crypto_generichash(32, hashedShared);
                    let verMsg = verBuf.concat(Array.from(verHash));

                    sendMessage(Buffer.from(verMsg), false);
                });
            } else if(data[4] == 3) { 
                let verified = Buffer.from(data.slice(5)).equals(Buffer.from(verHash));

                printConnectionMessage("Secure connection has been established...", "\x1b[32m");

                // Send encrypted message
                sendMessage("Hello, world......", true);
            }
        });
    });
}

function discoverServices(peripheral) {
    console.log("Discovering services...");

    return new Promise(function(resolve, reject) {
        peripheral.discoverServices([pingService, interfaceService], function(error, services) {
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
            
            Promise.all([pingPromise, streamPromise]).then(function(data) {
                resolve([data[0], data[1][0], data[1][1]]);
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

loadSodium().then(function() {
    tryConnect();
});

let nonce = [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1];
function sendMessage(msg, encrypt) {
    console.log("sending Message");

    let buffer = msg;

    if(encrypt) {
        buffer = Buffer.from(sodium.crypto_secretbox_easy(msg, Buffer.from(nonce), cryptoKeys["encrypt"]));

        let msgBuf = ConvertIntToByteBufferLittleEndian(msg.length);
        msgBuf.push(0);

        buffer = Buffer.concat([Buffer.from(msgBuf), buffer]);

        console.log("Sending encrypted");
        console.log(buffer);
    }

    readChar.write(Buffer.concat([Buffer.from([encrypt?1:0]),buffer]), true);
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