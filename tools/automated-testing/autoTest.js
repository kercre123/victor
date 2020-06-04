/// temp test

var tap = require('tap');
var files = require("fs");
var path = require('path');

class AutoScript {
  constructor(config) {
    this.config = {};
    this.stateDict = {};
    this.dateTime = this.getDateTime();

    if(config) {
      this.config = config;
    }

    let outputPath = 'vector-tap-' + this.dateTime + '.txt';
    
    if('output' in this.config) {
      outputPath = this.config['output'];
    }
    
    var writableStream = files.createWriteStream(outputPath);
    tap.pipe(writableStream);
  }

  getDateTime() {
    let d = new Date(Date.now());
    let year = '' + d.getFullYear();
    let month = ('' + (d.getMonth() + 1)).padStart(2, '0');
    let day = ('' + d.getDate()).padStart(2, '0');
    let hours = ('' + d.getHours()).padStart(2, '0');
    let mins = ('' + d.getMinutes()).padStart(2, '0');
    let secs = ('' + d.getSeconds()).padStart(2, '0');

    return year + '-' + month + '-' + day + '-' + hours + '-' + mins + '-' + secs;
  }

  async run(handler) {
    console.log("=> Beginning tests");

    let self = this;

    await this.writeMetaData(handler); 

    await tap.test('Anki Account Authorization', function(t) {
      self.test_AnkiAuth(handler, t).then(function(r) { t.end(); });
    }); 

    await tap.test('WiFi AP Mode', function(t) {
      self.test_WifiAccessPointMode(handler, t).then(function(r) { t.end(); });
    });
    
    await tap.test('WiFi Connection', function(t) {
      self.test_WifiConnect(handler, t, self.config.ssid, self.config.pw, self.config.auth).then(function(r) { t.end(); });
    });    

    await tap.test('mDNS Discovery', function(t) {
      self.test_Mdns(handler, t).then(function(r) { t.end(); });
    });   

    await tap.test('auto-test Binaries', function(t) {
      self.test_AutoTestBinaries(handler, t).then(function(r) { t.end(); });
    });  

    tap.end();

    let result = await this.checkForUpdate(handler);
    if(this.config.update) {
      if(result.hasUpdate) {
        await this.otaUpdateToLkg(handler, result);
      }
    } else {
      console.log("=> Skipping update.");
    }

    process.exit(0);
  }

  async writeMetaData(handler) {
    let statusMsg = await handler.doStatus();
    let version = statusMsg.value.version;
    tap.write("\n");
    tap.comment("Robot: " + handler.vectorBle.name);
    tap.comment("  ESN: " + statusMsg.value.esn);
    tap.comment("Build: " + version);
    tap.comment(" Date: " + this.dateTime);
    tap.write("\n");
  }

  async getCurrentVersion(handler) {
    let statusMsg = await handler.doStatus();

    let currentString = statusMsg.value.version;
    let currentPieces = currentString.split('-')[0].split('.');
    return Number(currentPieces[currentPieces.length - 1]);
  }

  async otaUpdateToLkg(handler, versions) {
    console.log(`=> Updating from ${versions.current} to ${versions.lkg}`);
        
    handler.onOtaProgress(function(value) {
      console.log(value);
    });
    let updateStatus = await handler.doOtaStart('http://ota.global.anki-dev-services.com/vic/master-dev/lo8awreh23498sf/full/lkg.ota');

    if(updateStatus == null) {
      console.log('=> Update failed.');
    } else {
      console.log('=> Update successful, rebooting');
    }
  }

  async checkForUpdate(handler) {
    let currentVersion = await this.getCurrentVersion(handler);
    let nextVersionString = (await SpawnChild('curl', [
      '-X', 
      'GET', 
      'http://ota.global.anki-dev-services.com/vic/master-dev/lo8awreh23498sf/full/lkg.ota',
      '--range',
      '0-1000' ])).stdout;

    let nextPieces = nextVersionString.split('\n');
    let nextVersion = 0;
    for(let i = 0; i < nextPieces.length; i++) {
      if(nextPieces[i].includes('update_version')) {
        let p = nextPieces[i].split('=')[1].split('.');
        nextVersion = Number(p[p.length - 1].substring(0, p[p.length - 1].length - 1));
        break;
      }
    }

    let result = { current:currentVersion, lkg:nextVersion, hasUpdate:false };
    
    if(nextVersion > currentVersion) {
      result['hasUpdate'] = true;
    }

    return result;
  }

