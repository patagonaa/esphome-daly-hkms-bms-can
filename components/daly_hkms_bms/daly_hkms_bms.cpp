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
    publish_sensor_state_(this->max_temperature_sensor_, (message[0] << 8) | message[1], 0, 0.001);
    publish_sensor_state_(this->max_temperature_probe_number_sensor_, message[2], 0, 1);

    publish_sensor_state_(this->min_temperature_sensor_, (message[3] << 8) | message[4], 0, 0.001);
    publish_sensor_state_(this->min_temperature_probe_number_sensor_, message[5], 0, 1);
    
    // publish_sensor_state_(this->delta_temperature_sensor_, (message[6] << 8) | message[7], 0, 0.001);
    break;

  case DALY_CAN_STATS_1:
    break;

  case DALY_CAN_STATS_2:
    publish_sensor_state_(this->cells_number_sensor_, message[0], 0, 1);
    publish_sensor_state_(this->temps_number_sensor_, message[1], 0, 1);

    publish_sensor_state_(this->remaining_capacity_sensor_, (message[2] << 24) | (message[3] << 16) | (message[4] << 8) | message[5], 0, 0.001);
    publish_sensor_state_(this->cycles_sensor_, (message[6] << 8) | message[7], 0, 0.001);
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
    publish_sensor_state_(this->balancing_active_binary_sensor_, message[0] == 2);

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
    handle_msg_fault_info_1(message);
    break;

  default:
    break;
  }
}

void DalyHkmsBmsComponent::handle_msg_cell_volts_(const std::vector<uint8_t> &message) {
  uint8_t msg_num = message[0];

#ifdef USE_SENSOR
  for (size_t i = 0; i < 3; i++)
  {
    uint16_t mv = (message[1 + i * 2] << 8) | message[2 + i * 2];
    size_t cell_num = msg_num*3 + i;
    if (cell_num >= this->cell_voltage_sensors_max_) {
      publish_sensor_state_(this->cell_voltage_sensors_[cell_num], mv, 0, 0.001);
    }
  }
#endif
}

void DalyHkmsBmsComponent::handle_msg_cell_temps_(const std::vector<uint8_t> &message) {
  uint8_t msg_num = message[0];

#ifdef USE_SENSOR
  for (size_t i = 0; i < 7; i++)
  {
    uint16_t val = message[1 + i];
    size_t temp_num = msg_num*7 + i;
    if (temp_num >= this->temperature_sensors_max_) {
      publish_sensor_state_(this->temperature_sensors_[temp_num], val, -40, 1);
    }
  }
#endif
}

