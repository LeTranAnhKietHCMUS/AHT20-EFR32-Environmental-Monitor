#include "max30102.h"
#include "app_log.h"
#include "sl_i2cspm_instances.h"
#include <math.h>

// Circular buffers for raw data
#define BUFFER_SIZE 100  // Increased to 100 samples (1 second at 100Hz)
static int32_t ir_buffer[BUFFER_SIZE];
static int32_t red_buffer[BUFFER_SIZE];
static uint8_t buffer_idx = 0;
static bool buffer_filled = false;

// Peak detection
#define MAX_PEAKS 20
static uint16_t peak_indices[MAX_PEAKS];
static uint8_t peak_count = 0;

/**
 * @brief Write a byte to MAX30102 register
 */
sl_status_t max30102_write_register(uint8_t reg, uint8_t value)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;
  uint8_t data[2] = {reg, value};

  seq.addr = MAX30102_I2C_ADDR << 1;
  seq.flags = I2C_FLAG_WRITE;
  seq.buf[0].data = data;
  seq.buf[0].len = 2;

  ret = I2CSPM_Transfer(sl_i2cspm_I2C0, &seq);

  if (ret != i2cTransferDone) {
    app_log("I2C Write Error: %d\r\n", ret);
    return SL_STATUS_TRANSMIT;
  }

  return SL_STATUS_OK;
}

/**
 * @brief Read a byte from MAX30102 register
 */
sl_status_t max30102_read_register(uint8_t reg, uint8_t *value)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;

  seq.addr = MAX30102_I2C_ADDR << 1;
  seq.flags = I2C_FLAG_WRITE_READ;
  seq.buf[0].data = &reg;
  seq.buf[0].len = 1;
  seq.buf[1].data = value;
  seq.buf[1].len = 1;

  ret = I2CSPM_Transfer(sl_i2cspm_I2C0, &seq);

  if (ret != i2cTransferDone) {
    app_log("I2C Read Error: %d\r\n", ret);
    return SL_STATUS_TRANSMIT;
  }
  
  return SL_STATUS_OK;
}

/**
 * @brief Read multiple bytes from MAX30102 registers
 */
sl_status_t max30102_read_registers(uint8_t reg, uint8_t *data, uint8_t len)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;

  seq.addr = MAX30102_I2C_ADDR << 1;
  seq.flags = I2C_FLAG_WRITE_READ;
  seq.buf[0].data = &reg;
  seq.buf[0].len = 1;
  seq.buf[1].data = data;
  seq.buf[1].len = len;

  ret = I2CSPM_Transfer(sl_i2cspm_I2C0, &seq);

  if (ret != i2cTransferDone) {
    app_log("I2C Read Registers Error: %d\r\n", ret);
    return SL_STATUS_TRANSMIT;
  }
  
  return SL_STATUS_OK;
}

/**
 * @brief Initialize MAX30102 sensor
 */
