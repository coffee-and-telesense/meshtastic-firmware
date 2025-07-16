#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"
#include <SparkFun_BMV080_Arduino_Library.h>

class BMV080Sensor : public TelemetrySensor
{
  private:
    SparkFunBMV080 bmv080;

  protected:
    virtual void setup() override;

  public:
    BMV080Sensor();
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
    bool getAirQualityMetrics(meshtastic_Telemetry *measurement);
};

#endif
