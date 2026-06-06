#include "daly_hkms_bms.h"
#include "daly_hkms_bms_registers.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include <cinttypes>
#include <cstring>
#include <sstream>

namespace esphome {
namespace daly_hkms_bms {

static const char *const TAG = "daly_hkms_bms";

DalyHkmsBmsComponent::DalyHkmsBmsComponent(canbus::Canbus *canbus) { this->canbus = canbus; }

void DalyHkmsBmsComponent::set_daly_address(uint8_t daly_address) {
  this->daly_address_ = daly_address;
}

void DalyHkmsBmsComponent::setup() {
  auto cb = [this](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data) -> void {
    this->on_frame(can_id, extended_id, rtr, data);
  };

  this->canbus->add_callback(cb);
}

void DalyHkmsBmsComponent::loop() {
}

void DalyHkmsBmsComponent::on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &message) {
  if (message.size() < 8 || !extended_id) {
    return;
  }

  uint8_t bms_id = can_id & 0x000000FF;
  if (bms_id != this->daly_address_) {
    return;
  }

  uint16_t register_id = (can_id & 0xFFF00000) >> 20;

  switch (register_id)
  {
  case DALY_CAN_CELL_VOLTS:
    this->handle_msg_cell_volts(message);
    break;
  
  default:
    break;
  }
}

void DalyHkmsBmsComponent::handle_msg_cell_volts(const std::vector<uint8_t> &message) {
  uint8_t msg_num = message[0];

  for (size_t i = 0; i < 3; i++)
  {
    uint16_t mv = message[1 + i * 2] | (message[2 + i * 2] << 8);
    size_t cell_num = msg_num*3 + i;
    if (cell_num >= this->cell_voltage_sensors_max_) {
      publish_sensor_state(this->cell_voltage_sensors_[cell_num], mv, 0, 0.001);
    }
  }
}

void DalyHkmsBmsComponent::publish_sensor_state(sensor::Sensor *sensor, int32_t value, int16_t offset, float factor, int32_t unavailable_value) {
  if (sensor == nullptr)
    return;
  float out_value = value == unavailable_value ? NAN : (value + offset) * factor;
  sensor->publish_state(out_value);
}

void DalyHkmsBmsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DALY HKMS BMS:");
  ESP_LOGCONFIG(TAG, "  Address: %d", this->daly_address_);
}

}  // namespace daly_hkms_bms
}  // namespace esphome
