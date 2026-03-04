#ifndef MAX30102_H_
#define MAX30102_H_

#include <stdint.h>
#include <stdbool.h>
#include "sl_status.h"
#include "sl_i2cspm.h"

#define MAX30102_I2C_ADDR           0x57

// Register definitions
#define MAX30102_REG_INT_STATUS     0x00
#define MAX30102_REG_INT_ENABLE     0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_FIFO_OVF_CTR   0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_FIFO_CONFIG    0x08
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C  // LED1 (Red) Pulse Amplitude
#define MAX30102_REG_LED2_PA        0x0D  // LED2 (IR) Pulse Amplitude
#define MAX30102_REG_TEMP_INT       0x1F
#define MAX30102_REG_TEMP_FRAC      0x20
#define MAX30102_REG_REVISION_ID    0xFE
#define MAX30102_REG_PART_ID        0xFF

// Mode configuration values
#define MAX30102_MODE_HR_ONLY       0x02  // Heart Rate only (Red LED)
#define MAX30102_MODE_SPO2          0x03  // SpO2 mode (Red + IR LEDs)
#define MAX30102_MODE_MULTILED      0x07  // Multi-LED mode

// SPO2 configuration
#define MAX30102_SPO2_ADC_RES_15    0x00  // 15-bit resolution
#define MAX30102_SPO2_ADC_RES_16    0x01  // 16-bit resolution
#define MAX30102_SPO2_ADC_RES_17    0x02  // 17-bit resolution
#define MAX30102_SPO2_ADC_RES_18    0x03  // 18-bit resolution (best)

#define MAX30102_SPO2_SAMPLE_RATE_50   0x00   // 50 samples per second
#define MAX30102_SPO2_SAMPLE_RATE_100  0x01   // 100 samples per second
#define MAX30102_SPO2_SAMPLE_RATE_200  0x02   // 200 samples per second
#define MAX30102_SPO2_SAMPLE_RATE_400  0x03   // 400 samples per second

// LED current settings (mA)
#define MAX30102_LED_CURRENT_0      0x00
#define MAX30102_LED_CURRENT_5      0x01
#define MAX30102_LED_CURRENT_10     0x02
#define MAX30102_LED_CURRENT_50     0x0F
#define MAX30102_LED_CURRENT_100    0x1F

typedef struct {
  uint32_t red_led;   // Red LED ADC value
  uint32_t ir_led;    // IR LED ADC value
} max30102_sample_t;

typedef struct {
  uint32_t heart_rate;  // BPM (beats per minute) - 0 if no valid reading
  uint32_t spo2;        // SpO2 percentage (0-100) - 0 if no valid reading
  float temperature;    // Temperature in Celsius (not used)
} max30102_data_t;

// Function declarations
sl_status_t max30102_init(void);
sl_status_t max30102_reset(void);
sl_status_t max30102_set_mode(uint8_t mode);
sl_status_t max30102_set_spo2_config(uint8_t adc_res, uint8_t sample_rate);
sl_status_t max30102_set_led_current(uint8_t led1_current, uint8_t led2_current);
sl_status_t max30102_read_fifo(max30102_sample_t *samples, uint8_t *num_samples);
sl_status_t max30102_calculate_hr_spo2(max30102_data_t *data);
sl_status_t max30102_write_register(uint8_t reg, uint8_t value);
sl_status_t max30102_read_register(uint8_t reg, uint8_t *value);
sl_status_t max30102_read_registers(uint8_t reg, uint8_t *data, uint8_t len);

#endif /* MAX30102_H_ */
