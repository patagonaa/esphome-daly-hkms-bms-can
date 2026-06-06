#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#include "esphome/components/canbus/canbus.h"

#include <vector>

namespace esphome {
namespace daly_hkms_bms {

static const uint8_t DALY_CAN_MAX_CELL_COUNT = 48;

class DalyHkmsBmsComponent : public Component {
 public:
  DalyHkmsBmsComponent(canbus::Canbus *canbus);
  void setup() override;
  void loop() override;

  void on_frame(uint32_t can_id, bool extended_id, bool rtr, const std::vector<uint8_t> &message);

  void dump_config() override;

  void set_daly_address(uint8_t address);

#ifdef USE_SENSOR
  void set_cell_voltage_sensor(uint16_t cell, sensor::Sensor *sensor) {
    if (cell > this->cell_voltage_sensors_max_)
      this->cell_voltage_sensors_max_ = cell;
    this->cell_voltage_sensors_[cell - 1] = sensor;
  };

  SUB_SENSOR(voltage)
  SUB_SENSOR(current)
  SUB_SENSOR(battery_level)
  SUB_SENSOR(max_cell_voltage)
  SUB_SENSOR(max_cell_voltage_number)
  SUB_SENSOR(min_cell_voltage)
  SUB_SENSOR(min_cell_voltage_number)
  SUB_SENSOR(delta_cell_voltage)
  SUB_SENSOR(max_temperature)
  SUB_SENSOR(max_temperature_probe_number)
  SUB_SENSOR(min_temperature)
  SUB_SENSOR(min_temperature_probe_number)
  SUB_SENSOR(remaining_capacity)
  SUB_SENSOR(cycles)
  SUB_SENSOR(balance_current)
  SUB_SENSOR(cells_number)
  SUB_SENSOR(temps_number)
  SUB_SENSOR(power)
  SUB_SENSOR(charge_power)
  SUB_SENSOR(discharge_power)
  SUB_SENSOR(energy)
  SUB_SENSOR(temperature_mos)
  SUB_SENSOR(temperature_board)

  SUB_SENSOR(alarm_level_cell_overvoltage)
  SUB_SENSOR(alarm_level_cell_undervoltage)
  SUB_SENSOR(alarm_level_cell_voltage_diff)
  SUB_SENSOR(alarm_level_charge_overtemperature)
  SUB_SENSOR(alarm_level_charge_undertemperature)
  SUB_SENSOR(alarm_level_discharge_overtemperature)
  SUB_SENSOR(alarm_level_discharge_undertemperature)
  SUB_SENSOR(alarm_level_temperature_diff)
  SUB_SENSOR(alarm_level_overvoltage)
  SUB_SENSOR(alarm_level_undervoltage)
  SUB_SENSOR(alarm_level_charge_overcurrent)
  SUB_SENSOR(alarm_level_discharge_overcurrent)
  SUB_SENSOR(alarm_level_soc_low)
  SUB_SENSOR(alarm_level_soh_low)
  SUB_SENSOR(alarm_level_mos_overtemperature)

  SUB_SENSOR(temperature_1)
  SUB_SENSOR(temperature_2)
  SUB_SENSOR(temperature_3)
  SUB_SENSOR(temperature_4)
  SUB_SENSOR(temperature_5)
  SUB_SENSOR(temperature_6)
  SUB_SENSOR(temperature_7)
  SUB_SENSOR(temperature_8)
#endif

#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(status)
  SUB_TEXT_SENSOR(alerts)
#endif

#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(charging_mos_enabled)
  SUB_BINARY_SENSOR(discharging_mos_enabled)
  SUB_BINARY_SENSOR(precharging_mos_enabled)
  SUB_BINARY_SENSOR(balancing_active)

  SUB_BINARY_SENSOR(error_charge_mos_overtemperature)
  SUB_BINARY_SENSOR(error_charge_mos_temperature_detect)
  SUB_BINARY_SENSOR(error_discharge_mos_overtemperature)
  SUB_BINARY_SENSOR(error_discharge_mos_temperature_detect)
  SUB_BINARY_SENSOR(error_short_circuit)
  SUB_BINARY_SENSOR(has_warnings)
  SUB_BINARY_SENSOR(has_errors)

  void set_cell_balancing_sensor(uint16_t cell, binary_sensor::BinarySensor *sensor) {
    if (cell > this->cell_balancing_sensors_max_)
      this->cell_balancing_sensors_max_ = cell;
    this->cell_balancing_sensors_[cell - 1] = sensor;
  };
#endif

 protected:
  canbus::Canbus *canbus;
  void handle_msg_cell_volts(const std::vector<uint8_t> &message);
  void publish_sensor_state(sensor::Sensor *sensor, int32_t value, int16_t offset, float factor,  int32_t unavailable_value = -1);

  uint8_t daly_address_;
  uint32_t update_interval_fast_;

#ifdef USE_SENSOR
  sensor::Sensor *cell_voltage_sensors_[DALY_CAN_MAX_CELL_COUNT]{};
#endif
  uint16_t cell_voltage_sensors_max_{0};

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *cell_balancing_sensors_[DALY_CAN_MAX_CELL_COUNT]{};
#endif
  uint16_t cell_balancing_sensors_max_{0};
};

}  // namespace daly_hkms_bms
}  // namespace esphome
