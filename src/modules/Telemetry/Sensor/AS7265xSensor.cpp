#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "AS7265xSensor.h"
#include "TelemetrySensor.h"
#include <SparkFun_AS7265X.h>
#include <typeinfo>

AS7265XSensor::AS7265XSensor() : TelemetrySensor(meshtastic_TelemetrySensorType_AS7265X, "AS7265X") {}

int32_t AS7265XSensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!as7265x.begin(*nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_AS7265X].second)) {
        this->status = 1;
    } else {
        this->status = 0;
    }

    // Wait 1000ms for startup
    delay(1000);

    if (as7265x.isConnected()) {
        LOG_INFO("AS7265X connected");
        this->status = 1;
    } else {
        LOG_DEBUG("AS7265X could not connect");
        this->status = 0;
    }

    // Disable the indicator light
    as7265x.disableIndicator();

    byte deviceType = as7265x.getDeviceType();
    byte hwVersion = as7265x.getHardwareVersion();
    byte majorFwVersion = as7265x.getMajorFirmwareVersion();
    byte patchFwVersion = as7265x.getPatchFirmwareVersion();
    byte buildFwVersion = as7265x.getBuildFirmwareVersion();
    LOG_DEBUG("AS7265X hw version: 0x%x%x", deviceType, hwVersion);
    LOG_DEBUG("AS7265X fw version: 0x%x.0x%x.0x%x", majorFwVersion, patchFwVersion, buildFwVersion);

    // Set a default distance of 1 meter
    distance = 1.0f;

    // Set to not use bulbs by default
    use_bulbs = false;

    return initI2CSensor();
}

void AS7265XSensor::setup() {}

bool AS7265XSensor::getMetrics(meshtastic_Telemetry *measurement)
{
    LOG_DEBUG("AS7265X getMetrics");
    measurement->variant.environment_metrics.has_lux = true;
    measurement->variant.environment_metrics.has_ir_lux = true;
    measurement->variant.environment_metrics.has_uv_lux = true;

    if (use_bulbs)
        as7265x.takeMeasurementsWithBulb();
    else
        as7265x.takeMeasurements();

    while (!as7265x.dataAvailable()) {
        LOG_DEBUG("AS7265X data not ready yet, delaying");
        delay(AS7265X_POLLING_DELAY);
    }

    measurement->variant.environment_metrics.lux = this->getVisible();
    measurement->variant.environment_metrics.ir_lux = this->getIR();
    measurement->variant.environment_metrics.uv_lux = this->getUV();
    return true;
}

void AS7265XSensor::setDistanceConfig(float distance)
{
    // this probably needs module configs and more
    this->distance = distance;
}

void AS7265XSensor::setBulbConfig(bool use_bulb)
{
    this->use_bulbs = use_bulb;
}

// Unsure about these calculations, doubt their right tbh
// None agree with phone app Lux, but that is a phone measurement lmao
//
// https://forums.adafruit.com/viewtopic.php?t=173878
// https://github.com/apairoftimbs/smart-lighting/issues/5
// https://github.com/kriswiner/AS7265X/issues/5
// https://en.wikipedia.org/wiki/Lumen_(unit)
// https://www.translatorscafe.com/unit-converter/en-US/illumination/11-1/watt/centimeter%C2%B2%20(at%20555%20nm)-lux/
// https://cdn.sparkfun.com/assets/c/2/9/0/a/AS7265x_Datasheet.pdf

// Raw visible light lux
float AS7265XSensor::getVisible()
{
    float avg_vis = 0.0f;
    avg_vis += as7265x.getCalibratedG();
    avg_vis += as7265x.getCalibratedH();
    avg_vis += as7265x.getCalibratedI();
    avg_vis += as7265x.getCalibratedJ();
    avg_vis += as7265x.getCalibratedK();
    avg_vis += as7265x.getCalibratedL();
    avg_vis = avg_vis / 6.0f;
    return (avg_vis / (distance * distance));
}

// Raw IR light lux
float AS7265XSensor::getIR()
{
    float avg_vis = 0.0f;
    avg_vis += as7265x.getCalibratedR();
    avg_vis += as7265x.getCalibratedS();
    avg_vis += as7265x.getCalibratedT();
    avg_vis += as7265x.getCalibratedU();
    avg_vis += as7265x.getCalibratedV();
    avg_vis += as7265x.getCalibratedW();
    avg_vis = avg_vis / 6.0f;
    return (avg_vis / (distance * distance));
}

// Raw UV light lux
float AS7265XSensor::getUV()
{
    float avg_vis = 0.0f;
    avg_vis += as7265x.getCalibratedA();
    avg_vis += as7265x.getCalibratedB();
    avg_vis += as7265x.getCalibratedC();
    avg_vis += as7265x.getCalibratedD();
    avg_vis += as7265x.getCalibratedE();
    avg_vis += as7265x.getCalibratedF();
    avg_vis = avg_vis / 6.0f;
    return (avg_vis / (distance * distance));
}

#endif
