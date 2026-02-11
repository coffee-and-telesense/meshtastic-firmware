// #define RADIOLIB_CUSTOM_ARDUINO 1
// #define RADIOLIB_TONE_UNSUPPORTED 1
// #define RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED 1

#define ARDUINO_ARCH_AVR

// rename serial device based on region
#define SERIAL_RENAMING 1

#undef MESHTASTIC_MINIMIZE_BUILD

// Yes, we are using the SCD30 for both Air Quality and Environment Metrics
// #define SHARING_SENSORS 1
// #define SENSOR_COUNT 2
// #define USE_SCD30 1
// #define USE_PM25AQI 0

// Minimize the image's features
// #define MESHTASTIC_EXCLUDE_MODULES 0
// #define MESHTASTIC_EXCLUDE_WIFI 1
// #define MESHTASTIC_EXCLUDE_BLUETOOTH 1
// #define MESHTASTIC_EXCLUDE_GPS 0
// #define MESHTASTIC_EXCLUDE_SCREEN 1
// #define MESHTASTIC_EXCLUDE_MQTT 1
// #define MESHTASTIC_EXCLUDE_POWERMON 0
// #define MESHTASTIC_EXCLUDE_I2C 0
// #define MESHTASTIC_EXCLUDE_PKI 0
// #define MESHTASTIC_EXCLUDE_POWER_FSM 0
// #define MESHTASTIC_EXCLUDE_TZ 0

// Minimize the images modules
// #define MESHTASTIC_EXCLUDE_AUDIO 1
// #define MESHTASTIC_EXCLUDE_DETECTIONSENSOR 1
// #define MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR 0
// #define MESHTASTIC_EXCLUDE_HEALTH_TELEMETRY 0
// #define MESHTASTIC_EXCLUDE_EXTERNALNOTIFICATION 1
// #define MESHTASTIC_EXCLUDE_PAXCOUNTER 1
// #define MESHTASTIC_EXCLUDE_POWER_TELEMETRY 0
// Use Error Telemetry
#define MESHTASTIC_EXCLUDE_ERROR_TELEMETRY 0
// #define MESHTASTIC_EXCLUDE_RANGETEST 0
// #define MESHTASTIC_EXCLUDE_REMOTEHARDWARE 0
// #define MESHTASTIC_EXCLUDE_STOREFORWARD 1
// we still want to forward texts
// #define MESHTASTIC_EXCLUDE_TEXTMESSAGE 0
// #define MESHTASTIC_EXCLUDE_ATAK 1
// #define MESHTASTIC_EXCLUDE_CANNEDMESSAGES 1
// #define MESHTASTIC_EXCLUDE_NEIGHBORINFO 0
// #define MESHTASTIC_EXCLUDE_TRACEROUTE 0
// #define MESHTASTIC_EXCLUDE_WAYPOINT 1
// #define MESHTASTIC_EXCLUDE_INPUTBROKER 0
// used for sending messages from serial across mesh, do not need
// #define MESHTASTIC_EXCLUDE_SERIAL 0
// #define MESHTASTIC_EXCLUDE_POWERSTRESS 0
// #define MESHTASTIC_EXCLUDE_ADMIN 0

// #define USE_SSD1306

// #define USE_SH1106 1

// default I2C pins:
// SDA = 4
// SCL = 5

// Recommended pins for SerialModule:
// txd = 8
// rxd = 9

#define EXT_NOTIFY_OUT 22
#define BUTTON_PIN 7
// #define BUTTON_NEED_PULLUP

#define LED_PIN PIN_LED

// #define BATTERY_PIN 26
//  ratio of voltage divider = 3.0 (R17=200k, R18=100k)
// #define ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic

#define USE_RF95 // RFM95/SX127x

#undef LORA_SCK
#undef LORA_MISO
#undef LORA_MOSI
#undef LORA_CS

// https://www.adafruit.com/product/5714
// https://learn.adafruit.com/feather-rp2040-rfm95
// https://learn.adafruit.com/assets/120283
// https://learn.adafruit.com/assets/120813
#define LORA_SCK 14  // 10 12P
#define LORA_MISO 8  // 12 10P
#define LORA_MOSI 15 // 11 11P
#define LORA_CS 16   // 3 13P

#define LORA_RESET 17 // 15 14P

#define LORA_DIO0 21 // ?? 6P
#define LORA_DIO1 22 // 20 7P
#define LORA_DIO2 23 // 2 8P
#define LORA_DIO3 19 // ?? 3P
#define LORA_DIO4 20 // ?? 4P
#define LORA_DIO5 18 // ?? 15P

#ifdef USE_SX1262
#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH
// #define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif
