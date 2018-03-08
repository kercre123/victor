const noble = require("noble");
const readline = require("readline");
const Victor = require("./victor.js");
const fs = require('fs');
const sleep = require('system-sleep');

var quitting = false;
var victorAds = {};

var connectedVictor = undefined;
var connectingPeripheral = undefined;

function completer(line) {
    var args = line.split(/(\s+)/);
    args = args.filter(function(entry) {return /\S/.test(entry); });
    const completions = 'connect dhcptool disconnect help ifconfig ping print-heartbeats quit reboot restart-adb scan ssh-set-authorized-keys stop-scan sync-time wifi-scan wifi-set-config wifi-start wifi-stop wpa_cli'.split(' ');
    const hits = completions.filter((c) => c.startsWith(args[0]));
    if (hits.length == 0) {
        return [completions, line];
    } else if (hits.length == 1) {
        var hits_with_space = [hits[0] + " "];
        return [hits_with_space, line];
    }
    return [completions, line];
}

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    completer: completer
});

rl.setPrompt("victor-ble-cli$ ");

function outputResponse(line) {
    if (!line || quitting) {
        return;
    }
    console.log("\n" + line);
    rl.prompt(true);
};

function printHelp() {
    var help = `Commands
    help                                  -  This message
    quit                                  -  Exit this app
    scan                                  -  Search for Victors advertising via BLE.  Could take a minute.
    stop-scan                             -  Stop searching for Victors
    connect [name]                        -  Connect to a Victor by name. Defaults to first found
    disconnect                            -  Disconnect from Victor
    ping                                  -  Ping Victor
    print-heartbeats                      -  Toggle heartbeat printing for connected Victor (off by default)
    reboot [boot arg]                     -  Reboot Victor
    restart-adb                           -  Restart adb on Victor
    ssh-set-authorized-keys file          -  Use file as the ssh authorized_keys file on Victor
    sync-time                             -  Set the clock on Victor to match the host clock
    wifi-scan                             -  Ask Victor to scan for WiFi access points
    wifi-set-config ssid psk [ssid2 psk2] -  Overwrite and set wifi config on victor
    wifi-start                            -  Bring WiFi interface up
    wifi-stop                             -  Bring WiFi interface down
    wpa_cli args                          -  Execute wpa_cli with arguments on Victor
    ifconfig [args]                       -  Execute ifconfig on Victor
    dhcptool [args]                       -  Execute dhcptool on Victor`;
    outputResponse(help);
}

var onBLEServiceDiscovery = function (peripheral, error, services, characteristics) {
    if (error) {
        outputResponse("Error discovering services for "
                       + peripheral.advertisement.localName + " , error = " + error);
        peripheral.disconnect();
        return;
    }
    var victorService = undefined;
    outputResponse("Services discovered for " + peripheral.advertisement.localName);
    services.forEach(function (service) {
        // Locate Victor service ID
        if (service.uuid === Victor.SERVICE_UUID) {
            victorService = service;
        } else if (service.uuid === Victor.OLD_SERVICE_UUID) {
            victorService = service;
        }
    });
    if (!victorService) {
        outputResponse("Did not discover required Victor service ");
        peripheral.disconnect();
        return;
    }
    var receive, send;
    characteristics.forEach(function (char) {
        switch (char.uuid) {
        case Victor.RECV_CHAR_UUID:
            receive = char;
            break;
        case Victor.SEND_CHAR_UUID:
            send = char;
            break;
        case Victor.RECV_ENC_CHAR_UUID:
            char.subscribe();
            break;
        }
    });
    if (!send || !receive) {
        outputResponse("Didn't find required characteristics. Disconnecting.");
        peripheral.disconnect();
        return;
    }
    connectingPeripheral = undefined;
    connectedVictor = new Victor(peripheral, victorService, send, receive, outputResponse);
    outputResponse("Fully connected to " + peripheral.advertisement.localName);
};

var onBLEConnect = function (peripheral) {
    peripheral.once('disconnect', onBLEDisconnect.bind(this, peripheral));
    outputResponse("Connected to " + peripheral.advertisement.localName);
    var serviceUUIDs = [Victor.SERVICE_UUID, Victor.OLD_SERVICE_UUID];
    var characteristicUUIDs = [Victor.RECV_CHAR_UUID,
                               Victor.SEND_CHAR_UUID,
                               Victor.RECV_ENC_CHAR_UUID];
    peripheral.discoverSomeServicesAndCharacteristics(serviceUUIDs, characteristicUUIDs,
                                                      onBLEServiceDiscovery.bind(this, peripheral));
};

var onBLEDisconnect = function (peripheral) {
    peripheral.removeAllListeners();
    outputResponse("Disconnected from " + peripheral.advertisement.localName);
    connectingPeripheral = undefined;
    connectedVictor = undefined;
};

var onBLEDiscover = function (peripheral) {
    if (!peripheral) {
        return;
    }
    if (!peripheral.advertisement) {
        return;
    }
    var localName = peripheral.advertisement.localName;
    if (!localName) {
        return;
    }
    if (!(localName.startsWith("VICTOR") || localName.startsWith("Vector"))) {
        return;
    }

    victorAds[localName] = peripheral;
    outputResponse("Found " + localName + " (RSSI = " + peripheral.rssi + ")");
};

noble.on('discover', onBLEDiscover);
noble.on('scanStart', function () {
    outputResponse("Scanning started.....");
});
noble.on('scanStop', function () {
    outputResponse("Scanning stopped.");
});