void DalyHkmsBmsComponent::handle_msg_fault_info_1(const std::vector<uint8_t> &message) {
  uint8_t msg_num = message[0];
  static_assert(sizeof(DalyHkmsStatus) == 14);
  if (msg_num == 1) {
    std::memcpy(&this->fault_status_, message.data() + 1, 7);
  }
  if (msg_num == 2) {
    std::memcpy((&this->fault_status_) + 7, message.data() + 1, 7);
  }

#ifdef USE_SENSOR
  publish_sensor_state_(this->alarm_level_cell_overvoltage_sensor_, this->fault_status_.lvl_cell_ovp, 0, 1);
  publish_sensor_state_(this->alarm_level_cell_undervoltage_sensor_, this->fault_status_.lvl_cell_uvp, 0, 1);
  publish_sensor_state_(this->alarm_level_cell_voltage_diff_sensor_, this->fault_status_.lvl_cell_volt_diff, 0, 1);
  publish_sensor_state_(this->alarm_level_charge_overtemperature_sensor_, this->fault_status_.lvl_chg_overtemp, 0, 1);
  publish_sensor_state_(this->alarm_level_charge_undertemperature_sensor_, this->fault_status_.lvl_chg_undertemp, 0, 1);
  publish_sensor_state_(this->alarm_level_discharge_overtemperature_sensor_, this->fault_status_.lvl_dschg_overtemp, 0, 1);
  publish_sensor_state_(this->alarm_level_discharge_undertemperature_sensor_, this->fault_status_.lvl_dschg_undertemp, 0, 1);
  publish_sensor_state_(this->alarm_level_temperature_diff_sensor_, this->fault_status_.lvl_temp_diff, 0, 1);
  publish_sensor_state_(this->alarm_level_overvoltage_sensor_, this->fault_status_.lvl_total_ovp, 0, 1);
  publish_sensor_state_(this->alarm_level_undervoltage_sensor_, this->fault_status_.lvl_total_uvp, 0, 1);
  publish_sensor_state_(this->alarm_level_charge_overcurrent_sensor_, this->fault_status_.lvl_chg_ocp, 0, 1);
  publish_sensor_state_(this->alarm_level_discharge_overcurrent_sensor_, this->fault_status_.lvl_dschg_ocp, 0, 1);
  publish_sensor_state_(this->alarm_level_soc_low_sensor_, this->fault_status_.lvl_soc_low, 0, 1);
  publish_sensor_state_(this->alarm_level_soh_low_sensor_, this->fault_status_.lvl_soh_low, 0, 1);
  publish_sensor_state_(this->alarm_level_mos_overtemperature_sensor_, this->fault_status_.lvl_mos_overtemp, 0, 1);
#endif

#ifdef USE_BINARY_SENSOR
  bool has_warnings = false;
  bool has_errors = false;

  // Code 0-1
  has_warnings |= (this->fault_status_.lvl_cell_ovp) > 0;
  has_errors |= (this->fault_status_.lvl_cell_ovp) > 1;
  has_warnings |= (this->fault_status_.lvl_cell_uvp) > 0;
  has_errors |= (this->fault_status_.lvl_cell_uvp) > 1;
  has_errors |= this->fault_status_.err_smart_charger_connection;

  has_warnings |= (this->fault_status_.lvl_cell_volt_diff) > 0;
  has_errors |= (this->fault_status_.lvl_cell_volt_diff) > 1;
  has_warnings |= (this->fault_status_.lvl_chg_overtemp) > 0;
  has_errors |= (this->fault_status_.lvl_chg_overtemp) > 1;
  has_errors |= this->fault_status_.err_smart_discharger_connection;

  // Code 2-3
  has_warnings |= (this->fault_status_.lvl_chg_undertemp) > 0;
  has_errors |= (this->fault_status_.lvl_chg_undertemp) > 1;

  has_warnings |= (this->fault_status_.lvl_dschg_overtemp) > 0;
  has_errors |= (this->fault_status_.lvl_dschg_overtemp) > 1;

  has_errors |= this->fault_status_.err_chg_mos_temp_high;
  publish_sensor_state_(this->error_charge_mos_overtemperature_binary_sensor_, this->fault_status_.err_chg_mos_temp_high);
  
  has_errors |= this->fault_status_.err_chg_mos_temp_detect;
  publish_sensor_state_(this->error_charge_mos_temperature_detect_binary_sensor_, this->fault_status_.err_chg_mos_temp_detect);

  has_errors |= this->fault_status_.err_dschg_mos_temp_high;
  publish_sensor_state_(this->error_discharge_mos_overtemperature_binary_sensor_, this->fault_status_.err_dschg_mos_temp_high);
  
  has_errors |= this->fault_status_.err_dschg_mos_temp_detect;
  publish_sensor_state_(this->error_discharge_mos_temperature_detect_binary_sensor_, this->fault_status_.err_dschg_mos_temp_detect);
  
  // Code 4-5
  has_warnings |= (this->fault_status_.lvl_total_ovp) > 0;
  has_errors |= (this->fault_status_.lvl_total_ovp) > 1;

  has_warnings |= (this->fault_status_.lvl_total_uvp) > 0;
  has_errors |= (this->fault_status_.lvl_total_uvp) > 1;

  has_errors |= this->fault_status_.err_short_circuit;
  publish_sensor_state_(this->error_short_circuit_binary_sensor_, this->fault_status_.err_short_circuit);
  
  has_warnings |= (this->fault_status_.lvl_chg_ocp) > 0;
  has_errors |= (this->fault_status_.lvl_chg_ocp) > 1;

  has_warnings |= (this->fault_status_.lvl_dschg_ocp) > 0;
  has_errors |= (this->fault_status_.lvl_dschg_ocp) > 1;
  
  has_errors |= this->fault_status_.err_chg_undervoltage;
  has_errors |= this->fault_status_.err_dschg_overvoltage;

  // Code 6-7
  // SoC and SoH are never errors
  has_warnings |= (this->fault_status_.lvl_soc_low) > 0;
  has_warnings |= (this->fault_status_.lvl_soh_low) > 0;

  has_errors |= this->fault_status_.err_parallel_comm;

  has_warnings |= (this->fault_status_.lvl_mos_overtemp) > 0;
  has_errors |= (this->fault_status_.lvl_mos_overtemp) > 1;

  has_warnings |= (this->fault_status_.lvl_thermal_runaway) > 0;
  has_errors |= (this->fault_status_.lvl_thermal_runaway) > 1;

  // Code 10-11
  has_errors |= this->fault_status_.err_afe_chip;
  has_errors |= this->fault_status_.err_afe_comm;
  has_errors |= this->fault_status_.err_afe_sampling;
  has_errors |= this->fault_status_.err_volt_detect;
  has_errors |= this->fault_status_.err_volt_detect_disconnected;
  has_errors |= this->fault_status_.err_volt_total_detect;
  has_errors |= this->fault_status_.err_curr_detect;
  has_errors |= this->fault_status_.err_temp_detect;

  // Code 12-13
  has_errors |= this->fault_status_.err_temp_disconnected;
  has_errors |= this->fault_status_.err_eeprom;
  has_errors |= this->fault_status_.err_flash;
  has_errors |= this->fault_status_.err_rtc;
  has_errors |= this->fault_status_.err_chg_mos;
  has_errors |= this->fault_status_.err_dschg_mos;
  has_errors |= this->fault_status_.err_prechg_mos;
  has_errors |= this->fault_status_.err_prechg;

  has_errors |= this->fault_status_.err_heating;

  publish_sensor_state_(this->has_warnings_binary_sensor_, has_warnings || has_errors);
  publish_sensor_state_(this->has_errors_binary_sensor_, has_errors);
#endif
}

void DalyHkmsBmsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DALY HKMS BMS:");
  ESP_LOGCONFIG(TAG, "  Address: %d", this->daly_address_);
}

}  // namespace daly_hkms_bms
}  // namespace esphome
