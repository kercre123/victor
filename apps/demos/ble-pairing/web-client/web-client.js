//const aes = require("./aes.js");
//const bluetooth	= require('webbluetooth').bluetooth;

function connect() {
    navigator.bluetooth.requestDevice({
        filters:[{ services:[ "00112233-4455-6677-8899-887766554433" ] }],
        optionalServices: [0x1234, 0xABCD]
    })
    .then(device => {
        return device.gatt.connect();
    })
    .then(server => {
        let pingPromise = server.getPrimaryService(0xABCD);
        let interfacePromise = server.getPrimaryService(0x1234);

        return Promise.all([pingPromise, interfacePromise]);
    }).then(services => {
        let pingChar = services[0].getCharacteristic(0x5555);
        let writeChar = services[1].getCharacteristic(0x3333);
        let readChar = services[1].getCharacteristic(0x4444);

        return Promise.all([pingChar, readChar, writeChar]);
    }).then(c => {
        c[0].startNotifications().then(ch => 
            { ch.addEventListener('characteristicvaluechanged', onPing); }
        );

        c[1].startNotifications().then(ch => 
            { ch.addEventListener('characteristicvaluechanged', onRead); }
        );

        c[2].startNotifications().then(ch => 
            { ch.addEventListener('characteristicvaluechanged', onWrite); }
        );

        console.log("eff me");
    });
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
