#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#define DEBUG_MODE true

#define SENSORS_POWER_PIN 25
#define RTC_INT_PIN 4
#define MPU_INT_PIN 5

#define STOR_INI_ERR 0
#define READ_ERR 1
#define WRITE_ERR 2
#define APPEND_ERR 3
#define ERASE_ERR 4
#define OVERFLOW_ERR 5


static const uint8_t log_codes[] = {
  /*0-4*/  0x01, 0x02, 0x03, 0x04, 0x05, 
  /*5-10*/ 0x06, 0x07, 0x08, 0x09, 0x10 
};


struct SensorData {
    uint32_t id;
    uint32_t timestamp;
    float temperature;
    float humidity;
};

#endif

// struct feature{
// uint16_t size;
// uint32_t timestamp;
// float temperature;
// uint8_t data;
// };
