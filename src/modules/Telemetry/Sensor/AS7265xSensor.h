
#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"
#include <SparkFun_AS7265X.h>

class AS7265XSensor : public TelemetrySensor
{
  private:
    AS7265X as7265x;
    float distance;
    bool use_bulbs;

  protected:
    virtual void setup() override;

  public:
    AS7265XSensor();
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
    bool getEnvironmentMetrics(meshtastic_Telemetry *measurement);
    bool getAirQualityMetrics(meshtastic_Telemetry *measurement);
    void setDistanceConfig(float distance);
    void setBulbConfig(bool use_bulb);
    float getVisible();
    float getIR();
    float getUV();
};

#endif
