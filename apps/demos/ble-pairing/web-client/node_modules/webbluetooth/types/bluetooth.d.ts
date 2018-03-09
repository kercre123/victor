import { EventDispatcher } from "./dispatcher";
import { BluetoothDevice } from "./device";
/**
 * Bluetooth Options interface
 */
export interface BluetoothOptions {
    /**
     * A `device found` callback function to allow the user to select a device
     */
    deviceFound?: (device: BluetoothDevice, selectFn: () => void) => boolean;
    /**
     * The amount of seconds to scan for the device (default is 10)
     */
    scanTime?: number;
    /**
     * An optional referring device
     */
    referringDevice?: BluetoothDevice;
}
/**
 * BluetoothLE Scan Filter Init interface
 */
export interface BluetoothLEScanFilterInit {
    /**
     * An array of service UUIDs to filter on
     */
    services?: Array<string | number>;
    /**
     * The device name to filter on
     */
    name?: string;
    /**
     * The device name prefix to filter on
     */
    namePrefix?: string;
}
/**
 * Request Device Options interface
 */
export interface RequestDeviceOptions {
    /**
     * An array of device filters to match
     */
    filters?: Array<BluetoothLEScanFilterInit>;
    /**
     * An array of optional services to have access to
     */
    optionalServices?: Array<string | number>;
    /**
     * Whether to accept all devices
     */
    acceptAllDevices?: boolean;
}
/**
 * Bluetooth class
 */
export declare class Bluetooth extends EventDispatcher {
    /**
     * Bluetooth Availability Changed event
     * @event
     */
    static EVENT_AVAILABILITY: string;
    /**
     * Referring device for the bluetooth instance
     */
    readonly referringDevice?: BluetoothDevice;
    private deviceFound;
    private scanTime;
    private scanner;
    /**
     * Bluetooth constructor
     * @param options Bluetooth initialisation options
     */
    constructor(options?: BluetoothOptions);
    private filterDevice(options, deviceInfo, validServices);
    /**
     * Gets the availability of a bluetooth adapter
     * @returns Promise containing a flag indicating bluetooth availability
     */
    getAvailability(): Promise<boolean>;
    /**
     * Scans for a device matching optional filters
     * @param options The options to use when scanning
     * @returns Promise containing a device which matches the options
     */
    requestDevice(options?: RequestDeviceOptions): Promise<BluetoothDevice>;
    /**
     * Cancels the scan for devices
     */
    cancelRequest(): Promise<void>;
}
