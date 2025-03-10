#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "SPS30Sensor.h"
#include "TelemetrySensor.h"
#include <sps30.h>
#include <typeinfo>

SPS30Sensor::SPS30Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_SPS30, "SPS30") {}

void SPS30Sensor::setup() {}

int32_t SPS30Sensor::runOnce() 
{
    //todo FINISH
    //this is where all the setup should happen
    LOG_INFO("attempting to init sensor: %s", sensorName);

    //i wonder if it'll get an i2c interface/hope it does
    if(!SPS30_instance.begin()) {
        SPS30_instance.begin();
    }
    
    //check fof sps connection
    if (!SPS30_instance.probe()) {
        LOG_ERROR("coudln't set i2c comms channel?");
    } else
    {
        LOG_DEBUG("detected sps30");
    }


    // call measurment setters
    // set delays if inecessary

    return initI2CSensor();
}


bool SPS30Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    LOG_DEBUG("SPS30 getMetrics");

}

#endif