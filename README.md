# esphome-daly-hkms-bms-can
ESPHome component to monitor DALY H/K/M/S-Series Battery Management Systems via CAN bus.  
Works with other of the "new" DALY BMS as well.

> [!CAUTION]
> This is not tested thorougly, use at your own risk and check if data is plausible!

Also see https://github.com/syssi/esphome-daly-bms for a similar component that uses BLE.

## Configuration

### Main component
```yaml
daly_hkms_bms:
  - id: bms_1
    canbus_id: bms_can
    bms_type: POWER
    address: 1
    update_interval: 10s
```

There can be multiple BMS attached to one CAN bus. To make this work, each BMS needs its own address which can be set in the DALY BMS Tool (Administrator -> Password `12345678` -> Manufacturing -> Board number).

#### Options:
- **id**: ID of this component
- **canbus_id**: ID of the [canbus component](https://esphome.io/components/canbus.html) the BMS is attached to
- **bms_type**: The type of the BMS. Can be `POWER` or `ENERGY_STORAGE`. For most DALY BMS, `POWER` should be correct.
- **address**: The address of the BMS. By default, this is set to 1. The address ("board number") can be set using the DALY PC software.
- **update_interval**: Delay between data requests (default `30s`)
- **update_interval_fast**:
    Delay between interjected "fast" data requests (includes only voltage and current, default: off).
    Be aware that fast requests always have priority, so if normal updates dont (always) work, this might be set too low.

### Sensor component

```yaml
sensor:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    voltage:
      name: "Battery Voltage"
    current:
      name: "Battery Current"
    battery_level:
      name: "Battery Level"
    remaining_capacity:
      name: "Remaining Capacity"
    cycles:
      name: "Battery Cycles"
    temperature_mos:
      name: "Temperature MOS"
    max_temperature:
      name: "Temperature Max"
    min_temperature:
      name: "Temperature Min"
    max_cell_voltage:
      name: "Cell Voltage Max"
    min_cell_voltage:
      name: "Cell Voltage Min"
```

#### Options:

- **daly_hkms_bms_id**: ID of the daly_hkms_bms component (required for multiple BMS).
- **voltage**: Voltage of the battery pack.
- **current**: Current flowing through the BMS (positive when charging, negative when discharging).
- **battery_level**: Battery level in % (SoC).
- **remaining_capacity**: The capacity in Ah left in the battery.
- **cycles**: The number of charge cycles of the battery.
- **balance_current** (not supported on all BMS versions)
- **power**: Power (positive when charging, negative when discharging)
- **energy**: (not supported on all BMS versions)
- **temperature_mos**: The BMS MOSFET temperature.
- **temperature_board**: The BMS PCB temperature (might not be available on all models).
- **temps_number**: The number of temperature sensors.
- **max_temperature**: The highest temperature measured from the temperature sensors.
- **max_temperature_probe_number**: The sensor number which has measured the highest temperature.
- **min_temperature**: The lowest temperature measured from the temperature sensors.
- **min_temperature_probe_number**: The sensor number which has measured the lowest temperature.
- **temperature_1**: The first temperature sensor. There can be up to 8 temperature sensors.
- **cells_number**: The number of cells in series in the battery pack.
- **max_cell_voltage**: The cell of the battery with the highest voltage.
- **max_cell_voltage_number**: The cell number of the battery with the highest voltage.
- **min_cell_voltage**: The cell of the battery with the lowest voltage.
- **min_cell_voltage_number**: The cell number of the battery with the lowest voltage.
- **cell_1_voltage**: The voltage of cell number 1. Cell number can be from 1 to 48.
- **alarm_level_cell_overvoltage**
- **alarm_level_cell_undervoltage**
- **alarm_level_cell_voltage_diff**
- **alarm_level_charge_overtemperature**
- **alarm_level_charge_undertemperature**
- **alarm_level_discharge_overtemperature**
- **alarm_level_discharge_undertemperature**
- **alarm_level_temperature_diff**
- **alarm_level_overvoltage**
- **alarm_level_undervoltage**
- **alarm_level_charge_overcurrent**
- **alarm_level_discharge_overcurrent**
- **alarm_level_soc_low**
- **alarm_level_soh_low**
- **alarm_level_mos_overtemperature**

### Text Sensor component

```yaml
text_sensor:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    status:
      name: "BMS status"
    alerts:
      name: "BMS alerts"
```

#### Options:
- **daly_hkms_bms_id**: ID of the daly_hkms_bms component (required for multiple BMS).
- **status**: The BMS status (Charging, Discharging, Stationary).
- **alerts**: The BMS alerts/messages, newline-separated ("cell volt high lvl 2", "chg mos temp detect fault", etc.).

### Binary Sensor component

```yaml
binary_sensor:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    balancing_active:
      name: "BMS balancing"
    charging_mos_enabled:
      name: "BMS charging FET enabled"
    discharging_mos_enabled:
      name: "BMS discharging FET enabled"
    has_warnings:
      name: "BMS has warnings"
    has_errors:
      name: "BMS has errors"
    canbus_connectivity:
      name: "BMS canbus connectivity"
```

#### Options:
- **daly_hkms_bms_id**: ID of the daly_hkms_bms component (required for multiple BMS).
- **balancing_active**: Whether the BMS is currently balancing or not.
- **cell_1_balancing**: Whether the cell is currently being balanced or not. Cell number can be from 1 to 48.
- **charging_mos_enabled**: BMS charging MOS status.
- **discharging_mos_enabled**: BMS discharging MOS status.
- **precharging_mos_enabled**: BMS precharging MOS status.
- **has_warnings**: BMS warning status.
- **has_errors**: BMS error status.
- **canbus_connectivity**: BMS CAN connectivity status ("on" if the BMS is currently responding to data requests).
- **error_charge_mos_overtemperature**
- **error_charge_mos_temperature_detect**
- **error_discharge_mos_overtemperature**
- **error_discharge_mos_temperature_detect**
- **error_short_circuit**

### Switch component

```yaml
switch:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    charge_mos:
      name: "BMS charge FET"
    discharge_mos:
      name: "BMS discharge FET"
```

#### Options:
- **daly_hkms_bms_id**: ID of the daly_hkms_bms component (required for multiple BMS).
- **charge_mos**: BMS charging MOS switch.
- **discharge_mos**: BMS discharging MOS switch.

### Full Example:
```yaml
external_components:
  - source: github://patagonaa/esphome-daly-hkms-bms@main
    components: [daly_hkms_bms]

canbus:
  - platform: esp32_can
    id: bms_can
    tx_pin: GPIO27
    rx_pin: GPIO26
    use_extended_id: true
    can_id: 0
    bit_rate: 250kbps

daly_hkms_bms:
  - canbus_id: bms_can
    id: bms_1
    address: 1
    update_interval: 10s

sensor:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    voltage:
      name: "BMS total volt"
    current:
      name: "BMS current"
    battery_level:
      name: "BMS state of charge"
    remaining_capacity:
      name: "BMS capacity remaining"
    cycles:
      name: "BMS charging cycles"
    min_cell_voltage:
      name: "BMS cell min volt"
    max_cell_voltage:
      name: "BMS cell max volt"
    delta_cell_voltage:
      name: "BMS cell delta volt"
    temperature_1:
      name: "BMS temperature 1"
    temperature_2:
      name: "BMS temperature 2"
    cell_1_voltage:
      name: "BMS cell volt 01"
    cell_2_voltage:
      name: "BMS cell volt 02"
    # [...]

text_sensor:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    status:
      name: "BMS status"
    alerts:
      name: "BMS alerts"

binary_sensor:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    balancing_active:
      name: "BMS balancing"
    charging_mos_enabled:
      name: "BMS charging FET enabled"
    discharging_mos_enabled:
      name: "BMS discharging FET enabled"
    canbus_connectivity:
      name: "BMS canbus connectivity"

switch:
  - platform: daly_hkms_bms
    daly_hkms_bms_id: bms_1
    charge_mos:
      name: "BMS charge FET"
    discharge_mos:
      name: "BMS discharge FET"
```

## Advanced usage
Setting of registers other than the charge/discharge FETs, especially configuration registers (like warning and error levels) is not implemented in this component, both for security and simplicity reasons. It is however still possible to do so by directly calling the `daly_hkms_bms` component's `write_register(uint16_t reg, std::vector<uint8_t> data)` function.

The writable registers are not made public by DALY themselves, they can however be reverse engineered from the DALY PC software (look in `FrmParameter`, `FrmProduct`, etc.) or sniffed from the bus.
Be aware that the ids differ between `BMS_TYPE_01` ("Energy Storage") and `BMS_TYPE_02` ("Power").

Some maybe useful registers (type 1 / 2):
- `0x181E` / `0x161E` SoC * 10 (100% * 10 = value 1000)

Example:  
To reset the SoC to 100% every 30 seconds if the BMS total voltage is above 51V, you can do:
```yaml
interval:
  - interval: 30s
    then:
      - if:
          condition:
            sensor.in_range:
              id: bms_1_total_volt # sensor id of the BMS total voltage
              above: 51.0
          then:
            - lambda: |-
                id(bms_1).write_register(0x161E, 1000);
```
This will prevent the BMS SoC from dropping while the battery is fully charged. This may be necessary because the BMS only resets the SoC to 100% when switching off due to cell overvoltage, which either means overcharging the cells, or setting the limit unnecessarily low so it can be hit during normal charging.

## BMS connection
Be aware that by default, the BMS goes to sleep after 1 hour of inactivity and can not be woken up via CAN (probably, to be tested!), only by RX/TX UART communication (including the Bluetooth dongle), charging/discharging the battery or toggling the switch input.

The BMS connectors are compatible with JST-GH (1.25mm pin pitch).

### CAN
In a CAN bus, no pins are swapped between sender and receiver(s) (L -> L, H -> H, GND -> GND).

Also, the CAN outputs of the BMS are isolated from the battery (at least on the K-series), so there are no ground issues like the UART pins have.

Still, always connect the ground when using CAN, to avoid stray currents running through the CAN transceiver.

## Support

Please consider donating (via GitHub Sponsors) if this project is useful to you.

You can also support this project in other ways:

- by testing with other BMS models than the ones listed and adding them to the readme
- by reporting bugs (via issues)
- by requesting features (via issues/discussions)
- by contributing code directly (via pull requests)

AI-generated contributions (code or issues) are not welcome and will not be considered.