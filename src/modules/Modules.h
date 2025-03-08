#pragma once

/**
 * Create module instances here.  If you are adding a new module, you must 'new' it here (or somewhere else)
 */
void setupModules();

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ERROR_TELEMETRY
#include "modules/Telemetry/ErrorTelemetry.h"
extern ErrorTelemetryModule *errorTelemetryModule;
#endif
