#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "SCD30Sensor.h"
#include "TelemetrySensor.h"
#include <Adafruit_SCD30.h>
#include <typeinfo>

SCD30Sensor::SCD30Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_SCD30, "SCD30") {}

int32_t SCD30Sensor::runOnce()
{
    LOG_INFO("SCD30 setup:");
    if (!scd30.begin()) {
        scd30.begin();
    }
    LOG_INFO("begin() result: %d", status);
    scd30.setMeasurementInterval(2);
    scd30.startContinuousMeasurement();
    delay(2000); // Wait 2 seconds for initialization and calibration
    status = scd30.getMeasurementInterval();
    LOG_INFO("Measurement interval of %d seconds", status);
    // set scd30 co2 to -1 as hacky init
    scd30.CO2 = INFINITY;
    return initI2CSensor();
}

void SCD30Sensor::setup() {}

bool SCD30Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    switch (measurement->which_variant) {
    case meshtastic_Telemetry_environment_metrics_tag:
        LOG_INFO("Environment Telemetry getMetrics() request");
        return getEnvironmentMetrics(measurement);

    case meshtastic_Telemetry_air_quality_metrics_tag:
        LOG_INFO("Air Quality Telemetry getMetrics() request");
        return getAirQualityMetrics(measurement);
    }

    // unsupported metric
    return false;
}

bool SCD30Sensor::getEnvironmentMetrics(meshtastic_Telemetry *measurement)
{
    measurement->variant.environment_metrics.has_temperature = true;
    measurement->variant.environment_metrics.has_relative_humidity = true;
    if (!scd30.dataReady()) {
        LOG_DEBUG("scd30 data not ready yet, delaying");
        delay(2000);
        return false;
    }
    if (!scd30.read()) {
        LOG_DEBUG("SCD30 read failed!");
        delay(500);
        return false;
    }
    measurement->variant.environment_metrics.temperature = scd30.temperature;
    LOG_INFO("SCD30 Temperature: %0.2f degrees C", scd30.temperature);
    measurement->variant.environment_metrics.relative_humidity = scd30.relative_humidity;
    LOG_INFO("SCD30 Relative Humidity: %0.2f%%", scd30.relative_humidity);
    return true;
}

bool SCD30Sensor::getAirQualityMetrics(meshtastic_Telemetry *measurement)
{
    measurement->variant.air_quality_metrics.has_co2 = true;
    if (!scd30.dataReady()) {
        LOG_DEBUG("scd30 data not ready yet, delaying");
        delay(2000);
        return false;
    }
    if (!scd30.read()) {
        LOG_DEBUG("SCD30 read failed!");
        delay(500);
        return false;
    }
    if (scd30.CO2 == INFINITY) {
        LOG_DEBUG("SCD30 reporting infinity ppm, probably not ready");
        delay(500);
        return false;
    }
    measurement->variant.air_quality_metrics.co2 = scd30.CO2;
    LOG_INFO("SCD30 CO2: %0.2f ppm", scd30.CO2);
    return true;
}
#endif
