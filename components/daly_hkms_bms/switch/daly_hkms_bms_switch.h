#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../daly_hkms_bms.h"

#include <vector>

namespace esphome {
namespace daly_hkms_bms {

class DalyHkmsBmsSwitch : public switch_::Switch, public Component, public DalyHkmsBmsInput {
 public:
  void set_parent(DalyHkmsBmsComponent *parent) {
    this->parent_ = parent;
  };
  void set_reg_addr(uint16_t address, bool type_dependant) {
    this->address_ = address;
    // for most registers, the address for "Power" BMS is different by 0xE00 for "Energy Storage" BMS...
    // e.g. 0x182A (energy storage) vs. 0x162A (power)
    if (type_dependant && this->parent_->get_bms_type() == DalyHkmsBmsType::POWER) {
      this->internal_address_ = address ^ 0x0E00;
    } else {
      this->internal_address_ = address;
    }
  };

  uint16_t get_reg_addr() override {
    return this->address_;
  };

  void handle_update(uint16_t value) override {
    this->publish_state(value > 0);
  };

 protected:
  DalyHkmsBmsComponent *parent_;
  uint16_t address_;
  uint16_t internal_address_;

  void write_state(bool state) override {
    std::vector<uint8_t> data = {(uint8_t)(state ? 1 : 0)};
    this->parent_->write_register(this->internal_address_, data);
  };
};

}  // namespace daly_hkms_bms
}  // namespace esphome
