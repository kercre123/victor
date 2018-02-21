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
function __export(m) {
    for (var p in m) if (!exports.hasOwnProperty(p)) exports[p] = m[p];
}
Object.defineProperty(exports, "__esModule", { value: true });
const bluetooth_1 = require("./bluetooth");
exports.Bluetooth = bluetooth_1.Bluetooth;
/**
 * Default bluetooth instance synonymous with `navigator.bluetooth`
 */
exports.bluetooth = new bluetooth_1.Bluetooth();
/**
 * Helper methods and enums
 */
__export(require("./helpers"));
/**
 * Other classes if required
 */
var device_1 = require("./device");
exports.BluetoothDevice = device_1.BluetoothDevice;
var server_1 = require("./server");
exports.BluetoothRemoteGATTServer = server_1.BluetoothRemoteGATTServer;
var service_1 = require("./service");
exports.BluetoothRemoteGATTService = service_1.BluetoothRemoteGATTService;
var characteristic_1 = require("./characteristic");
exports.BluetoothRemoteGATTCharacteristic = characteristic_1.BluetoothRemoteGATTCharacteristic;
var descriptor_1 = require("./descriptor");
exports.BluetoothRemoteGATTDescriptor = descriptor_1.BluetoothRemoteGATTDescriptor;

//# sourceMappingURL=index.js.map
