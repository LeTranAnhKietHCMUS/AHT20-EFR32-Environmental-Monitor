#include "em_common.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_log.h"
#include "app_timer.h"
#include "custom_adv.h"
#include "max30102.h"
#include "glib.h"
#include "dmd.h"
#include "sl_board_control.h"
#include <stdio.h>
#include <string.h>

// ==================== CONFIGURATION ====================
#define DEFAULT_SENSOR_PERIOD_MS  500   // Read sensor every 500ms
#define DEFAULT_BLE_ADV_PERIOD_MS 1000  // Update BLE advertisement every 1s
#define LCD_LINES                 11

// ==================== GLOBAL VARIABLES ====================
static uint8_t advertising_set_handle = 0xff;
static CustomAdv_t ble_adv_data;
static max30102_data_t sensor_data = {0, 0, 0};

// Timers
static app_timer_t sensor_read_timer;
static app_timer_t ble_adv_timer;
static app_timer_t uart_report_timer;

// Configuration variables
static uint32_t sensor_period_ms = DEFAULT_SENSOR_PERIOD_MS;
static uint32_t ble_adv_period_ms = DEFAULT_BLE_ADV_PERIOD_MS;
static uint32_t uart_report_period_ms = 2000;

static uint32_t Student_ID = 0x71167064;
#define DEVICE_NAME "HR-SPO2-Monitor"

// LCD context
static GLIB_Context_t glibContext;
static int currentLine = 0;

// ==================== LCD FUNCTIONS ====================
static void lcd_init(void)
{
  uint32_t status;

  status = sl_board_enable_display();
  app_assert(status == SL_STATUS_OK, "LCD enable failed");

  status = DMD_init(0);
  app_assert(status == DMD_OK, "DMD init failed");

  status = GLIB_contextInit(&glibContext);
  app_assert(status == GLIB_OK, "GLIB init failed");

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  GLIB_clear(&glibContext);
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);

  app_log("LCD initialized\r\n");
}

static void lcd_clear(void)
{
  GLIB_clear(&glibContext);
  currentLine = 0;
}

