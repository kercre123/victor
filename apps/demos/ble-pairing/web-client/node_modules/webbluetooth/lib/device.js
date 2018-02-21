"use strict";
/*
* Node Web Bluetooth
* Copyright (c) 2017 Rob Moran
*
* The MIT License (MIT)
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
Object.defineProperty(exports, "__esModule", { value: true });
const dispatcher_1 = require("./dispatcher");
const server_1 = require("./server");
/**
 * Bluetooth Device class
 */
class BluetoothDevice extends dispatcher_1.EventDispatcher {
    /**
     * Device constructor
     * @param init A partial class to initialise values
     */
    constructor(init) {
        super();
        /**
         * The unique identifier of the device
         */
        this.id = null;
        /**
         * The name of the device
         */
        this.name = null;
        /**
         * The gatt server of the device
         */
        this.gatt = null;
        /**
         * Whether adverts are being watched (not implemented)
         */
        this.watchingAdvertisements = false;
        /**
         * @hidden
         */
        this._bluetooth = null;
        /**
         * @hidden
         */
        this._allowedServices = [];
        /**
         * @hidden
         */
        this._serviceUUIDs = [];
        this.id = init.id;
        this.name = init.name;
        this.gatt = init.gatt;
        this.watchAdvertisements = init.watchAdvertisements;
        this.adData = init.adData;
        this._bluetooth = init._bluetooth;
        this._allowedServices = init._allowedServices;
        this._serviceUUIDs = init._serviceUUIDs;
        if (!this.name)
            this.name = `Unknown or Unsupported Device (${this.id})`;
        if (!this.gatt)
            this.gatt = new server_1.BluetoothRemoteGATTServer(this);
    }
    /**
     * Starts watching adverts from this device (not implemented)
     */
    watchAdvertisements() {
        return new Promise((_resolve, reject) => {
            reject("watchAdvertisements error: method not implemented");
        });
    }
    /**
     * Stops watching adverts from this device (not implemented)
     */
    unwatchAdvertisements() {
        return new Promise((_resolve, reject) => {
            reject("unwatchAdvertisements error: method not implemented");
        });
    }
}
/**
 * Server Disconnected event
 * @event
 */
BluetoothDevice.EVENT_DISCONNECTED = "gattserverdisconnected";
/**
 * Advertisement Received event
 * @event
 */
BluetoothDevice.EVENT_ADVERT = "advertisementreceived";
exports.BluetoothDevice = BluetoothDevice;

//# sourceMappingURL=device.js.map
