import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.canbus import CanbusComponent
from esphome.const import CONF_ID, CONF_ADDRESS

CODEOWNERS = ["@patagonaa"]
MULTI_CONF = True
DEPENDENCIES = ["canbus"]

CONF_CANBUS_ID = "canbus_id"
CONF_DALY_HKMS_BMS_ID = "daly_hkms_bms_id"
MAX_CELL_NUMBER = 48

daly_hkms_bms_ns = cg.esphome_ns.namespace("daly_hkms_bms")
DalyHkmsBmsComponent = daly_hkms_bms_ns.class_(
    "DalyHkmsBmsComponent", cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DalyHkmsBmsComponent),
        cv.Required(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Optional(CONF_ADDRESS, default=1): cv.positive_int,
    }
)


async def to_code(config):
    canbus = await cg.get_variable(config[CONF_CANBUS_ID])
    canbus_var = cg.new_Pvariable(config[CONF_ID], canbus)
    await cg.register_component(canbus_var, config)

    hub = await cg.get_variable(config[CONF_ID])
    cg.add(hub.set_daly_address(config[CONF_ADDRESS]))
