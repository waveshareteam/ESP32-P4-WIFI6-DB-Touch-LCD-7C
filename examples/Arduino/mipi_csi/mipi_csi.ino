#include <Arduino.h>
#include <ESP_Video.h>
#include <esp_rom_sys.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

static constexpr int lcd_width = 1024;
static constexpr int lcd_height = 600;
static constexpr size_t capture_buffer_count = 2;

static ESPVideoClass video;
static ESPVideoCaptureDevClass capture_device;
static esp_lcd_panel_handle_t lcd_panel = nullptr;
static bool camera_started = false;

struct LcdInitCommand {
  uint8_t command;
  uint8_t data;
  uint32_t delay_ms;
};

static bool check_esp_err(esp_err_t err, const char *message) {
  if (err == ESP_OK) {
    return true;
  }

  Serial.printf("%s: %s\n", message, esp_err_to_name(err));
  return false;
}

static bool start_lcd() {
  Serial.println("Initializing LCD backlight and reset");
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LCD_BL_OFF_LEVEL);

  pinMode(LCD_RST, OUTPUT);
  digitalWrite(LCD_RST, LCD_RST_ACTIVE_HIGH);
  delay(10);
  digitalWrite(LCD_RST, !LCD_RST_ACTIVE_HIGH);
  delay(20);

  esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
  Serial.println("Creating MIPI-DSI bus");
  esp_lcd_dsi_bus_config_t dsi_bus_config = {
    .bus_id = 0,
    .num_data_lanes = 2,
    .phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT,
    .lane_bit_rate_mbps = 1000,
  };
  if (!check_esp_err(esp_lcd_new_dsi_bus(&dsi_bus_config, &dsi_bus), "MIPI-DSI bus initialization failed")) {
    return false;
  }

  esp_lcd_panel_io_handle_t dbi_io = nullptr;
  Serial.println("Creating MIPI-DSI DBI IO");
  esp_lcd_dbi_io_config_t dbi_config = {
    .virtual_channel = 0,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
  };
  if (!check_esp_err(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &dbi_io), "MIPI-DSI DBI initialization failed")) {
    return false;
  }

  esp_lcd_dpi_panel_config_t dpi_config = {
    .virtual_channel = 0,
    .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
    .dpi_clock_freq_mhz = 52,
    .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
    .in_color_format = LCD_COLOR_FMT_RGB565,
    .out_color_format = LCD_COLOR_FMT_RGB565,
    .num_fbs = 1,
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
  };
  Serial.println("Creating MIPI-DSI DPI panel");
  if (!check_esp_err(esp_lcd_new_panel_dpi(dsi_bus, &dpi_config, &lcd_panel), "MIPI-DSI DPI initialization failed")) {
    return false;
  }

  static constexpr LcdInitCommand ek79007_init_commands[] = {
    {0xB2, 0x10, 0},
    {0x80, 0x8B, 0},
    {0x81, 0x78, 0},
    {0x82, 0x84, 0},
    {0x83, 0x88, 0},
    {0x84, 0xA8, 0},
    {0x85, 0xE3, 0},
    {0x86, 0x88, 0},
    {0x11, 0x00, 120},
  };
  Serial.println("Initializing EK79007");
  for (const auto &init_command : ek79007_init_commands) {
    const void *data = (init_command.command == 0x11) ? nullptr : &init_command.data;
    const size_t data_size = (data == nullptr) ? 0 : 1;
    if (!check_esp_err(esp_lcd_panel_io_tx_param(dbi_io, init_command.command, data, data_size), "EK79007 initialization failed")) {
      return false;
    }
    delay(init_command.delay_ms);
  }

  Serial.println("Starting MIPI-DSI panel");
  if (!check_esp_err(esp_lcd_panel_init(lcd_panel), "MIPI-DSI panel initialization failed")) {
    return false;
  }
  if (!check_esp_err(esp_lcd_panel_disp_on_off(lcd_panel, true), "MIPI-DSI panel enable failed")) {
    return false;
  }

  digitalWrite(LCD_BL, LCD_BL_ON_LEVEL);
  return true;
}

static bool start_camera() {
  Serial.println("Initializing OV5647 SCCB");
  ESPVideoCamConfigClass camera_config;
  if (!camera_config.begin(0, SCL, SDA)) {
    Serial.println("Camera SCCB initialization failed");
    return false;
  }

  ESPVideoCSIConfigClass csi_config;
  Serial.println("Configuring MIPI-CSI");
  if (!csi_config.begin(camera_config)) {
    Serial.println("MIPI-CSI configuration failed");
    return false;
  }
  Serial.println("Initializing OV5647");
  if (!video.begin(csi_config)) {
    Serial.println("OV5647 initialization failed");
    return false;
  }
  Serial.println("Opening camera capture device");
  if (!capture_device.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, capture_buffer_count)) {
    Serial.println("Camera capture device open failed");
    return false;
  }
  if (!capture_device.setFormat(ESP_VIDEO_FORMAT_RGB565)) {
    Serial.println("Camera RGB565 format configuration failed");
    return false;
  }
  if (!capture_device.startCapture()) {
    Serial.println("Camera capture start failed");
    return false;
  }

  return true;
}

void setup() {
  esp_rom_printf("Starting MIPI-CSI display test\n");
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial initialized");

  if (!start_lcd()) {
    return;
  }
  camera_started = start_camera();
  if (camera_started) {
    Serial.println("OV5647 direct display started");
  }
}

void loop() {
  if (!camera_started) {
    delay(1000);
    return;
  }

  ESPVideoBufferClass buffer = capture_device.captureBuffer();
  if (!buffer.valid()) {
    Serial.println("Camera capture failed");
    delay(10);
    return;
  }

  const int frame_width = static_cast<int>(buffer.getWidth());
  const int frame_height = static_cast<int>(buffer.getHeight());
  if ((frame_width <= 0) || (frame_height <= 0) || (frame_width > lcd_width)) {
    Serial.printf("Unsupported frame size: %d x %d\n", frame_width, frame_height);
    delay(1000);
    return;
  }

  const int draw_height = min(frame_height, lcd_height);
  const int source_y = (frame_height - draw_height) / 2;
  const int destination_x = (lcd_width - frame_width) / 2;
  const int destination_y = (lcd_height - draw_height) / 2;
  const uint8_t *frame_data = buffer.data() + source_y * frame_width * sizeof(uint16_t);

  if (!check_esp_err(
        esp_lcd_panel_draw_bitmap(lcd_panel, destination_x, destination_y, destination_x + frame_width, destination_y + draw_height, frame_data),
        "LCD frame drawing failed"
      )) {
    delay(10);
  }
}
