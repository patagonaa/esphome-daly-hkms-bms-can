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

static const uint32_t DALY_CAN_TIMEOUT_MS = 1000;

DalyHkmsBmsComponent::DalyHkmsBmsComponent(canbus::Canbus *canbus) { this->canbus = canbus; }

void DalyHkmsBmsComponent::setup() {
  auto cb = [this](uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &data) -> void {
    this->on_frame(can_id, extended_id, rtr, data);
  };

  this->canbus->add_callback(cb);

  // needs to be sent regularly so daly bms sends periodic data
  this->set_interval("request_data", 2000, [this]() { this->canbus->send_data(0x400FF80, true, false, {0, 0, 0, 0, 0, 0, 0, 0}); });

  // give ESPHome some time to settle before throwing warnings
  this->last_update_ = millis() + 5000;
}

void DalyHkmsBmsComponent::loop() {
  bool canbus_connectivity = millis() < this->last_update_ + DALY_CAN_TIMEOUT_MS;
  if (canbus_connectivity != this->canbus_connectivity_) {
    this->canbus_connectivity_ = canbus_connectivity;
#ifdef USE_BINARY_SENSOR
    publish_sensor_state_(this->canbus_connectivity_binary_sensor_, canbus_connectivity);
#endif
    if (!canbus_connectivity) {
      ESP_LOGW(TAG, "BMS %d: no response after %d ms", this->daly_address_, DALY_CAN_TIMEOUT_MS);
      this->status_set_warning(LOG_STR("BMS read timeout"));
    } else {
      ESP_LOGW(TAG, "BMS %d: available again", this->daly_address_);
      this->status_clear_warning();
    }
  }
}

void DalyHkmsBmsComponent::on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &message) {
  if (message.size() < 8 || !extended_id) {
    return;
  }

  uint8_t bms_id = can_id & 0x00000FF;
  if (bms_id != this->daly_address_) {
    return;
  }

  uint16_t register_id = (can_id & 0xFFFF0000) >> 16;

  this->last_update_ = millis();
  ESP_LOGD(TAG, "BMS %d: got register %x", this->daly_address_, register_id);

  switch (register_id)
  {
  case DALY_CAN_CELL_VOLTS:
  case DALY_CAN_CELL_VOLTS_ALT:
    handle_msg_cell_volts_(message);
    break;

  case DALY_CAN_CELL_TEMPS:
    handle_msg_cell_temps_(message);
    break;

#ifdef USE_SENSOR
  case DALY_CAN_INFO_0:
    publish_sensor_state_(this->voltage_sensor_, (message[0] << 8) | message[1], 0, 0.1);
    publish_sensor_state_(this->current_sensor_, (message[2] << 8) | message[3], -30000, 0.1);
    publish_sensor_state_(this->battery_level_sensor_, (message[4] << 8) | message[5], 0, 0.1);
    break;

  case DALY_CAN_INFO_1:
    publish_sensor_state_(this->power_sensor_, (message[0] << 8) | message[1], 0, 1);
    publish_sensor_state_(this->energy_sensor_, (message[2] << 8) | message[3], 0, 1);
    publish_sensor_state_(this->temperature_mos_sensor_, message[4], -40, 1);
    publish_sensor_state_(this->temperature_board_sensor_, message[5], -40, 1);
    break;

  case DALY_CAN_CELL_VOLT_STATS:
    publish_sensor_state_(this->max_cell_voltage_sensor_, (message[0] << 8) | message[1], 0, 0.001);
    publish_sensor_state_(this->max_cell_voltage_number_sensor_, message[2], 0, 1);

    publish_sensor_state_(this->min_cell_voltage_sensor_, (message[3] << 8) | message[4], 0, 0.001);
    publish_sensor_state_(this->min_cell_voltage_number_sensor_, message[5], 0, 1);

    publish_sensor_state_(this->delta_cell_voltage_sensor_, (message[6] << 8) | message[7], 0, 0.001);
    break;

  case DALY_CAN_CELL_TEMP_STATS:
    publish_sensor_state_(this->max_temperature_sensor_, message[0], -40, 1);
    publish_sensor_state_(this->max_temperature_probe_number_sensor_, message[1], 0, 1);

    publish_sensor_state_(this->min_temperature_sensor_, message[2], -40, 1);
    publish_sensor_state_(this->min_temperature_probe_number_sensor_, message[3], 0, 1);
    
    // publish_sensor_state_(this->delta_temperature_sensor_, (message[6] << 8) | message[7], 0, 0.001);
    break;

  case DALY_CAN_STATS_2:
    publish_sensor_state_(this->cells_number_sensor_, message[0], 0, 1);
    publish_sensor_state_(this->temps_number_sensor_, message[1], 0, 1);

    publish_sensor_state_(this->remaining_capacity_sensor_, (message[2] << 24) | (message[3] << 16) | (message[4] << 8) | message[5], 0, 0.001);
    publish_sensor_state_(this->cycles_sensor_, (message[6] << 8) | message[7], 0, 1);
    break;
#endif

#ifdef USE_TEXT_SENSOR
  case DALY_CAN_STATS_1:
    if (this->status_text_sensor_ != nullptr) {
      switch (message[0]) {
        case 0:
          this->status_text_sensor_->publish_state("Stationary");
          break;
        case 1:
          this->status_text_sensor_->publish_state("Charging");
          break;
        case 2:
          this->status_text_sensor_->publish_state("Discharging");
          break;
        default:
          break;
      }
    }
    break;
#endif

#ifdef USE_BINARY_SENSOR
  case DALY_CAN_STATS_0:
    publish_sensor_state_(this->charging_mos_enabled_binary_sensor_, message[0]);
    publish_sensor_state_(this->discharging_mos_enabled_binary_sensor_, message[1]);
    publish_sensor_state_(this->precharging_mos_enabled_binary_sensor_, message[2]);
    break;
#endif

  case DALY_CAN_BAL_INFO:
#ifdef USE_BINARY_SENSOR
    publish_sensor_state_(this->balancing_active_binary_sensor_, message[0] > 0);

    for (size_t i = 0; i < DALY_CAN_MAX_CELL_COUNT; i++) {
      size_t field = 4 + i / 8;
      size_t bit = i % 8;
      publish_sensor_state_(this->cell_balancing_sensors_[i], message[field] & (1 << bit));
    }
#endif
#ifdef USE_SENSOR
    publish_sensor_state_(this->balance_current_sensor_, (message[2] << 8) | message[3], -30000, 0.001);
#endif
    break;

  case DALY_CAN_FAULT_INFO_1:
    handle_msg_fault_info_1_(message);
    break;

  default:
    break;
  }
}

