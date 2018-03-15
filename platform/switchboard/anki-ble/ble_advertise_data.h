/**
 * File: ble_advertise_data.h
 *
 * Author: seichert
 * Created: 1/23/2018
 *
 * Description: Bluetooth Low Energy Advertising Settings
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <string>
#include <vector>

namespace Anki {

class BLEAdvertiseData {
 public:
  BLEAdvertiseData()
      : include_device_name_(false)
      , include_tx_power_level_(false)
      , min_interval_(0)
      , max_interval_(0)
  { }
  ~BLEAdvertiseData() = default;

  // Whether the device name should be included in the advertisement
  // The device's name is the name we have assigned to the Bluetooth adapter
  void SetIncludeDeviceName(const bool value) { include_device_name_ = value; }
  bool GetIncludeDeviceName() const { return include_device_name_; }

  // Whether or not to include the tranmission power level in the advertisement
  void SetIncludeTxPowerLevel(const bool value) { include_tx_power_level_ = value; }
  bool GetIncludeTxPowerLevel() const { return include_tx_power_level_; }

  void SetManufacturerData(const std::vector<uint8_t>& data) { manufacturer_data_ = data; }
  const std::vector<uint8_t>& GetManufacturerData() const { return manufacturer_data_; }
  std::vector<uint8_t>& GetManufacturerData() { return manufacturer_data_; }

  void SetServiceData(const std::vector<uint8_t>& data) { service_data_ = data; }
  const std::vector<uint8_t>& GetServiceData() const { return service_data_; }
  std::vector<uint8_t>& GetServiceData() { return service_data_; }

  void SetServiceUUID(const std::string uuid) {service_uuid_ = uuid;}
  const std::string& GetServiceUUID() const { return service_uuid_; }

  // The min and max interval are for the slave connection interval range
  // advertisement.  Leave set at 0 to NOT include the
  // slave connection interval range in the advertisements.
  void SetMinInterval(int min_interval) { min_interval_ = min_interval; }
  int GetMinInterval() const { return min_interval_; }

  void SetMaxInterval(int max_interval) { max_interval_ = max_interval; }
  int GetMaxInterval() const { return max_interval_; }

  bool empty() const {
    return (!include_device_name_
            && !include_tx_power_level_
            && manufacturer_data_.empty()
            && service_data_.empty()
            && service_uuid_.empty()
            && !min_interval_
            && !max_interval_);
  }

 private:
  bool include_device_name_;
  bool include_tx_power_level_;
  std::vector<uint8_t> manufacturer_data_;
  std::vector<uint8_t> service_data_;
  std::string service_uuid_;
  int min_interval_;
  int max_interval_;
};

} // namespace Anki