#pragma once
namespace esphome {
namespace daly_hkms_bms {

static const uint16_t DALY_CAN_CELL_VOLTS = 0x400;

static const uint16_t DALY_CAN_CELL_TEMPS = 0x401;
static const uint16_t DALY_CAN_INFO_0 = 0x402;
static const uint16_t DALY_CAN_INFO_1 = 0x403;
static const uint16_t DALY_CAN_CELL_VOLT_STATS = 0x404;
static const uint16_t DALY_CAN_CELL_TEMP_STATS = 0x405;

static const uint16_t DALY_CAN_STATS_0 = 0x406;
static const uint16_t DALY_CAN_STATS_1 = 0x407;
static const uint16_t DALY_CAN_STATS_2 = 0x408;

static const uint16_t DALY_CAN_FAULT_INFO_0 = 0x409;

static const uint16_t DALY_CAN_BAL_INFO = 0x40A;
static const uint16_t DALY_CAN_CHG_INFO = 0x40B;
static const uint16_t DALY_CAN_RTC_INFO = 0x40C;
static const uint16_t DALY_CAN_LIMIT_STATE = 0x40D;

static const uint16_t DALY_CAN_FAULT_INFO_1 = 0x40E;

static const uint16_t DALY_CAN_REG_CHG_MOS = 0x182A;
static const uint16_t DALY_CAN_REG_DSCHG_MOS = 0x182C;
}  // namespace daly_hkms_bms
}  // namespace esphome