  async test_AutoTestBinaries(handler, t) {
    const fs = require('fs');
    
    let ret = true;

    let autoTestBin = this.config['auto_test_bin'];
    if(autoTestBin.substring(0, 1) != "/") {
      // relative path
      autoTestBin = path.join(__dirname, autoTestBin);
    }

    let binaries = fs.readdirSync(autoTestBin);
    for(let i = 0; i < binaries.length; i++) {
      let result = await SpawnChild('ssh', ['root@' + this.stateDict['ip'], '"/auto-test/bin/' + binaries[i] + '"']);
      ret = ret && (result.code == 0);
      t.ok(result.code == 0, "'" + binaries[i] + "' exited with status code {" + result.code + "}");
      
      if(result.code != 0) {
        t.comment(`stdout:\n${result.stdout}`);
        t.comment(`stderr:\n${result.stderr}`);
      }
    }

    return ret;
  }

  async test_AnkiAuth(handler, t) {
    let result = undefined;
    
    result = await AnkiLogin(this.config.anki_username, this.config.anki_password, this.config.anki_app_token);
    t.ok(result, "Login to Anki Account");

    if(result == false) {
      return false;
    }

    result = await handler.doAnkiAuth(result).catch(err => console.log(err));
    let authorized = (result != undefined) && (result.value.success == true);
    t.ok(authorized, "Authorize Anki Account on Robot");

    if(!authorized) {
      return false;
    }
    
    this.stateDict['cloudToken'] = result.value.clientTokenGuid;

    return true;
  }

  async test_Mdns(handler, t) {
    let result = undefined;

    result = await DetectMdns(handler);
    let detectedMdns = (result != false) && (result.addresses.length != 0);
    t.ok(detectedMdns, "Detect Vector over mDNS");

    if(!detectedMdns) {
      return false;
    }

    this.stateDict['ip'] = result.addresses[0];

    result = await TestPing(this.stateDict['ip']);
    t.ok(result, "Ping Vector's IP address");

    if(result == false) {
      return false;
    }

    return true;
  }

  async test_WifiConnect(handler, t, ssid, password, auth) {
    let error = undefined;

    await handler.doWifiAp('true');
    await Delay(3);
    error = await handler.doWifiForget('!all').catch(err => { console.log(err); });
    t.ok(error != undefined, "WiFi forget SSID");

    // disable access point mode
    await handler.doWifiAp('false');
    await Delay(6);

    // scan local machine's wifi
    error = await handler.doWifiScan().catch(err => { console.log(err); });
    t.ok(error != undefined, "WiFi Scan");

    if(error == undefined) {
      // error
      return false;
    }

    // connect Vector to a WiFi network
    error = await handler.doWifiConnect(ssid, password, auth, 15).catch(err => { t.fail(err); });
    if(error == undefined) {
      // error
      t.fail("WiFi connection attempt");
      return false;
    }

    let connected = error.value.wifiState == 1 || error.value.wifiState == 2;
    t.ok(connected, "WiFi connection attempt");

    if(connected) {
      return true;
    }

    return false;
  }

  async test_WifiAccessPointMode(handler, t) {
    let result = undefined;
    let wifi = require('node-wifi');

    // initialize node-wifi
    wifi.init({ iface: null });

    // cycle access point mode
    await handler.doWifiAp('false').catch(err => { console.log(err); });

    let credentials = await handler.doWifiAp('true');

    let ssid = credentials.value.ssid;
    let pw = credentials.value.password;

    // scan local machine's wifi
    result = await WifiScan().catch(err => { console.log(err); });

    t.ok(result != undefined, "WiFi Scan");

    if(result == undefined) {
      // error
      return false;
    }

    // wait enough time for Vector to setup AP
    await Delay(5);

    // connect local machine to Vector's wifi
    ssid = await PromiseWithRetry(WifiConnect, [ssid, pw], 3, 2).catch(err => { console.log(err); });

    t.ok(ssid != undefined, "Host WiFi connection to Vector");

    if(ssid == undefined) {
      // error
      return false;
    }

    // disable access point mode
    await handler.doWifiAp('false');
    await Delay(5);

    return true;
  }

