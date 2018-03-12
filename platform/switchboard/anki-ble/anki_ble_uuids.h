
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
// TODO: VIC-1639 - replace kAnkiBluetoothSIGCompanyIdentifier with final assigned number
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
const std::string kCUDDescriptorUUID("00002901-0000-1000-8000-00805F9B34FB");
const std::string kCCCDescriptorUUID("00002902-0000-1000-8000-00805F9B34FB");

} // namespace Anki