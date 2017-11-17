A Node.js service to provide the current time over BLE

## Installation

```sh
bash$ ./install.sh
```

## Execution


```sh
bash$ node ./index.js
ble time server
on -> stateChange: poweredOn
on -> advertisingStart: success

```

## Notes
On macOS, the "Current Time Service" appears to be blacklisted by Apple.  I guess they don't
want us to be offering this service.  Not a problem on Ubuntu 14.04.  Regardless, the "Anki"
time service is probably easier for our purposes anyway as it just provides the time as number of
milliseconds since the epoch and the timezone as a string (like America/Los_Angeles).
