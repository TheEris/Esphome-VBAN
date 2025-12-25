import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import microphone
from esphome.const import CONF_ID

DEPENDENCIES = ["microphone"]

vban_audio_ns = cg.esphome_ns.namespace("vban_audio")
VBANAudio = vban_audio_ns.class_("VBANAudio", cg.Component)

CONF_MICROPHONE = "microphone"
CONF_IP_ADDRESS = "ip_address"
CONF_PORT = "port"
CONF_STREAM_NAME = "stream_name"
CONF_SAMPLE_RATE = "sample_rate"
CONF_GAIN = "gain"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(VBANAudio),

        cv.Required(CONF_MICROPHONE):
            cv.use_id(microphone.Microphone),

        cv.Required(CONF_IP_ADDRESS):
            cv.ipv4address,

        cv.Optional(CONF_PORT, default=6980):
            cv.port,

        cv.Optional(CONF_STREAM_NAME, default="ESP32"):
            cv.string,

        cv.Optional(CONF_SAMPLE_RATE, default=48000):
            cv.int_range(min=6000, max=192000),

        cv.Optional(CONF_GAIN, default=1.0):
            cv.float_range(min=0.0, max=10.0),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    mic = await cg.get_variable(config[CONF_MICROPHONE])
    cg.add(var.set_microphone(mic))

    cg.add(var.set_target_ip(str(config[CONF_IP_ADDRESS])))
    cg.add(var.set_target_port(config[CONF_PORT]))
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    cg.add(var.set_stream_name(config[CONF_STREAM_NAME]))
    cg.add(var.set_gain(config[CONF_GAIN]))
