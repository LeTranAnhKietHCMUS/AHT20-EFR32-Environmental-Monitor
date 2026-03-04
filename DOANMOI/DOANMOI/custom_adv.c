#include <string.h>
#include "custom_adv.h"

/**
 * @brief Fill advertisement packet with sensor data
 */
void fill_adv_packet(CustomAdv_t *pData, uint8_t flags, uint16_t companyID,
                     uint32_t student_id, uint8_t hr, uint8_t spo2,
                     char *name)
{
  int n;

  // ========== FLAGS SETUP (3 bytes) ==========
  pData->len_flags = 0x02;
  pData->type_flags = 0x01;
  pData->val_flags = flags;

  // ========== MANUFACTURER SPECIFIC DATA SETUP ==========
  pData->len_manuf = 11;
  pData->type_manuf = 0xFF;

  // Company ID (little-endian)
  pData->company_LO = companyID & 0xFF;
  pData->company_HI = (companyID >> 8) & 0xFF;

  // Student ID (4 bytes, big-endian order)
  pData->student_id_0 = (student_id >> 24) & 0xFF;
  pData->student_id_1 = (student_id >> 16) & 0xFF;
  pData->student_id_2 = (student_id >> 8) & 0xFF;
  pData->student_id_3 = student_id & 0xFF;

  // Sensor values
  pData->hr_value = hr & 0xFF;
  pData->spo2_value = spo2 & 0x7F;
  pData->temp_int = 36;
  pData->temp_frac = 5;

  // ========== LOCAL NAME SETUP ==========
  n = strlen(name);

  if (n > NAME_MAX_LENGTH) {
    n = NAME_MAX_LENGTH;
    pData->type_name = 0x08;  // Shortened Local Name
    app_log("WARNING: Device name truncated to %d bytes\r\n", NAME_MAX_LENGTH);
  } else {
    pData->type_name = 0x09;  // Complete Local Name
  }

  strncpy(pData->name, name, NAME_MAX_LENGTH);
  pData->name[NAME_MAX_LENGTH - 1] = '\0';

  pData->len_name = 1 + n;

  // ========== TOTAL PACKET SIZE CALCULATION ==========
  pData->data_size = 3 + (1 + pData->len_manuf) + (1 + pData->len_name);

  // ========== LOGGING ==========
  app_log("ADV packet filled:\r\n");
  app_log("  Total size: %d bytes\r\n", pData->data_size);
  app_log("  Student ID: 0x%02X%02X%02X%02X\r\n",
          pData->student_id_0, pData->student_id_1,
          pData->student_id_2, pData->student_id_3);
  app_log("  HR: %d BPM\r\n", pData->hr_value);
  app_log("  SpO2: %d%%\r\n", pData->spo2_value);
  app_log("  Device Name: %s\r\n", pData->name);
}

/**
 * @brief Start BLE advertisement with custom data
 *
 * ✅ FIXED: Changed sl_bt_advertiser_advertising_data_type
 *           to sl_bt_advertiser_advertising_data_packet
 */
void start_adv(CustomAdv_t *pData, uint8_t advertising_set_handle)
{
  sl_status_t sc;

  // Validate data size
  if (pData->data_size > 31) {
    app_log("ERROR: Advertisement data exceeds 31 bytes (%d bytes)\r\n",
            pData->data_size);
    return;
  }

  // Set custom advertising payload
  // ✅ FIXED: Use sl_bt_advertiser_advertising_data_packet instead
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        pData->data_size,
                                        (const uint8_t *)pData);

  if (sc != SL_STATUS_OK) {
    app_log("ERROR: Failed to set advertising data (0x%04x)\r\n", (int)sc);
    return;
  }

  // Start advertising (connectable)
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_legacy_advertiser_connectable);

  if (sc != SL_STATUS_OK) {
    app_log("ERROR: Failed to start advertising (0x%04x)\r\n", (int)sc);
    return;
  }

  app_log("Advertisement started successfully\r\n");
}

/**
 * @brief Update sensor values in advertisement data
 *
 * ✅ FIXED: Changed sl_bt_advertiser_advertising_data_type
 *           to sl_bt_advertiser_advertising_data_packet
 */
void update_adv_data(CustomAdv_t *pData, uint8_t advertising_set_handle,
                     uint8_t hr, uint8_t spo2)
{
  sl_status_t sc;

  // Update sensor value fields
  pData->hr_value = hr & 0xFF;
  pData->spo2_value = spo2 & 0x7F;

  // Update the advertising data in the BLE stack
  // ✅ FIXED: Use sl_bt_advertiser_advertising_data_packet instead
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        pData->data_size,
                                        (const uint8_t *)pData);

  if (sc != SL_STATUS_OK) {
    app_log("ERROR: Failed to update advertising data (0x%04x)\r\n", (int)sc);
    return;
  }
}
