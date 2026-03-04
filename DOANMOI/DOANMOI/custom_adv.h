#ifndef _CUSTOM_ADV_H_
#define _CUSTOM_ADV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_bt_api.h"
#include "app_assert.h"
#include "app_log.h"

#define NAME_MAX_LENGTH 20

// Define for packet
#define FLAG                0x06
#define COMPANY_ID          0x02FF

/**
 * @brief Custom Advertisement Data Structure
 *
 * BLE Advertisement packet format (max 31 bytes):
 *
 * [Flags: 3 bytes]
 * - len_flags (1) | type_flags (1) | val_flags (1)
 *
 * [Manufacturer Specific Data: variable]
 * - len_manuf (1) | type_manuf (1) | company_LO (1) | company_HI (1)
 * - student_id (4 bytes) | hr_value (1) | spo2_value (1) | temp_int (1) | temp_frac (1)
 *
 * [Local Name: variable]
 * - len_name (1) | type_name (1) | name (up to 20 bytes)
 */
typedef struct
{
  // ========== FLAGS SECTION (3 bytes) ==========
  uint8_t len_flags;        // Flags length (always 0x02)
  uint8_t type_flags;       // Flags AD type (always 0x01)
  uint8_t val_flags;        // Flags value (e.g., 0x06)

  // ========== MANUFACTURER SPECIFIC DATA SECTION ==========
  uint8_t len_manuf;        // Manufacturer data length
  uint8_t type_manuf;       // Manufacturer data AD type (0xFF)

  // Company ID (little-endian)
  uint8_t company_LO;       // Company ID low byte
  uint8_t company_HI;       // Company ID high byte

  // Student ID (4 bytes, big-endian)
  uint8_t student_id_0;     // MSB
  uint8_t student_id_1;
  uint8_t student_id_2;
  uint8_t student_id_3;     // LSB

  // Sensor Data from MAX30102
  uint8_t hr_value;         // Heart rate (0-255 BPM)
  uint8_t spo2_value;       // SpO2 (0-100%)
  uint8_t temp_int;         // Temperature integer part
  uint8_t temp_frac;        // Temperature fractional part

  // ========== LOCAL NAME SECTION ==========
  uint8_t len_name;         // Name element length (including type)
  uint8_t type_name;        // Name AD type (0x08 or 0x09)
  char name[NAME_MAX_LENGTH]; // Device name

  // ========== METADATA ==========
  char dummy;               // Space for null terminator
  uint8_t data_size;        // Total advertisement data size (excluding this field)

} CustomAdv_t;

/**
 * @brief Fill BLE advertisement packet with sensor data
 *
 * @param pData Pointer to CustomAdv_t structure to be filled
 * @param flags Flag value (typically 0x06 for LE General Discoverable Mode)
 * @param companyID Company ID in little-endian format
 * @param student_id Student ID (32-bit)
 * @param hr Heart rate in BPM
 * @param spo2 SpO2 percentage
 * @param name Device name (null-terminated string, max 20 bytes)
 */
void fill_adv_packet(CustomAdv_t *pData, uint8_t flags, uint16_t companyID,
                     uint32_t student_id, uint8_t hr, uint8_t spo2,
                     char *name);

/**
 * @brief Start BLE advertisement with custom data
 *
 * @param pData Pointer to CustomAdv_t structure containing the data
 * @param advertising_set_handle Advertising set handle allocated by BLE stack
 */
void start_adv(CustomAdv_t *pData, uint8_t advertising_set_handle);

/**
 * @brief Update sensor values in advertisement data and resend
 *
 * @param pData Pointer to CustomAdv_t structure
 * @param advertising_set_handle Advertising set handle allocated by BLE stack
 * @param hr New heart rate value
 * @param spo2 New SpO2 value
 */
void update_adv_data(CustomAdv_t *pData, uint8_t advertising_set_handle,
                     uint8_t hr, uint8_t spo2);

#ifdef __cplusplus
}
#endif

#endif
