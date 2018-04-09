const aes = require("./aes.js");
const ankible = require("./ankibluetooth.js");
const os = require('os');
var noble = require('noble');
const _sodium = require('libsodium-wrappers');
var readline = require('readline');
let sodium = null;

let pingChar;
let writeChar;
let readChar;
let secureReadChar;
let secureWriteChar;

let pairingService = "D55E356B59CC42659D5F3C61E9DFD70F";
let pingService = "ABCD";
let interfaceService = "1234";
let secureInterfaceService = "6767";

let pingCharService = "5555";
let readCharService = "7D2A4BDAD29B4152B7252491478C5CD7";
let writeCharService = "30619F2D0F5441BDA65A7588D8C85B45";
let secureReadCharService = "045C81553D7B41BC9DA00ED27D0C8A61";
let secureWriteCharService = "28C35E4CB21843CB97183D7EDE9B5316";

function tryConnect() {
    noble.on('discover', function(peripheral) {
        if(peripheral.advertisement.localName.includes("1dd99b69")) { //1f19f8bb
            console.log("Connecting to " + peripheral.advertisement.localName + "... ");
            peripheral.once('connect', function() { onConnect(peripheral); });
            peripheral.once('disconnect', function() { console.log("peripheralDisconnecting..."); });

            peripheral.connect();
            noble.stopScanning();
        } else {
            console.log("[" + peripheral.advertisement.localName + "]");
        }
    });

    noble.on('stateChange', function(state) {
        if(state == "poweredOn") {
            noble.startScanning([ pairingService ], false);
        }
    });
}

function handleReceivePlainText(data) {
    let MSG_PUBK = 4;
    let MSG_NONCE = 5;
    let MSG_CANCEL = 6;
    let MSG_HANDSHAKE = 7;

    data = Buffer.from(data);

    if(data[0] == MSG_HANDSHAKE) {
        let buf = [7];
        let msg = buf.concat(Array.from(data.slice(1)));
        protocolSend(Buffer.from(msg), false);
    }
    else if(data[0] == MSG_PUBK) {
        keys = sodium.crypto_kx_keypair();

        console.log("--> Received public key");
        let buf = [2]; // initial pairing request
        let msg = buf.concat(Array.from(keys.publicKey));
        protocolSend(Buffer.from(msg), false);

        let foreignKey = data.slice(1);
        let clientKeys = sodium.crypto_kx_client_session_keys(keys.publicKey, keys.privateKey, foreignKey);

        rl = readline.createInterface({
            input: process.stdin,
            output: process.stdout
        });

        rl.question("Enter pin:", function(pin) {
            let hashedShared = sodium.crypto_generichash(32, clientKeys.sharedRx, pin);
            cryptoKeys["decrypt"] = hashedShared;
            cryptoKeys["encrypt"] = clientKeys.sharedTx;

            if(receivedNonce) {
                protocolSend(Buffer.from([1, MSG_NONCE]), false);
            }

            enteredPin = true;
        });
    } else if(data[0] == MSG_NONCE) { 
        decryptNonce = Buffer.from(data.slice(1));
        encryptNonce = Buffer.from(data.slice(1));

        if(enteredPin) {
            protocolSend(Buffer.from([1, MSG_NONCE]), false);
        }

        receivedNonce = true;
    } else if(data[0] == MSG_CANCEL) {
        if(rl != null) {
            rl.close();
            console.log("");
        }

        printConnectionMessage("Victor cancelled pairing...", "\x1b[31m");
    }
}

