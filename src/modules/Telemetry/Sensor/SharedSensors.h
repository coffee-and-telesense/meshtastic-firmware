#pragma once
#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR
#if SHARING_SENSORS

#if USE_SCD30
#include "SCD30Sensor.h"
extern SCD30Sensor scd30Sensor;
#endif

#if USE_BMV080
#include "BMV080Sensor.h"
extern BMV080Sensor bmv080Sensor;
#endif

#endif
#endif