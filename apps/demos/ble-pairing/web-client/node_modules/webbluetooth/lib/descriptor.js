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
const adapter_1 = require("./adapter");
/**
 * Bluetooth Remote GATT Descriptor class
 */
class BluetoothRemoteGATTDescriptor {
    /**
     * Descriptor constructor
     * @param init A partial class to initialise values
     */
    constructor(init) {
        /**
         * The characteristic the descriptor is related to
         */
        this.characteristic = null;
        /**
         * The unique identifier of the descriptor
         */
        this.uuid = null;
        this._value = null;
        this.handle = null;
        this.characteristic = init.characteristic;
        this.uuid = init.uuid;
        this._value = init.value;
        this.handle = `${this.characteristic.uuid}-${this.uuid}`;
    }
    /**
     * The value of the descriptor
     */
    get value() {
        return this._value;
    }
    /**
     * Gets the value of the descriptor
     * @returns Promise containing the value
     */
    readValue() {
        return new Promise((resolve, reject) => {
            if (!this.characteristic.service.device.gatt.connected)
                return reject("readValue error: device not connected");
            adapter_1.adapter.readDescriptor(this.handle, dataView => {
                this._value = dataView;
                resolve(dataView);
            }, error => {
                reject(`readValue error: ${error}`);
            });
        });
    }
    /**
     * Updates the value of the descriptor
     * @param value The value to write
     */
    writeValue(value) {
        return new Promise((resolve, reject) => {
            if (!this.characteristic.service.device.gatt.connected)
                return reject("writeValue error: device not connected");
            function isView(source) {
                return source.buffer !== undefined;
            }
            const arrayBuffer = isView(value) ? value.buffer : value;
            const dataView = new DataView(arrayBuffer);
            adapter_1.adapter.writeDescriptor(this.handle, dataView, () => {
                this._value = dataView;
                resolve();
            }, error => {
                reject(`writeValue error: ${error}`);
            });
        });
    }
}
exports.BluetoothRemoteGATTDescriptor = BluetoothRemoteGATTDescriptor;

//# sourceMappingURL=descriptor.js.map
