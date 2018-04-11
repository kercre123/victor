#! /usr/bin/env node

/**
 *  ble server
 *
 */
const ankible = require("./ankibluetooth.js");
const os = require('os');
var noble = require('noble');

var net = require('net'),
    unixsocket = __dirname + '/uds_socket';

var main_socket;
var double_clicked = false;

var plain_end_token = Buffer.from("<UNENCRYPTED_END>");
var encrypt_end_token = Buffer.from("<UNENCRYPTED_END>");

if (process.argv.length < 3) {
    console.log("Must provide id to this script!");
    process.exit();
}
var robot_id = process.argv[2];
console.log("> Looking for " + robot_id);

var log = function(who, what) {
  return function() {
    var args = Array.prototype.slice.call(arguments);
    console.log('[%s on %s]', who, what, args);
  };
};

var send_ble = function(socket, data, encrypted_channel) {
  protocolSend(data, encrypted_channel);
};

var echo = function(socket) {
  /**
   *  net.Socket (http://nodejs.org/api/net.html#net_class_net_socket)
   *  events: end, data, end, timeout, drain, error, close
   *  methods:
   */
  socket.on('end', function() {
    // exec'd when socket other end of connection sends FIN packet
    console.log('> [socket on end]');
  });
  socket.on('data', function(data) {
    // data is a Buffer object
    var buf = new Buffer("check_for_double_click");
    var data_slice = data.slice(0, data.length - encrypt_end_token.length);
    if (buf.equals(data_slice)) {
        var result = new Buffer("check_for_double_click");
        if (double_clicked) {
            var clicked = Buffer.from("1");
            result = Buffer.concat([result, clicked]);
        } else {
            var not_clicked = Buffer.from("0");
            result = Buffer.concat([result, not_clicked]);
        }
        result = Buffer.concat([result, plain_end_token]);
        main_socket.write(result);
    } else {
        var encrypted = false;
        var data_slice = data.slice(0, data.length - encrypt_end_token.length);
        if (data.slice(data.length - encrypt_end_token.length).equals(encrypt_end_token)) {
            encrypted = true;
        } else {
            data_slice = data.slice(0, data.length - plain_end_token.length);
        }
        send_ble(socket, data_slice, encrypted);
    }
  });
  socket.on('end', function() {
    // emitted when the other end sends a FIN packet
  });

  socket.on('timeout', log('socket', 'timeout'));

  socket.on('drain', function() {
    // emitted when the write buffer becomes empty
    console.log('[socket on drain]');
  });
  socket.on('error', log('socket', 'error'));
  socket.on('close', log('socket', 'close'));
};

/**
 *  net.Server (http://nodejs.org/api/net.html#net_class_net_server)
 *  events: listening, connections, close, err
 *  methods: listen, address, getConnections,  
 */
var server = net.createServer(echo);
server.listen(unixsocket);

server.on('listening', function() {
  var ad = server.address();
  console.log('[server on listening] %s', ad);
});

server.on('connection', function(socket) {
  server.getConnections(function(err, count) {
    console.log('%d open connections!', count);
  });
  main_socket = socket;


  tryConnect();
  // // TODO: put this in the code the receives messages from Victor
  // data = Buffer.from([0x01, 0x00, 0x00, 0x00, 0x01]);
  // end_token = Buffer.from("<END>");
  // result = Buffer.concat([data, end_token]);
  // socket.write(result);
});

server.on('close', function() { console.log('[server on close]'); });
server.on('err', function(err) { 
  console.log(err);
  server.close(function() { console.log("> shutting down the server!"); });
});








// BLE CODE

// const aes = require("./aes.js");
// const ankible = require("./ankibluetooth.js");
// const os = require('os');
// var noble = require('noble');
// var math = require('mathjs');
// const _sodium = require('libsodium-wrappers');
// let sodium = null;

let pingChar;
let writeChar;
let readChar;
// let secureReadChar;
// let secureWriteChar;

let pairingService = "FEE3";
// let pairingService = "0000FEE300001000800000805F9B34FB";
// let pairingService = "D55E356B59CC42659D5F3C61E9DFD70F";
let pingService = "ABCD";
let interfaceService = "1234";
// let secureInterfaceService = "6767";

let pingCharService = "5555";
let readCharService = "7D2A4BDAD29B4152B7252491478C5CD7";
let writeCharService = "30619F2D0F5441BDA65A7588D8C85B45";
// let secureReadCharService = "045C81553D7B41BC9DA00ED27D0C8A61";
// let secureWriteCharService = "28C35E4CB21843CB97183D7EDE9B5316";

