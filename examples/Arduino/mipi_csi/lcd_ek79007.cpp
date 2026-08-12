/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * The EK79007 vendor command values are based on Espressif's Apache-2.0
 * esp_lcd_ek79007 component and are kept here as project-owned board code.
 */

#include <Arduino.h>

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"

#include "lcd_ek79007.h"

namespace {

constexpr int lcd_width = 1024;
constexpr int lcd_height = 600;
constexpr int dsi_lane_count = 2;
constexpr float dsi_lane_rate_mbps = 1000.0f;
constexpr int dsi_phy_ldo_channel = 3;
constexpr int dsi_phy_ldo_voltage_mv = 2500;
constexpr int dpi_clock_mhz = 52;

struct lcd_init_command_t {
  uint8_t command;
  const uint8_t *data;
  size_t data_size;
  uint32_t delay_ms;
};

static const uint8_t ek79007_dsi_lane_data[] = {0x10};
static const uint8_t ek79007_cmd_80_data[] = {0x8B};
static const uint8_t ek79007_cmd_81_data[] = {0x78};
static const uint8_t ek79007_cmd_82_data[] = {0x84};
static const uint8_t ek79007_cmd_83_data[] = {0x88};
static const uint8_t ek79007_cmd_84_data[] = {0xA8};
static const uint8_t ek79007_cmd_85_data[] = {0xE3};
static const uint8_t ek79007_cmd_86_data[] = {0x88};

static const lcd_init_command_t ek79007_init_commands[] = {
  {0x80, ek79007_cmd_80_data, sizeof(ek79007_cmd_80_data), 0},
  {0x81, ek79007_cmd_81_data, sizeof(ek79007_cmd_81_data), 0},
  {0x82, ek79007_cmd_82_data, sizeof(ek79007_cmd_82_data), 0},
  {0x83, ek79007_cmd_83_data, sizeof(ek79007_cmd_83_data), 0},
  {0x84, ek79007_cmd_84_data, sizeof(ek79007_cmd_84_data), 0},
  {0x85, ek79007_cmd_85_data, sizeof(ek79007_cmd_85_data), 0},
  {0x86, ek79007_cmd_86_data, sizeof(ek79007_cmd_86_data), 0},
  {0x11, nullptr, 0, 120},
};

static esp_ldo_channel_handle_t dsi_phy_ldo = nullptr;
static esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
static esp_lcd_panel_io_handle_t dbi_io = nullptr;
static esp_lcd_panel_handle_t lcd_panel = nullptr;

static esp_err_t report_error(const char *operation, esp_err_t err)
{
  if (err != ESP_OK) {
    Serial.printf("%s: %s\n", operation, esp_err_to_name(err));
  }
  return err;
}

static void set_backlight(bool enabled)
{
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, enabled ? LCD_BL_ON_LEVEL : LCD_BL_OFF_LEVEL);
}

static void reset_lcd()
{
  pinMode(LCD_RST, OUTPUT);
  digitalWrite(LCD_RST, LCD_RST_ACTIVE_HIGH);
  delay(10);
  digitalWrite(LCD_RST, !LCD_RST_ACTIVE_HIGH);
  delay(20);
}

static void cleanup_lcd()
{
  set_backlight(false);

  if (lcd_panel != nullptr) {
    esp_lcd_panel_del(lcd_panel);
    lcd_panel = nullptr;
  }
  if (dbi_io != nullptr) {
    esp_lcd_panel_io_del(dbi_io);
    dbi_io = nullptr;
  }
  if (dsi_bus != nullptr) {
    esp_lcd_del_dsi_bus(dsi_bus);
    dsi_bus = nullptr;
  }
  if (dsi_phy_ldo != nullptr) {
    esp_ldo_release_channel(dsi_phy_ldo);
    dsi_phy_ldo = nullptr;
  }
}

static esp_err_t send_init_command(const lcd_init_command_t &init_command)
{
  esp_err_t err = esp_lcd_panel_io_tx_param(
    dbi_io,
    init_command.command,
    init_command.data,
    init_command.data_size
  );
  if (err != ESP_OK) {
    return report_error("LCD initialization command failed", err);
  }

  if (init_command.delay_ms != 0) {
    delay(init_command.delay_ms);
  }
  return ESP_OK;
}

static esp_err_t send_ek79007_init_commands()
{
  esp_err_t err = esp_lcd_panel_io_tx_param(
    dbi_io,
    0xB2,
    ek79007_dsi_lane_data,
    sizeof(ek79007_dsi_lane_data)
  );
  if (err != ESP_OK) {
    return report_error("EK79007 lane configuration failed", err);
  }

  for (const lcd_init_command_t &init_command : ek79007_init_commands) {
    err = send_init_command(init_command);
    if (err != ESP_OK) {
      return err;
    }
  }
  return ESP_OK;
}

}  // namespace

extern "C" esp_err_t lcd_ek79007_init(esp_lcd_panel_handle_t *ret_panel)
{
  if (ret_panel == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  *ret_panel = nullptr;

  set_backlight(false);

  const esp_ldo_channel_config_t ldo_config = {
    .chan_id = dsi_phy_ldo_channel,
    .voltage_mv = dsi_phy_ldo_voltage_mv,
  };
  esp_err_t err = esp_ldo_acquire_channel(&ldo_config, &dsi_phy_ldo);
  if (err != ESP_OK) {
    return report_error("MIPI-DSI PHY LDO initialization failed", err);
  }

  const esp_lcd_dsi_bus_config_t dsi_bus_config = {
    .bus_id = 0,
    .num_data_lanes = dsi_lane_count,
    .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT,
    .lane_bit_rate_mbps = dsi_lane_rate_mbps,
  };
  err = esp_lcd_new_dsi_bus(&dsi_bus_config, &dsi_bus);
  if (err != ESP_OK) {
    cleanup_lcd();
    return report_error("MIPI-DSI bus initialization failed", err);
  }

  const esp_lcd_dbi_io_config_t dbi_config = {
    .virtual_channel = 0,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
  };
  err = esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &dbi_io);
  if (err != ESP_OK) {
    cleanup_lcd();
    return report_error("MIPI-DSI DBI initialization failed", err);
  }

  const esp_lcd_dpi_panel_config_t dpi_config = {
    .virtual_channel = 0,
    .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
    .dpi_clock_freq_mhz = dpi_clock_mhz,
    .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
    .in_color_format = LCD_COLOR_FMT_RGB565,
    .out_color_format = LCD_COLOR_FMT_RGB565,
    .num_fbs = 2,
    .video_timing = {
      .h_size = lcd_width,
      .v_size = lcd_height,
      .hsync_pulse_width = 10,
      .hsync_back_porch = 160,
      .hsync_front_porch = 160,
      .vsync_pulse_width = 1,
      .vsync_back_porch = 23,
      .vsync_front_porch = 12,
    },
    .flags = {
      .use_dma2d = 1,
    },
  };
  err = esp_lcd_new_panel_dpi(dsi_bus, &dpi_config, &lcd_panel);
  if (err != ESP_OK) {
    cleanup_lcd();
    return report_error("MIPI-DSI DPI panel initialization failed", err);
  }

  reset_lcd();
  err = send_ek79007_init_commands();
  if (err != ESP_OK) {
    cleanup_lcd();
    return err;
  }

  err = esp_lcd_panel_init(lcd_panel);
  if (err != ESP_OK) {
    cleanup_lcd();
    return report_error("MIPI-DSI panel initialization failed", err);
  }

  set_backlight(true);
  *ret_panel = lcd_panel;
  return ESP_OK;
}
