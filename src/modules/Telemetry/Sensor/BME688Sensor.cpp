#include "configuration.h"

#if !MESHTASTIC_EXLUCDE_ENVIRONMENTAL_SENSOR

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "BME688Sensor.h"
#include "TelemetrySensor.h"
#include <bme68xLibrary.h>
#include <typeinfo>
#define NEW_GAS_MEAS (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK | BME68X_NEW_DATA_MSK)


BME688Sensor::BME688Sensor() : TelemetrySensor(meshtastic_TelemetrySensorType_BME688, "BME688") {}

int32_t BME688Sensor::runOnce()
{
    BME688.begin(nodeTelemetrySensorsMap[sensorType].first, *nodeTelemetrySensorsMap[sensorType].second);    
    
    if(BME688.checkStatus())
    {
        if(BME688.checkStatus() == BME68X_ERROR)
        {
            LOG_ERROR("Sensor error: %s", BME688.statusString().c_str());
        }
        else if(BME688.checkStatus() == BME68X_WARNING)
        {
            LOG_ERROR("Sensor warning: %s", BME688.statusString().c_str());
        }
    }

    /* Set the default configuration for temperature, pressure and humidity */
    BME688.setTPH();
	/* Heater temperature in degree Celsius */
	uint16_t tempProf[10] = { 100, 200, 320 };
	/* Heating duration in milliseconds */
	uint16_t durProf[10] = { 150, 150, 150 }    ;

    BME688.setSeqSleep(BME68X_ODR_250_MS);
	BME688.setHeaterProf(tempProf, durProf, 3);
	BME688.setOpMode(BME68X_SEQUENTIAL_MODE); 

    LOG_INFO("TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%), Gas resistance(ohm), Status, Gas index");


    return initI2CSensor(); //check if this is canonical
}

void BME688Sensor::setup() {
    LOG_INFO("Init sensor: %s", sensorName);
}

bool BME688Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    bme68xData data;
    uint8_t nFieldsLeft = 0;

    //populate telemetry structure
    LOG_DEBUG("BME688 getMetrics");


    measurement->variant.environment_metrics.has_temperature = true;
    measurement->variant.environment_metrics.has_relative_humidity = true;
    measurement->variant.environment_metrics.has_barometric_pressure = true;
    measurement->variant.environment_metrics.has_gas_resistance = true;
    measurement->variant.environment_metrics.has_iaq = true;

    //getalldata?
    if(BME688.fetchData())
    {
        do
        {
            nFieldsLeft = BME688.getData(data);
            if (data.status == NEW_GAS_MEAS)
            {
                LOG_DEBUG("temp: %f", data.temperature);
                LOG_DEBUG("pressure: %f", data.pressure);
                LOG_DEBUG("humidity: %f", data.humidity);
                LOG_DEBUG("gas resist: %f", data.gas_resistance);

            }

        } while (nFieldsLeft);
        
    }


    return true;
}


#endif