  async clearUserData() {
    let self = this;
  
    let p = new Promise((resolve, reject) => {
      SpawnChild("ssh", ['root@' + self.stateDict['ip'], 'echo 1 > /run/wipe-data && reboot']).then(function() {
        resolve();
      });
    });
  
    return p;
  }
}

function WifiScan() {
  let wifi = require('node-wifi');

  let p = new Promise(function(resolve, reject) {
    wifi.scan(function(err, networks) {
      if(err) {
        reject(err);
      } else {
        resolve(networks);
      }
    });
  });

  return p;
}

function WifiConnect(ssid, password) {
  let wifi = require('node-wifi');

  let p = new Promise(function(resolve, reject) {
    wifi.connect({ ssid:ssid, password:password },
    function(err) {
      if (err) {
        reject(err);
      } else {
        resolve(ssid);
      }
    });
  });

  return p;
}

function TestPing(host) {
  let ping = require('ping');
  let p = new Promise(function(resolve, reject) {
    ping.sys.probe(host, function(isAlive){
      if(isAlive) {
        resolve(true);
      } else {
        resolve(false);
      }
    });
  });

  return p;
}

function DetectMdns(handler) {
  let mdns = require('mdns-js');
  let p = new Promise(function(resolve, reject) {
    let browser = mdns.createBrowser(mdns.tcp('ankivector'));
  
    browser.on('ready', function () {
      browser.discover(); 
    });

    let t = setTimeout(function() {
      browser.stop();
      resolve(false);
    }, 10000);
    
    browser.on('update', function (data) {
      let code = handler.vectorBle.name.split(' ')[1];
      if(data.host == 'Vector-' + code + '.local') {
        clearTimeout(t);
        browser.stop();
        resolve(data);
      }
    });
  });
  
  return p;
}

function WifiDelete(ssid) {
  let wifi = require('node-wifi');

  let p = new Promise(function(resolve, reject) {
    wifi.deleteConnection({ ssid:ssid },
    function(err) {
      if (err) {
        reject(err);
      } else {
        resolve(ssid);
      }
    });
  });

  return p;
}

function AnkiLogin(username, password, appKey) {
  let request = require('request');
  let p = new Promise(function(resolve, reject) {
    let options = {
      method: 'POST',
      url: 'https://accounts-dev2.api.anki.com/1/sessions',
      headers: {
        'Anki-App-Key': appKey
      },
      form: {
        'username': username,
        'password': password
      }
    };
    
    function callback(error, response, body) {
      if (!error && response.statusCode == 200) {
        var info = JSON.parse(body);
        resolve(info.session.session_token);
      } else {
        console.log(error);
        resolve(false);
      }
    }
    
    request(options, callback)
  });

  return PromiseWithTimeout(p, 10000);
}

function PromiseWithTimeout(promise, timeout) {
  let timeoutPromise = new Promise((resolve, reject) => {
    let t = setTimeout(() => {
      clearTimeout(t);
      resolve(false);
    }, timeout);
  });

  return Promise.race([ promise, timeoutPromise ]);
}

function PromiseWithRetry(promise, args, tryCount, wait) {
  let retryPromise = new Promise((resolve, reject) => {
    let success = function() {
      resolve.apply(null, arguments);
    };
    let failure = function() {
      if(tryCount <= 1) {
        reject.apply(null, arguments); 
      } else {
        Delay(wait).then(function() {
          PromiseWithRetry(promise, args, tryCount-1, 1).then(success).catch(() => { reject.apply(null, arguments); });
        });
      }
    };
    promise.apply(null, args).then(success).catch(failure);
  });

  return retryPromise;
}

function Delay(delaySeconds) {
  let delayPromise = new Promise((resolve, reject) => {
    let t = setTimeout(() => {
      clearTimeout(t);
      resolve();
    }, delaySeconds * 1000);
  });

  return delayPromise;
}

function SpawnChild(command, args) {
  const { spawn } = require('child_process');

  let p = new Promise((resolve, reject) => {
    const cmd = spawn(command, args); 

    ret = {};

    ret['stdout'] = "";
    ret['stderr'] = "";

    cmd.stdout.on('data', (data) => {
      ret['stdout'] += data; 
    });
  
    cmd.stderr.on('data', (data) => {
      ret['stderr'] += data;
    });
  
    cmd.on('close', (code) => {
      ret['code'] = code;
      resolve(ret);
    });
  });

  return p;
}

module.exports = { AutoScript };