var handleInput = function (line) {
    var trimmedLine = line.trim();
    var r = /[^\s"]+|"([^"]*)"/gi;
    var args = [];
    do {
        //Each call to exec returns the next regex match as an array
        var match = r.exec(trimmedLine);
        if (match != null)
        {
            //Index 1 in the array is the captured group if it exists
            //Index 0 is the matched text, which we use if no captured group exists
            args.push(match[1] ? match[1] : match[0]);
        }
    } while (match != null);
    args = args.filter(function(entry) {return /\S/.test(entry); });
    if (args.length > 0) {
        switch(args[0]) {
        case 'help':
            printHelp();
            break;
        case 'quit':
            quitting = true;
            if (connectedVictor) {
                connectedVictor.send(Victor.MSG_B2V_BTLE_DISCONNECT);
                setTimeout(function () {
                    if (connectedVictor) {
                        connectedVictor.disconnect();
                        connectedVictor = undefined;
                    }
                    rl.close();
                    process.exit();
                }, 30);
            } else {
                rl.close();
                process.exit();
            }
            break;
        case 'print-heartbeats':
            if (connectedVictor) {
                connectedVictor._print_heartbeats = !connectedVictor._print_heartbeats;
                if (connectedVictor._print_heartbeats) {
                    outputResponse("Heartbeat printing is now on.  Current count is " + connectedVictor._heartbeat_counter);
                } else {
                    outputResponse("Heartbeat printing is now off");
                }
            } else {
                outputResponse("Not connected to a Victor");
            }
            break;
        case 'scan':
            if (connectedVictor) {
                outputResponse("Disconnect from Victor first");
            } else {
                if (noble.state !== 'poweredOn') {
                    outputResponse("Please power on your Bluetooth adapter to start scanning.");
                    noble.once('stateChange', function (state) {
                        if (state === 'poweredOn') {
                            noble.startScanning();
                        }
                    });
                } else {
                    noble.startScanning();
                }
            }
            break;
        case 'stop-scan':
            noble.removeAllListeners('stateChange');
            if (noble.start === 'poweredOn') {
                noble.stopScanning();
            }
            victorAds = {};
            break;
        case 'restart-adb':
            if (connectedVictor) {
                connectedVictor.send(Victor.MSG_B2V_DEV_RESTART_ADBD);
            } else {
                outputResponse("You must be connected to a victor to restart adb");
            }
            break;
        case 'connect':
            if (connectedVictor) {
                outputResponse("You are already connected to a victor");
            } else if (connectingPeripheral) {
                outputResponse("Connecting already in progress to "
                               + connectingPeripheral.advertisement.localName);
            } else if (Object.keys(victorAds).length == 0) {
                outputResponse("No victors found to connect to.");
            } else {
                noble.once('scanStop', function () {
                    var localName = Object.keys(victorAds)[0];
                    if (args.length > 1) {
                        localName = args[1];
                    }
                    connectingPeripheral = victorAds[localName];
                    if (!connectingPeripheral) {
                        outputResponse("Couldn't find victor named " + localName);
                    } else {
                        victorAds = {};
                        connectingPeripheral.removeAllListeners();
                        outputResponse("Trying to connect to " + localName + "....");
                        connectingPeripheral.once('connect', onBLEConnect.bind(this, connectingPeripheral));
                        connectingPeripheral.connect(function (error) {
                            if (error) {
                                outputResponse(error);
                                connectingPeripheral.removeAllListeners();
                                connectingPeripheral = undefined;
                            }
                        });
                    }
                });
                noble.stopScanning();
            }
            break;
        case 'disconnect':
            if (!connectedVictor) {
                if (connectingPeripheral) {
                    connectingPeripheral.disconnect(function (error) {
                        connectingPeripheral = undefined;
                        if (error) {
                            outputResponse("Error disconnecting. error = " + error);
                        }
                    });
                    connectingPeripheral = undefined;
                    outputResponse("Stopping connection attempt");
                } else {
                    outputResponse("Not connected to a Victor");
                }
            } else {
                connectedVictor.send(Victor.MSG_B2V_BTLE_DISCONNECT);
                setTimeout(function () {
                    if (connectedVictor) {
                        connectedVictor.disconnect();
                        connectedVictor = undefined;
                    }
                }, 30);
            }
            break;
        case 'ping':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.send(Victor.MSG_B2V_CORE_PING_REQUEST);
            }
            break;
        case 'wifi-scan':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.send(Victor.MSG_B2V_WIFI_SCAN);
            }
            break;
        case 'wifi-set-config':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                var buf = Buffer.alloc(0);
                for (var i = 1 ; i < args.length ; i++) {
                    buf = Buffer.concat([buf, Buffer.from(args[i]), Buffer.from([0])]);
                }
                connectedVictor.send(Victor.MSG_B2V_WIFI_SET_CONFIG, buf);
            }
            break;
        case 'wifi-start':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.send(Victor.MSG_B2V_WIFI_START);
            }
            break;
        case 'wifi-stop':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.send(Victor.MSG_B2V_WIFI_STOP);
            }
            break;
        case 'ssh-set-authorized-keys':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                fs.readFile(args[1], 'utf8', function (err, data) {
                    if (err) {
                        outputResponse(err);
                    } else {
                        connectedVictor.send(Victor.MSG_B2V_SSH_SET_AUTHORIZED_KEYS, Buffer.from(data));
                    }
                });
            }
            break;
        case 'sync-time':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.syncTime();
            }
            break;
        default:
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.sendCommand(args);
            }
            break;
        }
    }
    if (!quitting) {
        rl.prompt();
    }

};

rl.on('line', handleInput);
rl.prompt();
