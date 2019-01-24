var program = require('commander');
const _vectorBle = require('./vectorBluetooth.js');
const { IntBuffer } = require('./clad.js');
const _sodium = require('libsodium-wrappers');
const { RtsV2Handler } = require('./rtsV2Handler.js');
const { RtsV4Handler } = require('./rtsV4Handler.js');
const { RtsV5Handler } = require('./rtsV5Handler.js');

function list(val) {
  return val.split(',');
}
 
// main
function Main() {
  program
    .version('0.0.1', '-v, --verison')
    .option('-f, --filter [type]', 'Filter BLE scan for specific Vector.', list)
    //.option('-d, --debug', 'Debug logs.')
    .option('-p, --protocol', 'Force a specific RTS protocol version.')
    .parse(process.argv);
  
  StartBleComms(program.filter);
}

async function InitializeSodium() {
  await _sodium.ready;
}

function GenerateHandshakeMessage(version) {
  let buffer = IntBuffer.Int32ToLE(version);
  return [1].concat(buffer);
}

function HandleHandshake(vec, version) {
  if(handler != null) {
    console.log('');

    handler.cleanup();
    handler = null;
  }

  console.log(`Vector is requesting RTS v${version}`);

  switch(version) {
    case 5:
      // RTSv4
      handler = new RtsV5Handler(vec, _sodium);
      break;
    case 4:
      // RTSv4
      handler = new RtsV4Handler(vec, _sodium);
      break;
    case 3:
      // RTSv3 (Dev)
      break;
    case 2:
      // RTSv2 (Factory)
      handler = new RtsV2Handler(vec, _sodium);
      break;
    default:
      // Unknown
      break;
  }

  vec.send(GenerateHandshakeMessage(version));
}

function StartBleComms(filter) {
  InitializeSodium().then(function() {
    // Start cli
    let vecBle = new _vectorBle.VectorBluetooth();
    let handshakeHandler = {};

    handshakeHandler.receive = function(data) {
      if(data[0] == 1 && data.length == 5) {
        // This message is a handshake from Vector
        let v = IntBuffer.BufferToUInt32(data.slice(1));
        HandleHandshake(vecBle, v);
      } else {
        // Received message after version handler exists

      }
    };

    vecBle.onReceive(handshakeHandler);
    vecBle.tryConnect(filter);
  });
}

// start cli
let handler = null;
Main();