function handleReceiveEncrypted(ctext) {
    let CRYPTO_PING = 1;
    let CRYPTO_ACCEPTED = 4;
    let CRYPTO_WIFI_SCAN_RESPONSE = 11;
    let CRYPTO_WIFI_CONNECT_RESPONSE = 5;

    let cipher = new Uint8Array(ctext);
    let nonce = new Uint8Array(decryptNonce);

    ctext = Buffer.from(ctext);

    try{
        let data = sodium.crypto_aead_xchacha20poly1305_ietf_decrypt(null, cipher, null, nonce, cryptoKeys["decrypt"]);
        sodium.increment(decryptNonce);

        if(data[0] == CRYPTO_PING) {
            let challenge = ConvertByteBufferToUInt(Buffer.from(data.slice(1)));
            let arr = ConvertIntToByteBufferLittleEndian(challenge + 1);
            arr.unshift(CRYPTO_PING);
            let buf = Buffer.from(arr);

            // send challenge response
            protocolSend(buf, true);
        } else if(data[0] == CRYPTO_ACCEPTED) {
            printConnectionMessage("Secure connection has been established...", "\x1b[32m");

            let wifiScanRequest = Buffer.from([10, 0]);
            printConnectionMessage("Requesting Victor wifi Scan...", "\x1b[32m");
            protocolSend(wifiScanRequest, true);
        } else if(data[0] == CRYPTO_WIFI_SCAN_RESPONSE) {
            let wifiScanResponse = []
            let status = data[1];
            let n = data[2];
            let pos = 3;

            for(let i = 0; i < n; i++) {
                let wifiNetwork = {};
                wifiNetwork["auth"] = data[pos++];
                wifiNetwork["signal_strength"] = data[pos++];
                let ssidSize = data[pos++];

                if(ssidSize == 0) {
                    continue;
                }
                
                wifiNetwork["ssid"] = stringFromUTF8Array(data.slice(pos, pos + ssidSize));
                pos += ssidSize;

                wifiScanResponse.push(wifiNetwork);
            }

            for(let i = 0; i < wifiScanResponse.length; i++) {
                console.log(wifiScanResponse[i]);
            }

            rl.close();
            rl = readline.createInterface({
                input: process.stdin,
                output: process.stdout
            });
    
            rl.question("Enter <ssid password>:", function(line) {
                let ssid = line.split(" ", 2)[0];
                let pw = line.split(" ", 2)[1];
                let wifiCred = Buffer.concat([Buffer.from([5]), 
                                              Buffer.from([ssid.length]), Buffer.from(ssid),
                                              Buffer.from([pw.length]), Buffer.from(pw)]);

                protocolSend(wifiCred, true);
            });

        } else if(data[0] == CRYPTO_WIFI_CONNECT_RESPONSE) {
            console.log("Wifi connection status: " + (data[1]==1)?"true":"false");
        }
    } catch(err) {
        console.log("Failed to decrypt");
        console.log(err);
    }
}

function protocolReceive(data, isEncrypted) {
    if(isEncrypted) {
        bleMsgProtocolSecure.receiveRawBuffer(Array.from(data));
    } else {
        bleMsgProtocol.receiveRawBuffer(Array.from(data));
    }
}

function protocolSend(msg, isEncrypted) {
    if(isEncrypted) {
        let nonce = new Uint8Array(encryptNonce);
        let msgBuffer = Buffer.from(msg);
        let data = new Uint8Array(msgBuffer);

        encrypted = sodium.crypto_aead_xchacha20poly1305_ietf_encrypt(data, null, null, nonce, cryptoKeys["encrypt"]);

        sodium.increment(encryptNonce);
        buffer = Buffer.from(encrypted);

        bleMsgProtocolSecure.sendMessage(Array.from(buffer));
    } else {
        bleMsgProtocol.sendMessage(Array.from(msg));
    }
}

function initBleProtocol() {
    bleMsgProtocol = new ankible.BleMessageProtocol(maxPacketSize);
    bleMsgProtocolSecure = new ankible.BleMessageProtocol(maxPacketSize);

    bleMsgProtocol.onReceiveMsg(handleReceivePlainText);
    bleMsgProtocolSecure.onReceiveMsg(handleReceiveEncrypted);

    bleMsgProtocol.onSendRaw(function(buffer) {
        sendMessage(Buffer.from(buffer), false);
    });

    bleMsgProtocolSecure.onSendRaw(function(buffer) {
        sendMessage(Buffer.from(buffer), true);
    });
}

