#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "SCD30Sensor.h"
#include "TelemetrySensor.h"
#include <Adafruit_SCD30.h>
#include <typeinfo>

//TODO: fill out these functions, add I2C scanning for the specific port, (scani2ctwowire.cpp, scani2c.h), add to main.cpp so it autodetects

// SCD30Sensor::SCD30Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_SCD30, "SCD30") {
//   // do we we need to grab/instantiate stuff ro the airquality telemetry pointer?
// }

SCD30Sensor::SCD30Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_SCD30, "SCD30") {
  // do we we need to grab/instantiate stuff ro the airquality telemetry pointer?
}

// do the functions in here need to be run more than once or are we good
int32_t SCD30Sensor::runOnce()
{
  if(!scd30.begin())
  {
    scd30.begin();
  }
  scd30.setMeasurementInterval(10);
  scd30.startContinuousMeasurement();
  LOG_INFO("SCD30 setup:");
  LOG_INFO("%f, seconds",scd30.getMeasurementInterval());
  // return 1;
  return initI2CSensor();
}

void SCD30Sensor::setup() {
  //is scd30sensor fully instatntiated by the time we call this here
  // if(aq_telemetry_object)
  //   {
      // aq_telemetry_object->ShareSensor();
    // }
}

// // @brief: grabs scd30 ssensor and places it itnot  a 
// void SCD30Sensor::grabSensor(AirQualityTelemetryModule *telemetry_object) {

// }

bool SCD30Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
  measurement->variant.environment_metrics.has_temperature = true;
  measurement->variant.environment_metrics.has_relative_humidity = true;
  measurement->variant.air_quality_metrics.has_co2 = true;

  //is set_measurement_interval being set right at the right time
  if (!scd30.dataReady())
  {
      LOG_DEBUG("scd30 data not ready yet, delaying");
      delay(500);
  }
  if (!scd30.read()) {
      LOG_DEBUG("SCD30 read failed!");
      return false;
  }
  measurement->variant.environment_metrics.temperature = scd30.temperature;
  LOG_INFO("SCD30 Temperature: %0.2f degrees C", scd30.temperature);
  measurement->variant.environment_metrics.relative_humidity = scd30.relative_humidity;
  LOG_INFO("SCD30 Relative Humidity: %0.2f %", scd30.relative_humidity);
  measurement->variant.air_quality_metrics.co2 = scd30.CO2;
  LOG_INFO("SCD30 CO2: %0.2f ppm", scd30.CO2);
  return true;
}
#endif