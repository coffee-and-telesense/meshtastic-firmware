#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "sps30sensor.h"
#include "TelemetrySensor.h"
#include <sps30.h>

SPS30Sensor::SPS30Sensor() : TelemetrySensor(meshtastic_TelemetrySensorTYpe_SPS30, "SPS30") {}

int32_t SPS30Sensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!hasSensor()) {
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }
    //set autoclean
    //

    //* User must have preform the wirePort.begin() in the sketch.
    SPS30_obj.begin(Wire);

    return initI2CSensor();

}

void SPS30Sensor::setup()
{

}

bool SPS30Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    //protobuff instatntaition

    SPS30_obj.start();
    //delay
    struct sps_values ret_val;
    uint8_t err = SPS30_obj.GetValues(&ret_val);

    //GetValuess get called iun erghere
    //

    //scani2cbus, we know what to expect 

}
#endif