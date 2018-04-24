/**
 * File: anki_ble_uuids.h
 *
 * Author: seichert
 * Created: 2/9/2018
 *
 * Description: Anki BLE UUIDs
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <vector>
#include <string>

// This header defines constants specific to the Anki BLE GATT Service.

namespace Anki {

// For Victor
const std::string kAnkiSingleMessageService_128_BIT_UUID("0000FEE3-0000-1000-8000-00805F9B34FB");
const std::string kAnkiSingleMessageService_16_BIT_UUID("FEE3");
const std::vector<uint8_t> kAnkiBluetoothSIGCompanyIdentifier({0xF8, 0x05});
const uint8_t kVictorProductIdentifier = (uint8_t) 'v';
const std::string kAppWriteCharacteristicUUID("7D2A4BDA-D29B-4152-B725-2491478C5CD7");
const std::string kAppReadCharacteristicUUID("30619F2D-0F54-41BD-A65A-7588D8C85B45");

// For Victor's cubes
const std::string kCubeService_128_BIT_UUID("C6F6C70F-D219-598B-FB4C-308E1F22F830");
const std::string kCubeOTATarget_128_BIT_UUID("9590BA9C-5140-92B5-1844-5F9D681557A4");
const std::string kCubeAppVersion_128_BIT_UUID("450AA175-8D85-16A6-9148-D50E2EB7B79E");
const std::string kCubeAppWrite_128_BIT_UUID("0EA75290-6759-A58D-7948-598C4E02D94A");
const std::string kCubeAppRead_128_BIT_UUID("43EF14AF-5FB1-7B81-3647-2A9477824CAB");

// Generic
const std::string kBluetoothBase_128_BIT_UUID("00000000-0000-1000-8000-00805F9B34FB");
const std::string kDeviceInfoService_128_BIT_UUID("0000180A-0000-1000-8000-00805F9B34FB");
const std::string kDeviceInformationService_16_BIT_UUID("180A");
const std::string kManufacturerNameString_128_BIT_UUID("00002A29-0000-1000-8000-00805F9B34FB");
const std::string kManufacturerNameString_16_BIT_UUID("2A29");
const std::string kModelNumberString_128_BIT_UUID("00002A24-0000-1000-8000-00805F9B34FB");
const std::string kModelNumberString_16_BIT_UUID("2A24");
const std::string kSerialNumberString_128_BIT_UUID("00002A25-0000-1000-8000-00805F9B34FB");
const std::string kSerialNumberString_16_BIT_UUID("2A25");
const std::string kHardwareRevisionString_128_BIT_UUID("00002A27-0000-1000-8000-00805F9B34FB");
const std::string kHardwareRevisionString_16_BIT_UUID("2A27");
const std::string kFirmwareRevisionString_128_BIT_UUID("00002A26-0000-1000-8000-00805F9B34FB");
const std::string kFirmwareRevisionString_16_BIT_UUID("2A26");
const std::string kSoftwareRevisionString_128_BIT_UUID("00002A28-0000-1000-8000-00805F9B34FB");
const std::string kSoftwareRevisionString_16_BIT_UUID("2A28");
const std::string kSystemID_128_BIT_UUID("00002A23-0000-1000-8000-00805F9B34FB");
const std::string kSystemID_16_BIT_UUID("2A23");
const std::string kIEEE1107320601Info_128_BIT_UUID("00002A2A-0000-1000-8000-00805F9B34FB");
const std::string kIEEE1107320601Info_16_BIT_UUID("2A2A");
const std::string kPnPID_128_BIT_UUID("00002A50-0000-1000-8000-00805F9B34FB");
const std::string kPnPID_16_BIT_UUID("2A50");
const std::string kCUDDescriptorUUID("00002901-0000-1000-8000-00805F9B34FB");
const std::string kCCCDescriptorUUID("00002902-0000-1000-8000-00805F9B34FB");

} // namespace Anki
