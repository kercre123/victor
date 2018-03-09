import { EventDispatcher } from "./dispatcher";
import { BluetoothDevice } from "./device";
import { BluetoothRemoteGATTCharacteristic } from "./characteristic";
/**
 * Bluetooth Remote GATT Service class
 */
export declare class BluetoothRemoteGATTService extends EventDispatcher {
    /**
     * Service Added event
     * @event
     */
    static EVENT_ADDED: string;
    /**
     * Service Changed event
     * @event
     */
    static EVENT_CHANGED: string;
    /**
     * Service Removed event
     * @event
     */
    static EVENT_REMOVED: string;
    /**
     * The device the service is related to
     */
    readonly device: BluetoothDevice;
    /**
     * The unique identifier of the service
     */
    readonly uuid: string;
    /**
     * Whether the service is a primary one
     */
    readonly isPrimary: boolean;
    private handle;
    private services;
    private characteristics;
    /**
     * Service constructor
     * @param init A partial class to initialise values
     */
    constructor(init: Partial<BluetoothRemoteGATTService>);
    /**
     * Gets a single characteristic contained in the service
     * @param characteristic characteristic UUID
     * @returns Promise containing the characteristic
     */
    getCharacteristic(characteristic: string | number): Promise<BluetoothRemoteGATTCharacteristic>;
    /**
     * Gets a list of characteristics contained in the service
     * @param characteristic characteristic UUID
     * @returns Promise containing an array of characteristics
     */
    getCharacteristics(characteristic?: string | number): Promise<Array<BluetoothRemoteGATTCharacteristic>>;
    /**
     * Gets a single service included in the service
     * @param service service UUID
     * @returns Promise containing the service
     */
    getIncludedService(service: string | number): Promise<BluetoothRemoteGATTService>;
    /**
     * Gets a list of services included in the service
     * @param service service UUID
     * @returns Promise containing an array of services
     */
    getIncludedServices(service?: string | number): Promise<Array<BluetoothRemoteGATTService>>;
}
