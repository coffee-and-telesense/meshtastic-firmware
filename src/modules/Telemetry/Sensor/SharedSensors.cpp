#include "SharedSensors.h"
#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR
#if SHARING_SENSORS

#if USE_SCD30
SCD30Sensor scd30Sensor;
#endif

#if USE_SPS30
SPS30Sensor sps30Sensor;
#endif

#endif
#endif