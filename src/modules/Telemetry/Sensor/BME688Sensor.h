#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"
#include <bme68xLibrary.h>

class BME688Sensor : public TelemetrySensor
{
    private:
    Bme68x BME688;

    protected:
    virtual void setup() override;

    public:
    BME688Sensor();
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;

};


#endif