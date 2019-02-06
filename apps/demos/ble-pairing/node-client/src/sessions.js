var nconf = require('nconf');
const { RtsCliUtil } = require('./rtsCliUtil.js');

class Sessions {
  constructor() {
  }

  static KeyDictToArray(dict) {
    let dKeys = Object.keys(dict);
    let ret = new Uint8Array(dKeys.length);

    for(let j = 0; j < dKeys.length; j++) {
      ret[j] = dict[dKeys[j]];
    }

    return ret;
  }

  static GetKeys() {
    nconf.use('file', { file: './settings.json'});
    nconf.load();
    let ret = {};
    let keys = [ "publicKey", "privateKey" ];
    let d = nconf.get('me'); 

    if(d == null) {
      return null;
    }
    
    for(let i = 0; i < keys.length; i++) {
      ret[keys[i]] = Sessions.KeyDictToArray(d[keys[i]]);
    }

    return ret;
  }

  static SaveKeys(publicKey, privateKey) {
    nconf.use('file', { file: './settings.json'});
    nconf.load();
    nconf.set('me', {
      publicKey:publicKey,
      privateKey:privateKey
    });
    nconf.save();
  }

  static GetSession(remoteKey) {
    nconf.use('file', { file: './settings.json'});
    nconf.load();

    let d = nconf.get(RtsCliUtil.keyToHexStr(remoteKey));

    if(d == null) {
      return null;
    }

    d.tx = Sessions.KeyDictToArray(d.tx);
    d.rx = Sessions.KeyDictToArray(d.rx);

    return d; 
  }

  static SaveSession(remoteKey, encryptKey, decryptKey) {
    nconf.use('file', { file: './settings.json'});
    nconf.load();
    nconf.set(RtsCliUtil.keyToHexStr(remoteKey), {
      tx:encryptKey,
      rx:decryptKey
    });
    nconf.save();
  }

  static SaveClientToken(remoteKey, token) {
    nconf.use('file', { file: './settings.json'});
    nconf.load();
    let obj = nconf.get(remoteKey);
    obj.clientToken = token;
    nconf.set(remoteKey, obj);
    nconf.save(); 
  }
}

module.exports = { Sessions };