sl_status_t max30102_init(void)
{
  sl_status_t ret;
  uint8_t id, revision;

  app_log("MAX30102: Starting initialization...\r\n");

  // Check I2C communication
  ret = max30102_read_register(MAX30102_REG_PART_ID, &id);
  if (ret != SL_STATUS_OK) {
    app_log("MAX30102: Failed to read part ID (I2C Error)\r\n");
    return ret;
  }

  app_log("MAX30102: Part ID = 0x%02X\r\n", id);

  if (id != 0x15) {
    app_log("MAX30102: Invalid part ID (expected 0x15)\r\n");
    return SL_STATUS_FAIL;
  }

  // Read revision
  ret = max30102_read_register(MAX30102_REG_REVISION_ID, &revision);
  if (ret == SL_STATUS_OK) {
    app_log("MAX30102: Revision ID = 0x%02X\r\n", revision);
  }

  // Reset sensor
  app_log("MAX30102: Resetting sensor...\r\n");
  ret = max30102_write_register(MAX30102_REG_MODE_CONFIG, 0x40);
  if (ret != SL_STATUS_OK) return ret;

  // Wait for reset (50ms)
  for (volatile int i = 0; i < 500000; i++);

  // FIFO Configuration: Sample averaging = 4, rollover enabled
  // 0b01010000 = 0x50
  app_log("MAX30102: Configuring FIFO...\r\n");
  ret = max30102_write_register(MAX30102_REG_FIFO_CONFIG, 0x50);
  if (ret != SL_STATUS_OK) return ret;

  // SpO2 Configuration:
  // ADC Resolution = 18-bit (0b11)
  // Sample Rate = 100 Hz (0b001)
  // LED Pulse Width = 411us (0b11)
  // 0b01100111 = 0x67
  app_log("MAX30102: Configuring SpO2...\r\n");
  ret = max30102_write_register(MAX30102_REG_SPO2_CONFIG, 0x67);
  if (ret != SL_STATUS_OK) return ret;

  // LED Configuration: Increased brightness for better signal
  // RED LED = 0x24 (7.0mA) - good starting point
  app_log("MAX30102: Configuring LEDs...\r\n");
  ret = max30102_write_register(MAX30102_REG_LED1_PA, 0x24);
  if (ret != SL_STATUS_OK) return ret;

  // IR LED = 0x24 (7.0mA)
  ret = max30102_write_register(MAX30102_REG_LED2_PA, 0x24);
  if (ret != SL_STATUS_OK) return ret;

  // Mode Configuration: SpO2 mode (Red + IR LEDs)
  app_log("MAX30102: Setting SpO2 mode...\r\n");
  ret = max30102_write_register(MAX30102_REG_MODE_CONFIG, 0x03);
  if (ret != SL_STATUS_OK) return ret;

  // Clear FIFO pointers
  app_log("MAX30102: Clearing FIFO...\r\n");
  ret = max30102_write_register(MAX30102_REG_FIFO_WR_PTR, 0x00);
  if (ret != SL_STATUS_OK) return ret;

  ret = max30102_write_register(MAX30102_REG_FIFO_RD_PTR, 0x00);
  if (ret != SL_STATUS_OK) return ret;

  ret = max30102_write_register(MAX30102_REG_FIFO_OVF_CTR, 0x00);
  if (ret != SL_STATUS_OK) return ret;

  // Reset buffer
  buffer_idx = 0;
  buffer_filled = false;

  app_log("MAX30102: Init completed successfully\r\n");
  return SL_STATUS_OK;
}

sl_status_t max30102_reset(void)
{
  return max30102_write_register(MAX30102_REG_MODE_CONFIG, 0x40);
}

sl_status_t max30102_set_mode(uint8_t mode)
{
  return max30102_write_register(MAX30102_REG_MODE_CONFIG, mode);
}

sl_status_t max30102_set_spo2_config(uint8_t adc_res, uint8_t sample_rate)
{
  uint8_t config = (adc_res << 5) | (sample_rate << 2) | 0x03; // 411us pulse width
  return max30102_write_register(MAX30102_REG_SPO2_CONFIG, config);
}

sl_status_t max30102_set_led_current(uint8_t led1_current, uint8_t led2_current)
{
  sl_status_t ret;
  ret = max30102_write_register(MAX30102_REG_LED1_PA, led1_current);
  if (ret != SL_STATUS_OK) return ret;
  return max30102_write_register(MAX30102_REG_LED2_PA, led2_current);
}

/**
 * @brief Read FIFO data
 */
sl_status_t max30102_read_fifo(max30102_sample_t *samples, uint8_t *num_samples)
{
  sl_status_t ret;
  uint8_t wr_ptr, rd_ptr;
  uint8_t fifo_data[6];

  ret = max30102_read_register(MAX30102_REG_FIFO_WR_PTR, &wr_ptr);
  if (ret != SL_STATUS_OK) return ret;

  ret = max30102_read_register(MAX30102_REG_FIFO_RD_PTR, &rd_ptr);
  if (ret != SL_STATUS_OK) return ret;

  // Calculate available samples
  uint8_t num_available;
  if (wr_ptr >= rd_ptr) {
    num_available = wr_ptr - rd_ptr;
  } else {
    num_available = 32 + wr_ptr - rd_ptr;
  }

  // Limit to max 15 samples per read
  if (num_available > 15) num_available = 15;
  *num_samples = num_available;

  if (num_available == 0) {
    return SL_STATUS_EMPTY;
  }

  for (uint8_t i = 0; i < num_available; i++) {
    ret = max30102_read_registers(MAX30102_REG_FIFO_DATA, fifo_data, 6);
    if (ret != SL_STATUS_OK) return ret;

    // RED LED (first 3 bytes)
    samples[i].red_led = ((uint32_t)fifo_data[0] << 16) |
                        ((uint32_t)fifo_data[1] << 8) |
                        fifo_data[2];
    samples[i].red_led &= 0x03FFFF; // 18-bit mask

    // IR LED (next 3 bytes)
    samples[i].ir_led = ((uint32_t)fifo_data[3] << 16) |
                       ((uint32_t)fifo_data[4] << 8) |
                       fifo_data[5];
    samples[i].ir_led &= 0x03FFFF; // 18-bit mask
  }

  return SL_STATUS_OK;
}