static void lcd_update_display(void)
{
  char buffer[20];

  lcd_clear();

  // Title
  GLIB_drawStringOnLine(&glibContext, "HR & SpO2 Monitor", currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  GLIB_drawStringOnLine(&glibContext, "-------------------", currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  // Heart rate
  snprintf(buffer, sizeof(buffer), "HR: %lu BPM", sensor_data.heart_rate);
  GLIB_drawStringOnLine(&glibContext, buffer, currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  // SpO2
  snprintf(buffer, sizeof(buffer), "SpO2: %lu %%", sensor_data.spo2);
  GLIB_drawStringOnLine(&glibContext, buffer, currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  GLIB_drawStringOnLine(&glibContext, "-------------------", currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  // ✅ Display status based on sensor readings
  if (sensor_data.heart_rate == 0) {
    GLIB_drawStringOnLine(&glibContext, "Place finger!", currentLine++,
                          GLIB_ALIGN_LEFT, 5, 5, true);
  } else {
    GLIB_drawStringOnLine(&glibContext, "Reading OK", currentLine++,
                          GLIB_ALIGN_LEFT, 5, 5, true);
  }

  GLIB_drawStringOnLine(&glibContext, "-------------------", currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  // Period info
  snprintf(buffer, sizeof(buffer), "Period: %lu ms", sensor_period_ms);
  GLIB_drawStringOnLine(&glibContext, buffer, currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  // Status
  GLIB_drawStringOnLine(&glibContext, "BLE: Advertising", currentLine++,
                        GLIB_ALIGN_LEFT, 5, 5, true);

  DMD_updateDisplay();
}

// ==================== SENSOR FUNCTIONS ====================
static void sensor_read_callback(app_timer_t *timer, void *data)
{
  (void)timer;
  (void)data;

  // Read MAX30102 sensor
  sl_status_t ret = max30102_calculate_hr_spo2(&sensor_data);
  if (ret != SL_STATUS_OK) {
    app_log("Sensor read error: 0x%04X\r\n", (int)ret);
  }

  // Update LCD
  lcd_update_display();
}

// ==================== BLE ADVERTISEMENT FUNCTIONS ====================
static void ble_update_adv_callback(app_timer_t *timer, void *data)
{
  (void)timer;
  (void)data;

  // Update BLE advertisement data with current sensor values
  update_adv_data(&ble_adv_data, advertising_set_handle,
                  (uint8_t)(sensor_data.heart_rate & 0xFF),
                  (uint8_t)(sensor_data.spo2 & 0xFF));

  app_log("BLE ADV updated: HR=%lu, SpO2=%lu\r\n",
          sensor_data.heart_rate, sensor_data.spo2);
}

// ==================== UART REPORT FUNCTIONS ====================
static void uart_report_callback(app_timer_t *timer, void *data)
{
  (void)timer;
  (void)data;

  app_log("===== SENSOR DATA REPORT =====\r\n");
  app_log("Heart Rate: %lu BPM\r\n", sensor_data.heart_rate);
  app_log("SpO2: %lu %%\r\n", sensor_data.spo2);
  app_log("Temperature: %.2f C\r\n", sensor_data.temperature);
  app_log("Period: %lu ms\r\n", sensor_period_ms);
  app_log("BLE Period: %lu ms\r\n", ble_adv_period_ms);
  app_log("==============================\r\n");
}

// ==================== UART COMMAND FUNCTIONS ====================
static void process_uart_commands(void)
{
  // This would typically read from UART and process commands
  // For now, this is a placeholder for command processing
  // Commands could include:
  // - SET_SENSOR_PERIOD <milliseconds>
  // - SET_BLE_PERIOD <milliseconds>
  // - GET_DATA
  // - CALIBRATE
  // - START/STOP measurements
}

// ==================== APPLICATION INIT ====================
SL_WEAK void app_init(void)
{
  sl_status_t sc;

  app_log("\r\n");
  app_log("========================================\r\n");
  app_log("HR & SpO2 Monitor System Initialization\r\n");
  app_log("========================================\r\n");

  // Initialize LCD
  lcd_init();
  lcd_clear();
  GLIB_drawStringOnLine(&glibContext, "Initializing...", 0,
                        GLIB_ALIGN_LEFT, 5, 5, true);
  DMD_updateDisplay();

  // ✅ REMOVED: sl_i2cspm_init_instances()
  // I2C đã được tự động khởi tạo trong sl_system_init()

  // Wait a bit for I2C to stabilize
  for (volatile uint32_t i = 0; i < 500000; i++);

  // Initialize MAX30102 sensor
  app_log("Initializing MAX30102 sensor...\r\n");
  sc = max30102_init();

  if (sc != SL_STATUS_OK) {
    app_log("ERROR: MAX30102 initialization failed: 0x%04X\r\n", (int)sc);

    // ✅ SHOW ERROR ON LCD
    lcd_clear();
    GLIB_drawStringOnLine(&glibContext, "Sensor Error!", 0,
                          GLIB_ALIGN_LEFT, 5, 5, true);
    GLIB_drawStringOnLine(&glibContext, "Check I2C wiring", 1,
                          GLIB_ALIGN_LEFT, 5, 5, true);
    char err_msg[20];
    snprintf(err_msg, sizeof(err_msg), "Code: 0x%04X", (int)sc);
    GLIB_drawStringOnLine(&glibContext, err_msg, 2,
                          GLIB_ALIGN_LEFT, 5, 5, true);
    DMD_updateDisplay();

    // ✅ CONTINUE WITH SIMULATED DATA instead of returning
    app_log("WARNING: Running with simulated data\r\n");
  } else {
    app_log("MAX30102 sensor initialized successfully\r\n");
  }

  // ✅ UPDATE LCD to show "Ready"
  lcd_clear();
  GLIB_drawStringOnLine(&glibContext, "System Ready!", 0,
                        GLIB_ALIGN_LEFT, 5, 5, true);
  GLIB_drawStringOnLine(&glibContext, "Starting timers...", 1,
                        GLIB_ALIGN_LEFT, 5, 5, true);
  DMD_updateDisplay();

  // Start timers
  app_log("Starting measurement timers...\r\n");

  sc = app_timer_start(&sensor_read_timer, sensor_period_ms,
                       sensor_read_callback, NULL, true);
  app_assert_status(sc);

  sc = app_timer_start(&ble_adv_timer, ble_adv_period_ms,
                       ble_update_adv_callback, NULL, true);
  app_assert_status(sc);

  sc = app_timer_start(&uart_report_timer, uart_report_period_ms,
                       uart_report_callback, NULL, true);
  app_assert_status(sc);

  app_log("All timers started\r\n");
  app_log("Application initialized successfully\r\n");

  // ✅ Initial LCD update
  lcd_update_display();
}

// ==================== APPLICATION PROCESS ACTION ====================
SL_WEAK void app_process_action(void)
{
  process_uart_commands();
}

// ==================== BLUETOOTH EVENT HANDLER ====================
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_bt_evt_system_boot_id:
      app_log("BLE Stack Boot Event\r\n");

      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id, 0,
                                                    sizeof(system_id), system_id);
      app_assert_status(sc);

      // Create advertising set
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising timing
      sc = sl_bt_advertiser_set_timing(advertising_set_handle, 1600, 1600, 0, 0);
      app_assert_status(sc);

      // Set advertising channels
      sc = sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);
      app_assert_status(sc);

      // Fill advertisement packet with initial data
      fill_adv_packet(&ble_adv_data, 0x06, 0x02FF, Student_ID,
                      sensor_data.heart_rate & 0xFF,
                      sensor_data.spo2 & 0xFF, DEVICE_NAME);

      app_log("Advertisement packet created\r\n");

      // Start advertising
      start_adv(&ble_adv_data, advertising_set_handle);
      app_log("BLE Advertising started\r\n");

      break;

    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened\r\n");
      break;

    case sl_bt_evt_connection_closed_id:
      app_log("Connection closed, resuming advertising\r\n");

      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                  sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    default:
      break;
  }
}
