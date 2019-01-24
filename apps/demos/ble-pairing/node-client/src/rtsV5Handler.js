var readline = require('readline');
var program = require('commander');
var stringArgv = require('string-argv');
var fs = require('fs');

const { RtsCliUtil } = require('./rtsCliUtil.js');

const { Anki
} = require('./messageExternalComms.js');

const Rts = Anki.Vector.ExternalComms;

class RtsV5Handler {
  constructor(vectorBle, sodium) {
    this.vectorBle = vectorBle;
    this.vectorBle.onReceive(this);
    this.sodium = sodium;
    this.encrypted = false;
    this.keysAuthorized = false;
    this.waitForResponse = '';
    this.promiseKeys = {};

    // remembered state
    this.wifiScanResults = {};
    this.otaProgress = {};
    this.logId = 0;
    this.logFile = [];
    this.isReading = false;

    this.rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
      completer:this.completer
    });

    this.rl.on("SIGINT", function () {
      process.emit("SIGINT");
    });

    let self = this;

    process.on("SIGINT", function () {
      //graceful shutdown
      if(self.waitForResponse != '') {
        switch(self.waitForResponse) {
          case 'ota-progress':
            clearInterval(self.otaProgress.interval);
            break;
        } 

        self.waitForResponse = '';
        self.startPrompt();
      } else {
        process.exit();
      }
    });

    this.setHelp();
  }

  setHelp() {
    let helpArgs = {
      'wifi-connect':{  args:2, 
                        des:'Connect Vector to a WiFi network.',
                        help:'wifi-connect {ssid} {password}' },
      'wifi-scan':{     args:0, 
                        des:'Get WiFi networks that Vector can scan.',
                        help:'wifi-scan' },
      'wifi-ip':{       args:0, 
                        des:'Get Vector\'s WiFi IPv4/IPv6 addresses.',
                        help:'wifi-ip' },
      'wifi-ap':{       args:1, 
                        des:'Enable/Disable Vector as a WiFi access point.',
                        help:'wifi-ap {true|false}' },
      'wifi-forget':{   args:1, 
                        des:'Forget a WiFi network, or optionally all of them.',
                        help:'wifi-forget {ssid|!all}' },
      'ota-start':{     args:1, 
                        des:'Tell Vector to start an OTA update with the given URL.',
                        help:'ota-start {url}' },
      'ota-progress':{  args:0, 
                        des:'Get the current OTA progress.',
                        help:'ota-progress' },
      'ota-cancel':{    args:0, 
                        des:'Cancel an OTA in progress.',
                        help:'ota-cancel' },
      'logs':{          args:0, 
                        des:'Download logs over BLE from Vector.',
                        help:'logs' },
      'status':{        args:0, 
                        des:'Get status information from Vector.',
                        help:'status' },
      'anki-auth':{     args:1, 
                        des:'Provision Vector with Anki account.',
                        help:'anki-auth {session_token}' },
      'connection-id':{ args:1, 
                        des:'Give Vector a DAS/analytics id for this BLE session.',
                        help:'connection-id {id}' },
      'sdk':{           args:3, 
                        des:'Send an SDK request over BLE.',
                        help:'sdk {path} {json} {client_app_guid}' }
    };

    this.helpArgs = helpArgs;

    return helpArgs;
  }

  completer(line) {
    const completions = 'wifi-connect wifi-scan wifi-ip wifi-ap wifi-forget ota-start ota-cancel ota-progress logs status connection-id anki-auth'.split(' ');
    const hits = completions.filter((c) => c.startsWith(line));
    return [hits.length? hits : [], line];
  }

  cleanup() {
    this.vectorBle.onReceiveUnsubscribe(this);
  }

  send(rtsConn5) {
    let rtsConn = Rts.RtsConnection.NewRtsConnectionWithRtsConnection_5(rtsConn5);
    let extResponse = Rts.ExternalComms.NewExternalCommsWithRtsConnection(rtsConn);

    let data = extResponse.pack();

    if(this.encrypted) {
      data = this.encrypt(data);
    }

    let packet = Array.from(Buffer.from(data));

    this.vectorBle.send(packet);
  }

  receive(data) {
    if(this.encrypted) {
      data = this.decrypt(data);
    }

    let comms = new Rts.ExternalComms();
    comms.unpack(data);

    if(comms.tag == Rts.ExternalCommsTag.RtsConnection) {
      switch(comms.value.tag) {
        case Rts.RtsConnectionTag.RtsConnection_5: {
          let rtsMsg = comms.value.value;

          switch(rtsMsg.tag) {
            case Rts.RtsConnection_5Tag.RtsConnRequest:
              this.onRtsConnRequest(rtsMsg.value);
              break;
            case Rts.RtsConnection_5Tag.RtsNonceMessage:
              this.onRtsNonceMessage(rtsMsg.value);
              break;
            case Rts.RtsConnection_5Tag.RtsChallengeMessage:
              this.onRtsChallengeMessage(rtsMsg.value);
              break;
            case Rts.RtsConnection_5Tag.RtsChallengeSuccessMessage:
              this.onRtsChallengeSuccessMessage(rtsMsg.value);
              break;

            // Post-connection messages
            case Rts.RtsConnection_5Tag.RtsWifiScanResponse_3:
              this.resolvePromise('wifi-scan', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsWifiConnectResponse_3:
              this.resolvePromise('wifi-connect', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsStatusResponse_5:
              this.resolvePromise('status', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsWifiForgetResponse:
              this.resolvePromise('wifi-forget', rtsMsg);
              break;
              case Rts.RtsConnection_5Tag.RtsWifiAccessPointResponse:
              this.resolvePromise('wifi-ap', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsWifiIpResponse:
              this.resolvePromise('wifi-ip', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsCloudSessionResponse:
              this.resolvePromise('anki-auth', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsOtaUpdateResponse:
              this.otaProgress['value'] = rtsMsg.value;
              if(this.waitForResponse == 'ota-start') {
                this.resolvePromise(this.waitForResponse, rtsMsg);
              } else if(this.waitForResponse == 'ota-cancel') {
                if(rtsMsg.status != 2) {
                  this.resolvePromise(this.waitForResponse, rtsMsg);
                }
              }
              break;
            case Rts.RtsConnection_5Tag.RtsResponse:
              this.rejectPromise(this.waitForResponse, rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsSdkProxyResponse:
              this.resolvePromise('sdk', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsAppConnectionIdResponse:
              this.resolvePromise('connection-id', rtsMsg);
              break;
            case Rts.RtsConnection_5Tag.RtsLogResponse:
              if(rtsMsg.value.exitCode == 0) {
                this.logId = rtsMsg.value.fileId;
              } else {
                // todo: error case
              }
              break;
            case Rts.RtsConnection_5Tag.RtsFileDownload:
              process.stdout.write(" Downloaded " + rtsMsg.value.packetNumber + "/" + rtsMsg.value.packetTotal + " bytes\r");

              if(this.logId != rtsMsg.value.fileId) {
                break;
              }

              this.logFile = this.logFile.concat(Array.from(rtsMsg.value.fileChunk));

              if(rtsMsg.value.packetTotal > 0 && rtsMsg.value.packetNumber == rtsMsg.value.packetTotal) {
                console.log('');

                // write logs folder/file
                if (!fs.existsSync('./tmp-logs')){
                  fs.mkdirSync('./tmp-logs');
                }

                let dateNow = new Date();
                let dateStr = dateNow.getFullYear() + "-" +
                              (dateNow.getMonth() + 1) + "-" +
                              dateNow.getDate() + "-" +
                              dateNow.getHours() + "-" + 
                              dateNow.getMinutes() + "-" +
                              dateNow.getSeconds();

                fs.writeFileSync('./tmp-logs/vec-logs-' + dateStr + '.tar.bz2', Buffer.from(this.logFile));

                this.resolvePromise('logs', rtsMsg);
              }

              break;
            default:
              break;
          } 
          break;
        }
        default:
          break;
      }
    }
  }

  encrypt(data) {
    let txt = new Uint8Array(Buffer.from(data));
    let nonce = new Uint8Array(this.nonces.encrypt);

    let cipher = this.sodium.crypto_aead_xchacha20poly1305_ietf_encrypt(
      txt, null, null, nonce, this.cryptoKeys.encrypt
    );

    this.sodium.increment(this.nonces.encrypt);
    return Buffer.from(cipher);
  }

  decrypt(cipher) {
    let c = new Uint8Array(cipher);
    let nonce = new Uint8Array(this.nonces.decrypt);

    let data = null;

    try {
      data = this.sodium.crypto_aead_xchacha20poly1305_ietf_decrypt(
        null, c, null, nonce, this.cryptoKeys.decrypt
      );

      this.sodium.increment(this.nonces.decrypt);
    } catch(e) {
      console.log('error decrypting');
    }

    return data;
  }

  onRtsConnRequest(msg) {
    this.remoteKeys = {}
    this.remoteKeys.publicKey = msg.publicKey;

    // generate keys
    this.keys = this.sodium.crypto_kx_keypair();

    this.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsConnResponse(
      new Rts.RtsConnResponse(Rts.RtsConnType.FirstTimePair, this.keys.publicKey)
    ));
  }

  onRtsNonceMessage(msg) {
    let self = this;
    this.cryptoKeys = {};
    this.nonces = {};
    this.rl.close();
    this.rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
      completer:this.completer
    });

    this.isReading = true;
    this.rl.question("pin> ", function(pin) {
      self.isReading = false;
      let clientKeys = self.sodium.crypto_kx_client_session_keys(self.keys.publicKey, self.keys.privateKey, self.remoteKeys.publicKey);
      let sharedRx = self.sodium.crypto_generichash(32, clientKeys.sharedRx, pin);
      let sharedTx = self.sodium.crypto_generichash(32, clientKeys.sharedTx, pin);

      self.cryptoKeys.decrypt = sharedRx;
      self.cryptoKeys.encrypt = sharedTx;

      self.nonces.decrypt = msg.toDeviceNonce;
      self.nonces.encrypt = msg.toRobotNonce;

      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsAck(
        new Rts.RtsAck(Rts.RtsConnection_5Tag.RtsNonceMessage)
      ));
      
      self.encrypted = true;
    });
  }

  onRtsChallengeMessage(msg) {
    this.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsChallengeMessage(
      new Rts.RtsChallengeMessage(msg.number + 1)
    ));
  }

  onRtsChallengeSuccessMessage(msg) {
    this.keysAuthorized = true;

    // time to start prompt or check for ownership/auth
    this.startPrompt();
  }

  storePromiseMethods(str, resolve, reject) {
    this.promiseKeys[str] = {};
    this.promiseKeys[str].resolve = resolve;
    this.promiseKeys[str].reject = reject;
  }

  resolvePromise(str, msg) {
    if(this.promiseKeys[str] != null) {
      this.promiseKeys[str].resolve(msg);
      this.promiseKeys[str] = null;
    }
  }

  rejectPromise(str, msg) {
    if(this.promiseKeys[str] != null) {
      this.promiseKeys[str].reject(msg);
      this.promiseKeys[str] = null;
    }
  }

  cliResolve(msg) {
    if(msg == null) {
      console.log('Request timed out.');
    } else {
      console.log(RtsCliUtil.msgToStr(msg.value));
    }
    this.waitForResponse = '';
    this.startPrompt();
  }

  //
  // <!-- API Promises
  //

  doWifiScan() {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('wifi-scan', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsWifiScanRequest(
        new Rts.RtsWifiScanRequest()
      ));
    });

    return p;
  }

  doWifiConnect(ssid, password, auth, timeout) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('wifi-connect', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsWifiConnectRequest(
        new Rts.RtsWifiConnectRequest(RtsCliUtil.convertStrToHex(ssid), password, timeout, auth, false)
      ));
    });

    return p;
  }
  
  doWifiForget(ssid) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('wifi-forget', resolve, reject);
      let deleteAll = ssid == '!all';
      let hexSsid = deleteAll? '' : RtsCliUtil.convertStrToHex(ssid);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsWifiForgetRequest(
        new Rts.RtsWifiForgetRequest(deleteAll, hexSsid)
      ));
    });

    return p;
  }

  doWifiAp(enable) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('wifi-ap', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsWifiAccessPointRequest(
        new Rts.RtsWifiAccessPointRequest(enable.toLowerCase() == 'true')
      ));
    });

    return p;
  }

  doWifiIp() {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('wifi-ip', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsWifiIpRequest(
        new Rts.RtsWifiIpRequest()
      ));
    });

    return p;
  }

  doStatus() {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('status', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsStatusRequest(
        new Rts.RtsStatusRequest()
      ));
    });

    return RtsCliUtil.addTimeout(p);
  }

  doAnkiAuth(sessionToken) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('anki-auth', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsCloudSessionRequest_5(
        new Rts.RtsCloudSessionRequest_5(sessionToken, '', '')
      ));
    });

    return p;
  }

  doOtaStart(url) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('ota-start', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsOtaUpdateRequest(
        new Rts.RtsOtaUpdateRequest(url)
      ));
    });

    return p;
  }

  doOtaCancel(url) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('ota-cancel', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsOtaCancelRequest(
        new Rts.RtsOtaCancelRequest(url)
      ));
    });

    return p;
  }

  doConnectionId() {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('connection-id', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsAppConnectionIdRequest(
        new Rts.RtsAppConnectionIdRequest(url)
      ));
    });

    return p;
  }

  doLog() {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('logs', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsLogRequest(
        new Rts.RtsLogRequest(0, [])
      ));
    });

    return p;
  }

  doSdk(clientGuid, id, path, json) {
    let self = this;
    let p = new Promise(function(resolve, reject) {
      self.storePromiseMethods('sdk', resolve, reject);
      self.send(Rts.RtsConnection_5.NewRtsConnection_5WithRtsSdkProxyRequest(
        new Rts.RtsSdkProxyRequest(clientGuid, id, path, json)
      ));
    });

    return p;
  }

  requireArgs(args, num) {
    if(args.length < num) {
      console.log('"' + args[0] + '" command requires ' + (num-1) + ' arguments');
      return false;
    }

    return true;
  }

  //
  // API Promises -->
  //

  startPrompt() {
    let self = this;
    let promptCode = this.vectorBle.name.split(' ')[1];

    this.isReading = true;
    this.rl.question(`[\x1b[34mBLE\x1b[0m] vector${promptCode}$ `, function(line) {
      self.isReading = false;
      line = RtsCliUtil.removeBackspace(line);

      let cmd = line.split(' ')[0];
      let args = stringArgv(line);

      let r = function(msg) { self.cliResolve(msg); };

      switch(cmd) {
        case "quit":
        case "exit":
          self.rl.close();
          process.exit();
          break;
        case "wifi-scan":
          self.waitForResponse = 'wifi-scan';
          self.doWifiScan().then(function(msg) {
            self.wifiScanResults = msg.value.scanResult;
            self.cliResolve(msg);
          }, r);
          break;
        case "wifi-connect":
          if(!self.requireArgs(args, 3)) break;

          self.waitForResponse = 'wifi-connect';

          let ssid = args[1];
          let hasScanned = false;
          let result = null;

          for(let i = 0; i < self.wifiScanResults.length; i++) {
            let r = self.wifiScanResults[i];

            if(ssid == RtsCliUtil.convertHexToStr(r.wifiSsidHex)) {
              result = r;
              hasScanned = true;
              break;
            }
          }

          self.doWifiConnect(
            ssid, 
            args[2], 
            (hasScanned? result.authType : 6), 
            15).then(function(msg) { self.cliResolve(msg); }, r);

          break;
        case "status":
          self.waitForResponse = 'status';
          self.doStatus().then(function(msg) { self.cliResolve(msg); }, r);
          break;
        case "wifi-ip":
          self.waitForResponse = 'wifi-ip';
          self.doWifiIp().then(function(msg) { self.cliResolve(msg); }, r);
          break;
        case "wifi-forget":
          if(!self.requireArgs(args, 2)) break;

          self.waitForResponse = 'wifi-forget';
          self.doWifiForget(args[1]).then(function(msg) { 
            self.cliResolve(msg); 
          }, r);
          break;
        case "wifi-ap":
          if(!self.requireArgs(args, 2)) break;

          self.waitForResponse = 'wifi-ap';
          self.doWifiAp(args[1]).then(function(msg) { 
            self.cliResolve(msg); 
          }, r);
          break;
        case "anki-auth":
          if(!self.requireArgs(args, 2)) break;

          self.waitForResponse = 'anki-auth';
          self.doAnkiAuth(args[1]).then(function(msg) { self.cliResolve(msg); }, r);
          break;
        case "ota-start":
          if(!self.requireArgs(args, 2)) break;

          self.waitForResponse = 'ota-start';
          self.doOtaStart(args[1]).then(function(msg) { 
            self.otaProgress.value = msg.value;
            self.cliResolve(msg); 
          }, r);
          break;
        case "ota-cancel":
          self.waitForResponse = 'ota-cancel';
          self.doOtaCancel().then(function(msg) { 
            self.otaProgress.value = msg.value;
            self.cliResolve(msg); 
          }, r);
          break;
        case "ota-progress":
          if(self.otaProgress.value != null) {
            console.log(RtsCliUtil.rtsOtaUpdateResponseStr(self.otaProgress.value));
          }
          
          break;
        case "connection-id":
          if(!self.requireArgs(args, 2)) break;

          self.waitForResponse = 'connection-id';
          self.doConnectionId().then(function(msg) { 
            self.cliResolve(msg); 
          }, r);
          break;
        case "sdk":
          if(!self.requireArgs(args, 3)) break;

          self.waitForResponse = 'sdk';
          self.doSdk(args[3], RtsCliUtil.makeId(), args[1], args[2]).then(function(msg) { 
            self.cliResolve(msg); 
          }, r);
          break;
        case "logs":
          console.log('downloading logs over BLE will probably take about 30 seconds...');
          self.waitForResponse = 'logs';
          self.doLog().then(function(msg) { 
            self.cliResolve(msg); 
          }, r);
          break;
        default:
          self.waitForResponse = '';
          break;
      }
  
      if(self.waitForResponse == '') {
        self.startPrompt();
      }
    });
  }
}


module.exports = { RtsV5Handler };