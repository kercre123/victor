const noble = require("noble");
const readline = require("readline");
const Victor = require("./victor.js");
const fs = require('fs');

var victorAds = {};
var connectedVictor = undefined;

function completer(line) {
    var args = line.split(/(\s+)/);
    args = args.filter(function(entry) {return /\S/.test(entry); });
    const completions = 'connect dhcptool disconnect help ifconfig ping quit reboot restart-adb scan ssh-set-authorized-keys stop-scan wifi-set-config wifi-start wifi-stop wpa_cli'.split(' ');
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

rl.setPrompt("$ ");

function outputResponse(line) {
    if (!line) {
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
    reboot [boot arg]                     -  Reboot Victor
    restart-adb                           -  Restart adb on Victor
    ssh-set-authorized-keys file          -  Use file as the ssh authorized_keys file on Victor
    wifi-set-config ssid psk [ssid2 psk2] -  Overwrite and set wifi config on victor
    wifi-start                            -  Bring WiFi interface up
    wifi-stop                             -  Bring WiFi interface down
    wpa_cli args                          -  Execute wpa_cli with arguments on Victor
    ifconfig [args]                       -  Execute ifconfig on Victor
    dhcptool [args]                       -  Execute dhcptool on Victor`;
    outputResponse(help);
}

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
    if (!localName.startsWith("VICTOR")) {
        return;
    }

    victorAds[localName] = peripheral;
    outputResponse("Found " + localName);

    peripheral.once('connect', function () {
        victorAds = {};
        outputResponse("Connected to " + localName);
        peripheral.discoverServices();
    });

    peripheral.once('servicesDiscover', function (services) {
        var haveVictorServiceId = false;
        services.forEach(function (service) {
            outputResponse("Services discovered for " + localName);
            // Locate Victor service ID
            if (service.uuid !== 'd55e356b59cc42659d5f3c61e9dfd70f') {
                return;
            }

            haveVictorServiceId = true;

            service.on('characteristicsDiscover', function(characteristics) {
                var receive, send;

                characteristics.forEach(function (char) {
                    switch (char.uuid) {
                    case '30619f2d0f5441bda65a7588d8c85b45':
                        receive = char;
                        break;
                    case '7d2a4bdad29b4152b7252491478c5cd7':
                        send = char;
                        break;
                    }
                });
                if (!send || !receive) {
                    outputResponse("Didn't find required characteristics. Disconnecting.");
                    peripheral.disconnect();
                    return;
                }
                connectedVictor = new Victor(peripheral, service, send, receive, outputResponse);
                outputResponse("Fully connected to " + localName);

            });

            service.discoverCharacteristics();
        });
        if (!haveVictorServiceId) {
            outputResponse("Didn't find required Victor service ID. Disconnecting.");
            peripheral.disconnect();
            return;
        }
    });

    peripheral.on('disconnect', function () {
        outputResponse("Disconnected");
        connectedVictor = undefined;
    });
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
    var args = trimmedLine.split(/(\s+)/);
    args = args.filter(function(entry) {return /\S/.test(entry); });
    if (args.length > 0) {
        switch(args[0]) {
        case 'help':
            printHelp();
            break;
        case 'quit':
            rl.close();
            process.exit();
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
            } else if (Object.keys(victorAds).length == 0) {
                outputResponse("No victors found to connect to.");
            } else {
                noble.stopScanning();
                var peripheral = undefined;
                var localName = Object.keys(victorAds)[0];
                if (args.length > 1) {
                    localName = args[1];
                }
                peripheral = victorAds[localName];
                if (!peripheral) {
                    outputResponse("Couldn't find victor named " + localName);
                } else {
                    outputResponse("Trying to connect to " + localName + "....");
                    peripheral.connect(function (error) {outputResponse(error);});
                }
            }
            break;
        case 'disconnect':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.disconnect();
                connectedVictor = undefined;
            }
            break;
        case 'ping':
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                connectedVictor.send(Victor.MSG_B2V_CORE_PING_REQUEST);
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
        default:
            if (!connectedVictor) {
                outputResponse("Not connected to a Victor");
            } else {
                var size = trimmedLine.length + 1;
                const buf = Buffer.alloc(size);
                var offset = 0;
                for (var i = 0 ; i < args.length ; i++) {
                    offset += buf.write(args[i], offset, args[i].length);
                    offset = buf.writeUInt8(0, offset);
                }
                connectedVictor.send(Victor.MSG_B2V_DEV_EXEC_CMD_LINE, buf);
            }
            break;
        }
    }
    rl.prompt();

};

rl.on('line', handleInput);
rl.prompt();