function onConnect(peripheral) {
    printConnectionMessage("Unsecure connection complete...", "\x1b[31m");

    initBleProtocol();

    discoverServices(peripheral).then(function(characteristics) {
        //pingChar = characteristics[0];
        // writeChar = characteristics[0];
        // readChar = characteristics[1];
        // secureWriteChar = characteristics[2];
        // secureReadChar = characteristics[3];

        // console.log("w: " + writeChar.uuid);
        // console.log("r: " + readChar.uuid);
        // console.log("sw: " + secureWriteChar.uuid);
        // console.log("sr: " + secureReadChar.uuid);

        printConnectionMessage("Finished discovering services and characteristics...", "\x1b[31m");

        secureWriteChar.on('data', function(ctext, isNotification) {
            protocolReceive(ctext, true);
        });
        writeChar.on('data', function(data, isNotification) {
            protocolReceive(data, false);
        });
    });
}

function discoverServices(peripheral) {
    console.log("Discovering services...");

    return new Promise(function(resolve, reject) {
        peripheral.discoverServices([pairingService], function(error, services) {
            console.log("Discovering characteristics...");

            let streamPromise = new Promise(function(resolve, reject) {
                services[0].discoverCharacteristics([writeCharService, readCharService, secureWriteCharService, secureReadCharService], function(error, characteristics) {
                    if(error != undefined) {
                        reject();
                    } else {
                        writeChar = characteristics[0];
                        readChar = characteristics[1];
                        secureWriteChar = characteristics[2];
                        secureReadChar = characteristics[3];

                        readChar.subscribe();
                        writeChar.subscribe();
                        secureReadChar.subscribe();
                        secureWriteChar.subscribe();
                        
                        resolve([writeChar, readChar, secureWriteChar, secureReadChar]);
                    }
                });
            });
            
            Promise.all([streamPromise]).then(function(data) {
                resolve([data[0], data[1], data[2], data[3]]);
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

let maxPacketSize = 20;
let bleMsgProtocol = new ankible.BleMessageProtocol(maxPacketSize);
let bleMsgProtocolSecure = new ankible.BleMessageProtocol(maxPacketSize);

loadSodium().then(function() {
    tryConnect();
});

function testMessageProtocol() {
    let bleMsgProtocol = new ankible.BleMessageProtocol(maxPacketSize);

    bleMsgProtocol.onReceiveMsg(function(buffer) {
        console.log("Received: ");
        console.log(buffer);
    });

    bleMsgProtocol.onSendRaw(function(buffer) {
        bleMsgProtocol.receiveRawBuffer(buffer);
    });

    let buffer = [];
    for(let i = 0; i < 40; i++) {
        buffer.push(i);
    }

    bleMsgProtocol.sendMessage(buffer);
}

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

function ConvertByteBufferToUShort(buffer) {
    var buf = new ArrayBuffer(2);
    var view = new DataView(buf);

    buffer.forEach(function (b, i) {
        view.setUint8(i, b);
    });

    return view.getUint16(0);
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

function stringFromUTF8Array(data)
  {
    const extraByteMap = [ 1, 1, 1, 1, 2, 2, 3, 0 ];
    var count = data.length;
    var str = "";
    
    for (var index = 0;index < count;)
    {
      var ch = data[index++];
      if (ch & 0x80)
      {
        var extra = extraByteMap[(ch >> 3) & 0x07];
        if (!(ch & 0x40) || !extra || ((index + extra) > count))
          return null;
        
        ch = ch & (0x3F >> extra);
        for (;extra > 0;extra -= 1)
        {
          var chx = data[index++];
          if ((chx & 0xC0) != 0x80)
            return null;
          
          ch = (ch << 6) | (chx & 0x3F);
        }
      }
      
      str += String.fromCharCode(ch);
    }
    
    return str;
  }