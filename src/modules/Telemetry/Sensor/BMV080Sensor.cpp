#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "BMV080Sensor.h"
#include "TelemetrySensor.h"
#include <SparkFun_BMV080_Arduino_Library.h>
#include <typeinfo>

BMV080Sensor::BMV080Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_BMV080, "BMV080") {}

int32_t BMV080Sensor::runOnce()
{
    if (bmv080.begin(nodeTelemetrySensorsMap[sensorType].first, *nodeTelemetrySensorsMap[sensorType].second) == false) {
        LOG_ERROR("BMV080 not detected at I2C address");
    }
    LOG_INFO("Init sensor: %s", sensorName);
    bmv080.init();
    if (bmv080.setMode(SF_BMV080_MODE_CONTINUOUS) == true) {
        LOG_INFO("BMV080 set to continuous mode");
    } else {
        LOG_ERROR("Error setting BMV080 mode");
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
    measurement->variant.air_quality_metrics.has_pm10_standard = true;
    measurement->variant.air_quality_metrics.has_pm25_standard = true;
    measurement->variant.air_quality_metrics.has_pm100_standard = true;

    if (bmv080.readSensor()) {
        measurement->variant.air_quality_metrics.pm10_standard = bmv080.PM1();
        measurement->variant.air_quality_metrics.pm25_standard = bmv080.PM25();
        measurement->variant.air_quality_metrics.pm100_standard = bmv080.PM10();
    } else {
        LOG_ERROR("BMV080 data not ready");
        return false;
    }

    return true;
}
#endif