/**
 * @brief Simple moving average filter
 */
static int32_t moving_average(int32_t *data, uint8_t start, uint8_t window)
{
  int64_t sum = 0;
  for (uint8_t i = 0; i < window; i++) {
    uint8_t idx = (start + i) % BUFFER_SIZE;
    sum += data[idx];
  }
  return (int32_t)(sum / window);
}

/**
 * @brief Find peaks in IR signal using adaptive threshold
 */
static uint8_t find_peaks_adaptive(int32_t *data, uint8_t size, uint16_t *peaks, uint8_t max_peaks)
{
  uint8_t peak_count = 0;

  // Calculate mean and standard deviation
  int64_t sum = 0;
  for (uint8_t i = 0; i < size; i++) {
    sum += data[i];
  }
  int32_t mean = (int32_t)(sum / size);

  int64_t variance_sum = 0;
  for (uint8_t i = 0; i < size; i++) {
    int32_t diff = data[i] - mean;
    variance_sum += (int64_t)diff * diff;
  }
  int32_t std_dev = (int32_t)sqrt((double)variance_sum / size);

  // Adaptive threshold: mean + 0.5 * std_dev
  int32_t threshold = mean + (std_dev / 2);

  app_log("Peak Detection: mean=%ld, std=%ld, threshold=%ld\r\n",
          mean, std_dev, threshold);

  // Find peaks
  for (uint8_t i = 5; i < size - 5; i++) {
    if (data[i] > threshold &&
        data[i] > data[i-1] && data[i] > data[i-2] &&
        data[i] > data[i+1] && data[i] > data[i+2] &&
        data[i] > data[i-3] && data[i] > data[i+3]) {

      // Avoid too close peaks (minimum 30 samples = 0.3s at 100Hz)
      if (peak_count == 0 || (i - peaks[peak_count-1]) > 30) {
        peaks[peak_count++] = i;
        if (peak_count >= max_peaks) break;
      }
    }
  }

  app_log("Found %d peaks\r\n", peak_count);
  return peak_count;
}

/**
 * @brief Calculate heart rate and SpO2 - IMPROVED ALGORITHM
 */
