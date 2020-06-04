/// Connect to WiFi

class AutoScript {
  constructor(config) {
    this.config = {};
    this.stateDict = {};

    if(config) {
      this.config = config;
    }
  }

  async run(handler) {
    let error = await handler.doWifiScan().catch(err => { console.log(err); });
  
    if(error == undefined) {
      // error
      console.log("Scan failed.");
      return false;
    }

    // connect Vector to a WiFi network
    let result = await handler.doWifiConnect(this.config.ssid, this.config.pw, this.config.auth, 15).catch(err => { console.log(err); });
    if(result == undefined) {
      // error
      console.log("Connection failed.");
      return false;
    }

    let connected = result.value.wifiState == 1 || result.value.wifiState == 2;

    if(connected) {
      process.exit(0);
    } else {
      process.exit(1);
    }
  }
}

module.exports = { AutoScript };