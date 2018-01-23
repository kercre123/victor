import { Bluetooth } from "./bluetooth";
/**
 * Default bluetooth instance synonymous with `navigator.bluetooth`
 */
export declare const bluetooth: Bluetooth;
/**
 * Helper methods and enums
 */
export * from "./helpers";
/**
 * Bluetooth class for creating new instances
 */
export { Bluetooth };
/**
 * Other classes if required
 */
export { BluetoothDevice } from "./device";
export { BluetoothRemoteGATTServer } from "./server";
export { BluetoothRemoteGATTService } from "./service";
export { BluetoothRemoteGATTCharacteristic } from "./characteristic";
export { BluetoothRemoteGATTDescriptor } from "./descriptor";
