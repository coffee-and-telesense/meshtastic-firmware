#include "configuration.h"

#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "TelemetrySensor.h"
// #include "../src/modules/Telemetry/AirQualityTelemetry.h"
#include <Adafruit_SCD30.h>

class SCD30Sensor : public TelemetrySensor
{
  private:
    Adafruit_SCD30 scd30 = Adafruit_SCD30();
    // AirQualityTelemetryModule *aq_telemetry_object; //does it get instatntiated or something or passed

  protected:
    virtual void setup() override;
    // void grabSensor(AirQualityTelemetryModule *telemetry_object);

  public:
    SCD30Sensor();
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
    bool getSensorData();
};

#endif