void DalyHkmsBmsComponent::handle_msg_cell_volts_(const std::vector<uint8_t> &message) {
  uint8_t page_num = message[0];

#ifdef USE_SENSOR
  for (size_t i = 0; i < 3; i++)
  {
    uint16_t mv = (message[1 + i * 2] << 8) | message[2 + i * 2];
    size_t cell_num = (page_num-1)*3 + i;
    if (cell_num < this->cell_voltage_sensors_max_) {
      publish_sensor_state_(this->cell_voltage_sensors_[cell_num], mv, 0, 0.001);
    }
  }
#endif
}

void DalyHkmsBmsComponent::handle_msg_cell_temps_(const std::vector<uint8_t> &message) {
  uint8_t page_num = message[0];

#ifdef USE_SENSOR
  for (size_t i = 0; i < 7; i++)
  {
    uint16_t val = message[1 + i];
    size_t temp_num = (page_num-1)*7 + i;
    if (temp_num < this->temperature_sensors_max_) {
      publish_sensor_state_(this->temperature_sensors_[temp_num], val, -40, 1);
    }
  }
#endif
}

void DalyHkmsBmsComponent::handle_msg_fault_info_1_(const std::vector<uint8_t> &message) {
  uint8_t page_num = message[0];

  static_assert(DALY_CAN_FAULT_STATUS_LEN == 14);
  if (page_num == 1) {
    std::memcpy(this->fault_status_, message.data() + 1, 7);
  }
  if (page_num == 2) {
    std::memcpy(this->fault_status_ + 7, message.data() + 1, 7);
  }

  DalyHkmsStatus status = *((DalyHkmsStatus*)this->fault_status_);

#ifdef USE_SENSOR
  publish_sensor_state_(this->alarm_level_cell_overvoltage_sensor_, status.lvl_cell_ovp, 0, 1);
  publish_sensor_state_(this->alarm_level_cell_undervoltage_sensor_, status.lvl_cell_uvp, 0, 1);
  publish_sensor_state_(this->alarm_level_cell_voltage_diff_sensor_, status.lvl_cell_volt_diff, 0, 1);
  publish_sensor_state_(this->alarm_level_charge_overtemperature_sensor_, status.lvl_chg_overtemp, 0, 1);
  publish_sensor_state_(this->alarm_level_charge_undertemperature_sensor_, status.lvl_chg_undertemp, 0, 1);
  publish_sensor_state_(this->alarm_level_discharge_overtemperature_sensor_, status.lvl_dschg_overtemp, 0, 1);
  publish_sensor_state_(this->alarm_level_discharge_undertemperature_sensor_, status.lvl_dschg_undertemp, 0, 1);
  publish_sensor_state_(this->alarm_level_temperature_diff_sensor_, status.lvl_temp_diff, 0, 1);
  publish_sensor_state_(this->alarm_level_overvoltage_sensor_, status.lvl_total_ovp, 0, 1);
  publish_sensor_state_(this->alarm_level_undervoltage_sensor_, status.lvl_total_uvp, 0, 1);
  publish_sensor_state_(this->alarm_level_charge_overcurrent_sensor_, status.lvl_chg_ocp, 0, 1);
  publish_sensor_state_(this->alarm_level_discharge_overcurrent_sensor_, status.lvl_dschg_ocp, 0, 1);
  publish_sensor_state_(this->alarm_level_soc_low_sensor_, status.lvl_soc_low, 0, 1);
  publish_sensor_state_(this->alarm_level_soh_low_sensor_, status.lvl_soh_low, 0, 1);
  publish_sensor_state_(this->alarm_level_mos_overtemperature_sensor_, status.lvl_mos_overtemp, 0, 1);
#endif

  for (auto &input : this->registered_inputs_) {
    uint16_t address = input->get_reg_addr();
    if (address == DALY_CAN_REG_CHG_MOS)
      input->handle_update(!status.chg_mos_off_bus);
    if (address == DALY_CAN_REG_DSCHG_MOS)
      input->handle_update(!status.dschg_mos_off_bus);
  }

#ifdef USE_BINARY_SENSOR
  bool has_warnings = false;
  bool has_errors = false;

  // Code 0-1
  has_warnings |= (status.lvl_cell_ovp) > 0;
  has_errors |= (status.lvl_cell_ovp) > 1;
  has_warnings |= (status.lvl_cell_uvp) > 0;
  has_errors |= (status.lvl_cell_uvp) > 1;
  has_errors |= status.err_smart_charger_connection;

  has_warnings |= (status.lvl_cell_volt_diff) > 0;
  has_errors |= (status.lvl_cell_volt_diff) > 1;
  has_warnings |= (status.lvl_chg_overtemp) > 0;
  has_errors |= (status.lvl_chg_overtemp) > 1;
  has_errors |= status.err_smart_discharger_connection;

  // Code 2-3
  has_warnings |= (status.lvl_chg_undertemp) > 0;
  has_errors |= (status.lvl_chg_undertemp) > 1;

  has_warnings |= (status.lvl_dschg_overtemp) > 0;
  has_errors |= (status.lvl_dschg_overtemp) > 1;

  has_errors |= status.err_chg_mos_temp_high;
  publish_sensor_state_(this->error_charge_mos_overtemperature_binary_sensor_, status.err_chg_mos_temp_high);
  
  has_errors |= status.err_chg_mos_temp_detect;
  publish_sensor_state_(this->error_charge_mos_temperature_detect_binary_sensor_, status.err_chg_mos_temp_detect);

  has_errors |= status.err_dschg_mos_temp_high;
  publish_sensor_state_(this->error_discharge_mos_overtemperature_binary_sensor_, status.err_dschg_mos_temp_high);
  
  has_errors |= status.err_dschg_mos_temp_detect;
  publish_sensor_state_(this->error_discharge_mos_temperature_detect_binary_sensor_, status.err_dschg_mos_temp_detect);
  
  // Code 4-5
  has_warnings |= (status.lvl_total_ovp) > 0;
  has_errors |= (status.lvl_total_ovp) > 1;

  has_warnings |= (status.lvl_total_uvp) > 0;
  has_errors |= (status.lvl_total_uvp) > 1;

  has_errors |= status.err_short_circuit;
  publish_sensor_state_(this->error_short_circuit_binary_sensor_, status.err_short_circuit);
  
  has_warnings |= (status.lvl_chg_ocp) > 0;
  has_errors |= (status.lvl_chg_ocp) > 1;

  has_warnings |= (status.lvl_dschg_ocp) > 0;
  has_errors |= (status.lvl_dschg_ocp) > 1;
  
  has_errors |= status.err_chg_undervoltage;
  has_errors |= status.err_dschg_overvoltage;

  // Code 6-7
  // SoC and SoH are never errors
  has_warnings |= (status.lvl_soc_low) > 0;
  has_warnings |= (status.lvl_soh_low) > 0;

  has_errors |= status.err_parallel_comm;

  has_warnings |= (status.lvl_mos_overtemp) > 0;
  has_errors |= (status.lvl_mos_overtemp) > 1;

  has_warnings |= (status.lvl_thermal_runaway) > 0;
  has_errors |= (status.lvl_thermal_runaway) > 1;

  // Code 10-11
  has_errors |= status.err_afe_chip;
  has_errors |= status.err_afe_comm;
  has_errors |= status.err_afe_sampling;
  has_errors |= status.err_volt_detect;
  has_errors |= status.err_volt_detect_disconnected;
  has_errors |= status.err_volt_total_detect;
  has_errors |= status.err_curr_detect;
  has_errors |= status.err_temp_detect;

  // Code 12-13
  has_errors |= status.err_temp_disconnected;
  has_errors |= status.err_eeprom;
  has_errors |= status.err_flash;
  has_errors |= status.err_rtc;
  has_errors |= status.err_chg_mos;
  has_errors |= status.err_dschg_mos;
  has_errors |= status.err_prechg_mos;
  has_errors |= status.err_prechg;

  has_errors |= status.err_heating;

  publish_sensor_state_(this->has_warnings_binary_sensor_, has_warnings || has_errors);
  publish_sensor_state_(this->has_errors_binary_sensor_, has_errors);
#endif

#ifdef USE_TEXT_ALERTS
  if (this->alerts_text_sensor_ != nullptr) {
    std::ostringstream alerts_buffer;

    // Code 0-1
    if (status.lvl_cell_ovp > 0) {
      alerts_buffer << "cell volt high lvl " << int(status.lvl_cell_ovp) << "\n";
    }
    if (status.lvl_cell_uvp > 0) {
      alerts_buffer << "cell volt low lvl " << int(status.lvl_cell_uvp) << "\n";
    }
    if (status.lvl_cell_volt_diff > 0) {
      alerts_buffer << "cell volt diff lvl " << int(status.lvl_cell_volt_diff) << "\n";
    }
    if (status.lvl_chg_overtemp > 0) {
      alerts_buffer << "chg temp high lvl " << int(status.lvl_chg_overtemp) << "\n";
    }

    if (status.smart_charger_connected) {
      alerts_buffer << "smart charger connected\n";
    }
    if (status.err_smart_charger_connection) {
      alerts_buffer << "smart charger disconnected\n";
    }

    if (status.smart_discharger_connected) {
      alerts_buffer << "smart discharger connected\n";
    }
    if (status.err_smart_discharger_connection) {
      alerts_buffer << "smart discharger disconnected\n";
    }

    // Code 2-3
    if (status.lvl_chg_undertemp > 0) {
      alerts_buffer << "chg temp low lvl " << int(status.lvl_chg_undertemp) << "\n";
    }
    if (status.lvl_dschg_overtemp > 0) {
      alerts_buffer << "dschg temp high lvl " << int(status.lvl_dschg_overtemp) << "\n";
    }
    if (status.lvl_dschg_undertemp > 0) {
      alerts_buffer << "dschg temp low lvl " << int(status.lvl_dschg_undertemp) << "\n";
    }
    if (status.lvl_temp_diff > 0) {
      alerts_buffer << "temp diff lvl " << int(status.lvl_temp_diff) << "\n";
    }

    if (status.err_chg_mos_temp_high) {
      alerts_buffer << "chg mos temp high\n";
    }
    if (status.err_chg_mos_temp_detect) {
      alerts_buffer << "chg mos temp detect fault\n";
    }

    if (status.err_dschg_mos_temp_high) {
      alerts_buffer << "dschg mos temp high\n";
    }
    if (status.err_dschg_mos_temp_detect) {
      alerts_buffer << "dschg mos temp detect fault\n";
    }

    // Code 4-5
    if (status.lvl_total_ovp > 0) {
      alerts_buffer << "total volt high lvl " << int(status.lvl_total_ovp) << "\n";
    }
    if (status.lvl_total_uvp > 0) {
      alerts_buffer << "total volt low lvl " << int(status.lvl_total_uvp) << "\n";
    }
    if (status.lvl_chg_ocp > 0) {
      alerts_buffer << "chg curr high lvl " << int(status.lvl_chg_ocp) << "\n";
    }
    if (status.lvl_dschg_ocp > 0) {
      alerts_buffer << "dschg curr high lvl " << int(status.lvl_dschg_ocp) << "\n";
    }

    if (status.err_short_circuit) {
      alerts_buffer << "short circuit protect\n";
    }
    if (status.upgrade_sign) {
      alerts_buffer << "upgrade sign\n";
    }

    if (status.err_chg_undervoltage) {
      alerts_buffer << "charge undervoltage\n";
    }
    if (status.err_dschg_overvoltage) {
      alerts_buffer << "discharge overvoltage\n";
    }

    // Code 6-7
    if (status.lvl_soc_low > 0) {
      alerts_buffer << "soc low lvl " << int(status.lvl_soc_low) << "\n";
    }
    if (status.lvl_soh_low > 0) {
      alerts_buffer << "soh low lvl " << int(status.lvl_soh_low) << "\n";
    }
    if (status.lvl_mos_overtemp > 0) {
      alerts_buffer << "mos temp high lvl " << int(status.lvl_mos_overtemp) << "\n";
    }
    if (status.lvl_thermal_runaway > 0) {
      alerts_buffer << "thermal runaway lvl " << int(status.lvl_thermal_runaway) << "\n";
    }

    if (status.parallel_comm) {
      alerts_buffer << "parallel comm ok\n";
    }
    if (status.err_parallel_comm) {
      alerts_buffer << "parallel comm fault\n";
    }

    // Code 10-11
    if (status.err_afe_chip) {
      alerts_buffer << "afe ic fault\n";
    }
    if (status.err_afe_comm) {
      alerts_buffer << "afe ic comm fault\n";
    }
    if (status.err_afe_sampling) {
      alerts_buffer << "afe ic ad fault\n";
    }
    if (status.err_volt_detect) {
      alerts_buffer << "cell volt detect fault\n";
    }
    if (status.err_volt_detect_disconnected) {
      alerts_buffer << "cell volt detect disconnected\n";
    }
    if (status.err_volt_total_detect) {
      alerts_buffer << "total volt detect fault\n";
    }
    if (status.err_curr_detect) {
      alerts_buffer << "curr detect fault\n";
    }
    if (status.err_temp_detect) {
      alerts_buffer << "temp detect fault\n";
    }

    // Code 12-13
    if (status.err_temp_disconnected) {
      alerts_buffer << "temp detect disconnected\n";
    }
    if (status.err_eeprom) {
      alerts_buffer << "EEPROM fault\n";
    }
    if (status.err_flash) {
      alerts_buffer << "flash fault\n";
    }
    if (status.err_rtc) {
      alerts_buffer << "RTC fault\n";
    }
    if (status.err_chg_mos) {
      alerts_buffer << "chg mos fault\n";
    }
    if (status.err_dschg_mos) {
      alerts_buffer << "dschg mos fault\n";
    }
    if (status.err_prechg_mos) {
      alerts_buffer << "prechg mos fault\n";
    }
    if (status.err_prechg) {
      alerts_buffer << "prechg failed\n";
    }

    if (status.chg_mos_off_bus) {
      alerts_buffer << "chg mos off (via comm)\n";
    }
    if (status.dschg_mos_off_bus) {
      alerts_buffer << "dschg mos off (via comm)\n";
    }
    if (status.chg_mos_off_switch) {
      alerts_buffer << "chg mos off (via switch)\n";
    }
    if (status.dschg_mos_off_switch) {
      alerts_buffer << "dschg mos off (via switch)\n";
    }
    if (status.fan_active) {
      alerts_buffer << "fan active\n";
    }
    if (status.heating_active) {
      alerts_buffer << "heater active\n";
    }
    if (status.current_limit_active) {
      alerts_buffer << "current limit active\n";
    }
    if (status.err_heating) {
      alerts_buffer << "heater fault\n";
    }

    std::string alerts_str = alerts_buffer.str();
    // remove trailing newline
    if (!alerts_str.empty())
      alerts_str.pop_back();

    this->alerts_text_sensor_->publish_state(alerts_str);
  }
#endif
}

void DalyHkmsBmsComponent::write_register(uint16_t reg, const std::vector<uint8_t> &data) {
  ESP_LOGD(TAG, "BMS %d: setting %x to state %d", this->daly_address_, reg, data[0]);
  uint32_t can_id = (uint32_t(reg) << 16) | (uint32_t(this->daly_address_) << 8) | 0x80;
  this->canbus->send_data(can_id, true, false, data);
}

void DalyHkmsBmsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DALY HKMS BMS:");
  ESP_LOGCONFIG(TAG, "  Address: %d", this->daly_address_);
}

}  // namespace daly_hkms_bms
}  // namespace esphome
