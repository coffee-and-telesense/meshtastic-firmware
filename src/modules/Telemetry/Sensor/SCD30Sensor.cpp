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
  if(!scd30.begin())
  {
    scd30.begin();
  }
  LOG_INFO("begin() result: %d", status);
  scd30.setMeasurementInterval(10);
  scd30.startContinuousMeasurement();
  delay(1000); // Wait 2 seconds for initialization and calibration
  status = scd30.getMeasurementInterval();
  LOG_INFO("Measurement interval of %d seconds", status);
  return initI2CSensor();
}

void SCD30Sensor::setup() {
}

bool SCD30Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
  switch (measurement->which_variant) {
    case meshtastic_Telemetry_environment_metrics_tag:
        return getEnvironmentMetrics(measurement);

    case meshtastic_Telemetry_air_quality_metrics_tag:
        return getAirQualityMetrics(measurement);
    }

    // unsupported metric
    return false;
}

bool SCD30Sensor::getEnvironmentMetrics(meshtastic_Telemetry *measurement)
{
  measurement->variant.environment_metrics.has_temperature = true;
  measurement->variant.environment_metrics.has_relative_humidity = true;
  measurement->variant.environment_metrics.has_iaq = false;
  measurement->variant.environment_metrics.has_wind_speed = false;
  if (!scd30.dataReady())
  {
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
  measurement->variant.air_quality_metrics.has_particles_03um = false;
  measurement->variant.air_quality_metrics.has_particles_05um = false;
  measurement->variant.air_quality_metrics.has_particles_100um = false;
  measurement->variant.air_quality_metrics.has_particles_10um = false;
  measurement->variant.air_quality_metrics.has_particles_25um = false;
  measurement->variant.air_quality_metrics.has_particles_50um = false;
  measurement->variant.air_quality_metrics.has_pm100_environmental = false;
  measurement->variant.air_quality_metrics.has_pm100_standard = false;
  measurement->variant.air_quality_metrics.has_pm10_environmental = false;
  measurement->variant.air_quality_metrics.has_pm10_standard = false;
  measurement->variant.air_quality_metrics.has_pm25_environmental = false;
  measurement->variant.air_quality_metrics.has_pm25_standard = false;
  if (!scd30.dataReady())
  {
    LOG_DEBUG("scd30 data not ready yet, delaying");
    delay(2000);
    return false;
  }
  if (!scd30.read()) {
      LOG_DEBUG("SCD30 read failed!");
      delay(500);
      return false;
  }
  measurement->variant.air_quality_metrics.co2 = scd30.CO2;
  LOG_INFO("SCD30 CO2: %0.2f ppm", scd30.CO2);
  return true;
}
#endif