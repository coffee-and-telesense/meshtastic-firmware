#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "SparkFun_BMV080_Arduino_Library.h"
#include "TelemetrySensor.h"
#include <typeinfo>

BMV080Sensor::BMV080Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_BMV080, "BMV080") {}

int32_t BMV080Sensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!bmv080.begin()) {
        bmv080.begin();
    }
    if (bmv080.setMode(SF_BMV080_MODE_CONTINUOUS) == true) {
        LOG_INFO !("BMV080 set to continuous mode");
    } else {
        LOG_ERROR !("Error setting BMV080 mode");
    }
    return initI2CSensor();
}

void BMV080Sensor::setup() {}

bool BMV080Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    LOG_DEBUG("BMV080 getMetrics");
    return getAirQualityMetrics(measurement);
}

bool BMV080Sensor::getAirQualityMetrics(meshtastic_Telemetry *measurement)
{
    measurement->variant.air_quality_metrics.has_pm25_standard = true;

    return true;
}
#endif
