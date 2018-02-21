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
const characteristic_1 = require("./characteristic");
const helpers_1 = require("./helpers");
const adapter_1 = require("./adapter");
/**
 * Bluetooth Remote GATT Service class
 */
class BluetoothRemoteGATTService extends dispatcher_1.EventDispatcher {
    /**
     * Service constructor
     * @param init A partial class to initialise values
     */
    constructor(init) {
        super();
        /**
         * The device the service is related to
         */
        this.device = null;
        /**
         * The unique identifier of the service
         */
        this.uuid = null;
        /**
         * Whether the service is a primary one
         */
        this.isPrimary = false;
        this.handle = null;
        this.services = null;
        this.characteristics = null;
        this.device = init.device;
        this.uuid = init.uuid;
        this.isPrimary = init.isPrimary;
        this.handle = this.uuid;
        this.dispatchEvent(BluetoothRemoteGATTService.EVENT_ADDED);
        this.device.dispatchEvent(BluetoothRemoteGATTService.EVENT_ADDED);
        this.device._bluetooth.dispatchEvent(BluetoothRemoteGATTService.EVENT_ADDED);
    }
    /**
     * Gets a single characteristic contained in the service
     * @param characteristic characteristic UUID
     * @returns Promise containing the characteristic
     */
    getCharacteristic(characteristic) {
        return new Promise((resolve, reject) => {
            if (!this.device.gatt.connected)
                return reject("getCharacteristic error: device not connected");
            if (!characteristic)
                return reject("getCharacteristic error: no characteristic specified");
            this.getCharacteristics(characteristic)
                .then(characteristics => {
                if (characteristics.length !== 1)
                    return reject("getCharacteristic error: characteristic not found");
                resolve(characteristics[0]);
            })
                .catch(error => {
                reject(`getCharacteristic error: ${error}`);
            });
        });
    }
    /**
     * Gets a list of characteristics contained in the service
     * @param characteristic characteristic UUID
     * @returns Promise containing an array of characteristics
     */
    getCharacteristics(characteristic) {
        return new Promise((resolve, reject) => {
            if (!this.device.gatt.connected)
                return reject("getCharacteristics error: device not connected");
            function complete() {
                if (!characteristic)
                    return resolve(this.characteristics);
                const filtered = this.characteristics.filter(characteristicObject => {
                    return (characteristicObject.uuid === helpers_1.getCharacteristicUUID(characteristic));
                });
                if (filtered.length !== 1)
                    return reject("getCharacteristics error: characteristic not found");
                resolve(filtered);
            }
            if (this.characteristics)
                return complete.call(this);
            adapter_1.adapter.discoverCharacteristics(this.handle, [], characteristics => {
                this.characteristics = characteristics.map(characteristicInfo => {
                    Object.assign(characteristicInfo, {
                        service: this
                    });
                    return new characteristic_1.BluetoothRemoteGATTCharacteristic(characteristicInfo);
                });
                complete.call(this);
            }, error => {
                reject(`getCharacteristics error: ${error}`);
            });
        });
    }
    /**
     * Gets a single service included in the service
     * @param service service UUID
     * @returns Promise containing the service
     */
    getIncludedService(service) {
        return new Promise((resolve, reject) => {
            if (!this.device.gatt.connected)
                return reject("getIncludedService error: device not connected");
            if (!service)
                return reject("getIncludedService error: no service specified");
            this.getIncludedServices(service)
                .then(services => {
                if (services.length !== 1)
                    return reject("getIncludedService error: service not found");
                resolve(services[0]);
            })
                .catch(error => {
                reject(`getIncludedService error: ${error}`);
            });
        });
    }
    /**
     * Gets a list of services included in the service
     * @param service service UUID
     * @returns Promise containing an array of services
     */
    getIncludedServices(service) {
        return new Promise((resolve, reject) => {
            if (!this.device.gatt.connected)
                return reject("getIncludedServices error: device not connected");
            function complete() {
                if (!service)
                    return resolve(this.services);
                const filtered = this.services.filter(serviceObject => {
                    return (serviceObject.uuid === helpers_1.getServiceUUID(service));
                });
                if (filtered.length !== 1)
                    return reject("getIncludedServices error: service not found");
                resolve(filtered);
            }
            if (this.services)
                return complete.call(this);
            adapter_1.adapter.discoverIncludedServices(this.handle, this.device._allowedServices, services => {
                this.services = services.map(serviceInfo => {
                    Object.assign(serviceInfo, {
                        device: this.device
                    });
                    return new BluetoothRemoteGATTService(serviceInfo);
                });
                complete.call(this);
            }, error => {
                reject(`getIncludedServices error: ${error}`);
            });
        });
    }
}
/**
 * Service Added event
 * @event
 */
BluetoothRemoteGATTService.EVENT_ADDED = "serviceadded";
/**
 * Service Changed event
 * @event
 */
BluetoothRemoteGATTService.EVENT_CHANGED = "servicechanged";
/**
 * Service Removed event
 * @event
 */
BluetoothRemoteGATTService.EVENT_REMOVED = "serviceremoved";
exports.BluetoothRemoteGATTService = BluetoothRemoteGATTService;

//# sourceMappingURL=service.js.map
