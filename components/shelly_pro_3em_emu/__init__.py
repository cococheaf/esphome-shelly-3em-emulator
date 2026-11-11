import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import mqtt, sensor, time
from esphome.components.esp32 import include_builtin_idf_component
from esphome.const import CONF_ID

AUTO_LOAD = ["json"]
DEPENDENCIES = ["api", "mqtt", "wifi", "esp32"]

CONF_MQTT_ID = "mqtt_id"
CONF_TOTAL_POWER = "total_power"
CONF_IMPORT_ENERGY = "import_energy"
CONF_EXPORT_ENERGY = "export_energy"
CONF_PHASES = "phases"
CONF_VOLTAGE = "voltage"
CONF_CURRENT = "current"
CONF_POWER = "power"
CONF_POWER_FACTOR = "power_factor"
CONF_PHASE_IMPORT_ENERGY = "import_energy"
CONF_PHASE_EXPORT_ENERGY = "export_energy"
CONF_PUBLISH_INTERVAL = "publish_interval"
CONF_TIME_ID = "time_id"

emu_ns = cg.esphome_ns.namespace("shelly_pro_3em_emu")
ShellyPro3EMEmu = emu_ns.class_("ShellyPro3EMEmu", cg.Component)

PHASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_VOLTAGE): cv.use_id(sensor.Sensor),
        cv.Required(CONF_CURRENT): cv.use_id(sensor.Sensor),
        cv.Required(CONF_POWER): cv.use_id(sensor.Sensor),
        cv.Required(CONF_POWER_FACTOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_PHASE_IMPORT_ENERGY): cv.use_id(sensor.Sensor),
        cv.Required(CONF_PHASE_EXPORT_ENERGY): cv.use_id(sensor.Sensor),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ShellyPro3EMEmu),
        cv.GenerateID(CONF_MQTT_ID): cv.use_id(mqtt.MQTTClientComponent),
        cv.Required(CONF_TOTAL_POWER): cv.use_id(sensor.Sensor),
        cv.Required(CONF_IMPORT_ENERGY): cv.use_id(sensor.Sensor),
        cv.Required(CONF_EXPORT_ENERGY): cv.use_id(sensor.Sensor),
        cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_PHASES): cv.All(cv.ensure_list(PHASE_SCHEMA), cv.Length(min=3, max=3)),
        cv.Optional(CONF_PUBLISH_INTERVAL, default="1s"): cv.All(
            cv.positive_time_period_milliseconds,
            cv.Range(min=cv.TimePeriod(seconds=1), max=cv.TimePeriod(seconds=60)),
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    include_builtin_idf_component("esp_http_server")
    broker = await cg.get_variable(config[CONF_MQTT_ID])
    var = cg.new_Pvariable(config[CONF_ID], broker)
    await cg.register_component(var, config)
    cg.add(var.set_total_power(await cg.get_variable(config[CONF_TOTAL_POWER])))
    cg.add(var.set_import_energy(await cg.get_variable(config[CONF_IMPORT_ENERGY])))
    cg.add(var.set_export_energy(await cg.get_variable(config[CONF_EXPORT_ENERGY])))
    cg.add(var.set_publish_interval(config[CONF_PUBLISH_INTERVAL].total_milliseconds))
    cg.add(var.set_clock(await cg.get_variable(config[CONF_TIME_ID])))
    for index, phase in enumerate(config[CONF_PHASES]):
        for key, method in (
            (CONF_VOLTAGE, "set_voltage"),
            (CONF_CURRENT, "set_current"),
            (CONF_POWER, "set_power"),
            (CONF_POWER_FACTOR, "set_power_factor"),
            (CONF_PHASE_IMPORT_ENERGY, "set_phase_import_energy"),
            (CONF_PHASE_EXPORT_ENERGY, "set_phase_export_energy"),
        ):
            cg.add(getattr(var, method)(index, await cg.get_variable(phase[key])))
