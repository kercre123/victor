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
const device_1 = require("./device");
const service_1 = require("./service");
const helpers_1 = require("./helpers");
const adapter_1 = require("./adapter");
/**
 * Bluetooth Remote GATT Server class
 */
class BluetoothRemoteGATTServer {
    /**
     * Server constructor
     * @param device Device the gatt server relates to
     */
    constructor(device) {
        /**
         * The device the gatt server is related to
         */
        this.device = null;
        this._connected = false;
        this.handle = null;
        this.services = null;
        this.device = device;
        this.handle = this.device.id;
    }
    /**
     * Whether the gatt server is connected
     */
    get connected() {
        return this._connected;
    }
    /**
     * Connect the gatt server
     * @returns Promise containing the gatt server
     */
    connect() {
        return new Promise((resolve, reject) => {
            if (this.connected)
                return reject("connect error: device already connected");
            adapter_1.adapter.connect(this.handle, () => {
                this._connected = true;
                resolve(this);
            }, () => {
                this.services = null;
                this._connected = false;
                this.device.dispatchEvent(device_1.BluetoothDevice.EVENT_DISCONNECTED);
                this.device._bluetooth.dispatchEvent(device_1.BluetoothDevice.EVENT_DISCONNECTED);
            }, error => {
                reject(`connect Error: ${error}`);
            });
        });
    }
    /**
     * Disconnect the gatt server
     */
    disconnect() {
        adapter_1.adapter.disconnect(this.handle);
        this._connected = false;
    }
    /**
     * Gets a single primary service contained in the gatt server
     * @param service service UUID
     * @returns Promise containing the service
     */
    getPrimaryService(service) {
        return new Promise((resolve, reject) => {
            if (!this.connected)
                return reject("getPrimaryService error: device not connected");
            if (!service)
                return reject("getPrimaryService error: no service specified");
            this.getPrimaryServices(service)
                .then(services => {
                if (services.length !== 1)
                    return reject("getPrimaryService error: service not found");
                resolve(services[0]);
            })
                .catch(error => {
                reject(`getPrimaryService error: ${error}`);
            });
        });
    }
    /**
     * Gets a list of primary services contained in the gatt server
     * @param service service UUID
     * @returns Promise containing an array of services
     */
    getPrimaryServices(service) {
        return new Promise((resolve, reject) => {
            if (!this.connected)
                return reject("getPrimaryServices error: device not connected");
            function complete() {
                if (!service)
                    return resolve(this.services);
                const filtered = this.services.filter(serviceObject => {
                    return (serviceObject.uuid === helpers_1.getServiceUUID(service));
                });
                if (filtered.length !== 1)
                    return reject("getPrimaryServices error: service not found");
                resolve(filtered);
            }
            if (this.services)
                return complete.call(this);
            adapter_1.adapter.discoverServices(this.handle, this.device._allowedServices, services => {
                this.services = services.map(serviceInfo => {
                    Object.assign(serviceInfo, {
                        device: this.device
                    });
                    return new service_1.BluetoothRemoteGATTService(serviceInfo);
                });
                complete.call(this);
            }, error => {
                reject(`getPrimaryServices error: ${error}`);
            });
        });
    }
}
exports.BluetoothRemoteGATTServer = BluetoothRemoteGATTServer;

//# sourceMappingURL=server.js.map
