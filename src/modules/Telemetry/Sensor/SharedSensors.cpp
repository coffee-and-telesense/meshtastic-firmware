#include "SharedSensors.h"
#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR
#if SHARING_SENSORS

#if USE_SCD30
SCD30Sensor scd30Sensor;
#endif

#endif
#endif