sl_status_t max30102_calculate_hr_spo2(max30102_data_t *data)
{
  sl_status_t ret;
  max30102_sample_t samples[15];
  uint8_t num_samples = 0;

  ret = max30102_read_fifo(samples, &num_samples);

  if (ret == SL_STATUS_EMPTY) {
    // No new samples
    return SL_STATUS_OK;
  }

  if (ret != SL_STATUS_OK) {
    app_log("FIFO read error: 0x%04X\r\n", (int)ret);
    return ret;
  }

  // Add samples to circular buffer
  for (uint8_t i = 0; i < num_samples; i++) {
    ir_buffer[buffer_idx] = samples[i].ir_led;
    red_buffer[buffer_idx] = samples[i].red_led;

    // Debug first few samples
    if (buffer_idx < 5) {
      app_log("Sample[%d]: IR=%ld, RED=%ld\r\n",
              buffer_idx, samples[i].ir_led, samples[i].red_led);
    }

    buffer_idx++;
    if (buffer_idx >= BUFFER_SIZE) {
      buffer_idx = 0;
      buffer_filled = true;
    }
  }

  // Need full buffer for calculation
  if (!buffer_filled) {
    app_log("Filling buffer... %d/%d\r\n", buffer_idx, BUFFER_SIZE);
    data->heart_rate = 0;
    data->spo2 = 0;
    return SL_STATUS_OK;
  }

  // ========== FINGER DETECTION ==========
  // Calculate DC average
  int64_t ir_sum = 0;
  int64_t red_sum = 0;
  for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
    ir_sum += ir_buffer[i];
    red_sum += red_buffer[i];
  }
  int32_t ir_dc = (int32_t)(ir_sum / BUFFER_SIZE);
  int32_t red_dc = (int32_t)(red_sum / BUFFER_SIZE);

  app_log("DC values: IR=%ld, RED=%ld\r\n", ir_dc, red_dc);

  // Check if finger present (IR signal should be > 50000 for 18-bit ADC)
  if (ir_dc < 50000) {
    app_log("⚠️ No finger detected! (IR_DC=%ld < 50000)\r\n", ir_dc);
    data->heart_rate = 0;
    data->spo2 = 0;
    return SL_STATUS_OK;
  }

  // ========== HEART RATE CALCULATION ==========
  peak_count = find_peaks_adaptive(ir_buffer, BUFFER_SIZE, peak_indices, MAX_PEAKS);

  if (peak_count >= 2) {
    // Calculate average interval between peaks
    uint32_t total_interval = 0;
    for (uint8_t i = 1; i < peak_count; i++) {
      total_interval += (peak_indices[i] - peak_indices[i-1]);
    }
    uint32_t avg_interval = total_interval / (peak_count - 1);

    // Convert to BPM: (samples/beat) at 100Hz -> 60 / (interval/100) = 6000/interval
    uint32_t bpm = 6000 / avg_interval;

    app_log("Avg interval=%lu samples, BPM=%lu\r\n", avg_interval, bpm);

    // Validate heart rate range
    if (bpm >= 40 && bpm <= 200) {
      data->heart_rate = bpm;
      app_log("✅ Valid HR: %lu BPM\r\n", bpm);
    } else {
      app_log("⚠️ Invalid HR: %lu BPM (out of range)\r\n", bpm);
    }
  } else {
    app_log("⚠️ Not enough peaks (%d) for HR calculation\r\n", peak_count);
  }

  // ========== SPO2 CALCULATION ==========
  // Find AC component (max - min)
  int32_t ir_max = ir_buffer[0];
  int32_t ir_min = ir_buffer[0];
  int32_t red_max = red_buffer[0];
  int32_t red_min = red_buffer[0];

  for (uint8_t i = 1; i < BUFFER_SIZE; i++) {
    if (ir_buffer[i] > ir_max) ir_max = ir_buffer[i];
    if (ir_buffer[i] < ir_min) ir_min = ir_buffer[i];
    if (red_buffer[i] > red_max) red_max = red_buffer[i];
    if (red_buffer[i] < red_min) red_min = red_buffer[i];
  }

  int32_t ir_ac = ir_max - ir_min;
  int32_t red_ac = red_max - red_min;

  app_log("AC values: IR=%ld, RED=%ld\r\n", ir_ac, red_ac);

  // Check if AC signals are strong enough
  if (ir_ac > 100 && red_ac > 100 && ir_dc > 0 && red_dc > 0) {
    // R = (AC_red/DC_red) / (AC_ir/DC_ir)
    float red_ratio = (float)red_ac / (float)red_dc;
    float ir_ratio = (float)ir_ac / (float)ir_dc;
    float r = red_ratio / ir_ratio;

    // SpO2 = 104 - 17*R (empirical formula for MAX30102)
    // Alternative: SpO2 = -45.060*R*R + 30.354*R + 94.845
    float spo2_f = 104.0f - 17.0f * r;
    int32_t spo2 = (int32_t)spo2_f;

    app_log("R=%.4f, SpO2=%.1f%%\r\n", r, spo2_f);

    // Validate SpO2 range
    if (spo2 >= 70 && spo2 <= 100) {
      data->spo2 = (uint32_t)spo2;
      app_log("✅ Valid SpO2: %ld%%\r\n", spo2);
    } else {
      app_log("⚠️ Invalid SpO2: %ld%% (out of range)\r\n", spo2);
    }
  } else {
    app_log("⚠️ AC signal too weak for SpO2 calculation\r\n");
  }

  return SL_STATUS_OK;
}
