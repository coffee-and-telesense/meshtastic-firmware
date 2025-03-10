#pragma once
#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR
#if SHARING_SENSORS

#if USE_SCD30
#include "SCD30Sensor.h"
extern SCD30Sensor scd30Sensor;
#endif

#if USE_SPS30
#include "SPS30Sensor.h"
extern SPS30Sensor sps30Sensor;
#endif

#endif
#endif