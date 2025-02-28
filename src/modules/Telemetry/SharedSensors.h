#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && !defined(ARCH_PORTDUINO) && SHARING_SENSORS
#if USE_SCD30
#include "modules/Telemetry/Sensor/SCD30Sensor.h"
extern SCD30Sensor scd30Sensor;
#endif
#endif