// allow for duplicates
// stop scanning after double click received

function tryConnect() {
    var found = false;
    noble.on('discover', function(peripheral) {
        if(peripheral.advertisement.localName && peripheral.advertisement.localName.includes(robot_id)){//"U7J1")){//"1e99eaa2")) { //1dd99b69
            if (!found) {
                found = true;
                console.log("> Connecting to " + peripheral.advertisement.localName + "... ");
                peripheral.once('connect', function() { onConnect(peripheral); });
                peripheral.once('disconnect', function() { console.log("> peripheralDisconnecting..."); });

                peripheral.connect();
                if (peripheral.advertisement.manufacturerData[3] != 0) {
                    double_clicked = (peripheral.advertisement.manufacturerData[3] != 0);
                }
            } else {
                double_clicked = (peripheral.advertisement.manufacturerData[3] != 0);
            }

            if (double_clicked) {
                noble.stopScanning();
            }
        } else if (!found) {
            console.log("> [" + peripheral.advertisement.localName + "]");
        }
    });

    noble.on('stateChange', function(state) {
        if(state == "poweredOn") {
            noble.startScanning([ pairingService ], true);
        }
    });
}

function handleReceivePlainText(data) {
    data = Buffer.from(data);
    
    result = Buffer.concat([data, plain_end_token]);
    main_socket.write(result);
}

function handleReceiveEncrypted(ctext) {
    ctext = Buffer.from(ctext);
    // TODO: send to python
    result = Buffer.concat([data, encrypt_end_token]);
    main_socket.write(result);
}

function protocolReceive(data, isEncrypted) {
    // if(isEncrypted) {
    //     bleMsgProtocolSecure.receiveRawBuffer(Array.from(data));
    // } else {
    bleMsgProtocol.receiveRawBuffer(Array.from(data));
    // }
}

function protocolSend(msg, isEncrypted) {
    // if(isEncrypted) {
        // bleMsgProtocolSecure.sendMessage(Array.from(msg));
    // } else {
    bleMsgProtocol.sendMessage(Array.from(msg));
    // }
}

function initBleProtocol() {
    bleMsgProtocol = new ankible.BleMessageProtocol(maxPacketSize);
    // bleMsgProtocolSecure = new ankible.BleMessageProtocol(maxPacketSize);

    bleMsgProtocol.onReceiveMsg(handleReceivePlainText);
    // bleMsgProtocolSecure.onReceiveMsg(handleReceiveEncrypted);

    bleMsgProtocol.onSendRaw(function(buffer) {
        sendMessage(Buffer.from(buffer), false);
    });

    // bleMsgProtocolSecure.onSendRaw(function(buffer) {
    //     sendMessage(Buffer.from(buffer), true);
    // });
}

function onConnect(peripheral) {
    initBleProtocol();

    discoverServices(peripheral).then(function(characteristics) {

        writeChar.on('data', function(data, isNotification) {
            protocolReceive(data, false);
        });
        // secureWriteChar.on('data', function(data, isNotification) {
        //     protocolReceive(data, true);
        // });
    });
}

function discoverServices(peripheral) {

    return new Promise(function(resolve, reject) {
        peripheral.discoverServices([pairingService], function(error, services) {

            let streamPromise = new Promise(function(resolve, reject) {
                services[0].discoverCharacteristics([writeCharService, readCharService/*, secureReadCharService, secureWriteCharService*/], function(error, characteristics) {
                    if(error != undefined) {
                        reject();
                    } else {
                        writeChar = characteristics[0];
                        readChar = characteristics[1];
                        // secureWriteChar = characteristics[2];
                        // secureReadChar = characteristics[3];

                        readChar.subscribe();
                        writeChar.subscribe();
                        // secureReadChar.subscribe();
                        // secureWriteChar.subscribe();
                        
                        resolve([writeChar, readChar/*, secureWriteChar, secureReadChar*/]);
                    }
                });
            });
            
            Promise.all([streamPromise]).then(function(data) {
                resolve([data[0], data[1]/*, data[2], data[3]*/]);
            });
        });
    });
}

let keys = null;
let cryptoKeys = {};
let verHash = [];
let enteredPin = false;
let receivedNonce = false;

let maxPacketSize = 20;
let bleMsgProtocol = new ankible.BleMessageProtocol(maxPacketSize);
// let bleMsgProtocolSecure = new ankible.BleMessageProtocol(maxPacketSize);

function sendMessage(msg, encrypt) {
    let buffer = msg;

    // if(encrypt) {
    //     secureReadChar.write(buffer, true);
    // } else {
        readChar.write(buffer, true);
    // }
}
