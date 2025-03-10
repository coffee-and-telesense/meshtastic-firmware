#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"
#include <sps30.h>


class SPS30Sensor : public TelemetrySensor
{
    private:
        SPS30 SPS30_instance;

    protected:
        virtual void setup() override;

    public:
        SPS30Sensor();
        virtual int32_t runOnce() override;
        virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
};

#endif