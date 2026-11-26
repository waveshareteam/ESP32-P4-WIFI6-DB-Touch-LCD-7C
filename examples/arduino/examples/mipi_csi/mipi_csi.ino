#include "lcd_ek79007.h"
#include <Arduino.h>
#include <ESP_Video.h>
#include <cstring>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_ops.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>


static constexpr int lcd_width = 1024;
static constexpr int lcd_height = 600;
static constexpr size_t lcd_bytes_per_pixel = sizeof(uint16_t);
static constexpr size_t display_buffer_count = 2;
static constexpr size_t capture_buffer_count = 2;

static ESPVideoClass video;
static ESPVideoCaptureDevClass capture_device;
static esp_lcd_panel_handle_t lcd_panel = nullptr;
static void *display_buffers[display_buffer_count] = {};
static size_t display_buffer_index = 0;
static SemaphoreHandle_t display_refresh_semaphore = nullptr;
static volatile uint32_t display_refresh_count = 0;
static bool camera_started = false;

static bool check_esp_err(esp_err_t err, const char *message)
{
    if (err == ESP_OK) {
        return true;
    }

    Serial.printf("%s: %s\n", message, esp_err_to_name(err));
    return false;
}

static bool IRAM_ATTR lcd_frame_complete_callback(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t *, void *)
{
    BaseType_t high_task_woken = pdFALSE;
    display_refresh_count = display_refresh_count + 1u;
    xSemaphoreGiveFromISR(display_refresh_semaphore, &high_task_woken);
    return high_task_woken == pdTRUE;
}

static bool start_lcd()
{
    Serial.println("Initializing project EK79007 LCD driver");
    if (!check_esp_err(lcd_ek79007_init(&lcd_panel), "Project LCD initialization failed")) {
        return false;
    }

    if (!check_esp_err(esp_lcd_dpi_panel_get_frame_buffer(lcd_panel, display_buffer_count, &display_buffers[0], &display_buffers[1]), "LCD frame buffer query failed")) {
        return false;
    }

    display_refresh_semaphore = xSemaphoreCreateBinary();
    if (display_refresh_semaphore == nullptr) {
        Serial.println("LCD refresh semaphore creation failed");
        return false;
    }

    esp_lcd_dpi_panel_event_callbacks_t display_callbacks = {};
    display_callbacks.on_frame_buf_complete = lcd_frame_complete_callback;
    if (!check_esp_err(esp_lcd_dpi_panel_register_event_callbacks(lcd_panel, &display_callbacks, nullptr), "LCD refresh callback registration failed")) {
        return false;
    }

    display_buffer_index = 0;
    return true;
}

static constexpr int camera_sccb_sda_pin = 7;
static constexpr int camera_sccb_scl_pin = 8;

static bool start_camera()
{
    Serial.println("Initializing OV5647 SCCB");
    ESPVideoCamConfigClass camera_config;
    if (!camera_config.begin(0, camera_sccb_scl_pin, camera_sccb_sda_pin)) {
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

static bool copy_frame_to_display_buffer(const ESPVideoBufferClass &buffer, void *display_buffer)
{
    const int frame_width = static_cast<int>(buffer.getWidth());
    const int frame_height = static_cast<int>(buffer.getHeight());
    if ((frame_width <= 0) || (frame_height <= 0) || (frame_width > lcd_width)) {
        Serial.printf("Unsupported frame size: %d x %d\n", frame_width, frame_height);
        return false;
    }

    const int draw_height = min(frame_height, lcd_height);
    const int source_y = (frame_height - draw_height) / 2;
    const int destination_x = (lcd_width - frame_width) / 2;
    const int destination_y = (lcd_height - draw_height) / 2;
    const size_t source_row_size = static_cast<size_t>(frame_width) * lcd_bytes_per_pixel;
    const size_t source_offset = static_cast<size_t>(source_y) * source_row_size;
    const size_t required_buffer_size = source_offset + static_cast<size_t>(draw_height) * source_row_size;
    if (buffer.size() < required_buffer_size) {
        Serial.printf("Camera frame is truncated: %lu < %lu bytes\n", static_cast<unsigned long>(buffer.size()), static_cast<unsigned long>(required_buffer_size));
        return false;
    }

    uint8_t *destination_buffer = static_cast<uint8_t *>(display_buffer);
    const size_t destination_row_size = static_cast<size_t>(lcd_width) * lcd_bytes_per_pixel;
    if (destination_y > 0) {
        memset(destination_buffer, 0, static_cast<size_t>(destination_y) * destination_row_size);
    }
    if (destination_y + draw_height < lcd_height) {
        memset(destination_buffer + static_cast<size_t>(destination_y + draw_height) * destination_row_size, 0, static_cast<size_t>(lcd_height - destination_y - draw_height) * destination_row_size);
    }

    const uint8_t *source_buffer = buffer.data() + source_offset;
    for (int row = 0; row < draw_height; ++row) {
        uint8_t *destination_row = destination_buffer + static_cast<size_t>(destination_y + row) * destination_row_size;
        if (destination_x > 0) {
            memset(destination_row, 0, static_cast<size_t>(destination_x) * lcd_bytes_per_pixel);
        }
        destination_row += static_cast<size_t>(destination_x) * lcd_bytes_per_pixel;
        memcpy(destination_row, source_buffer + static_cast<size_t>(row) * source_row_size, source_row_size);
        if (destination_x + frame_width < lcd_width) {
            memset(destination_row + source_row_size, 0, static_cast<size_t>(lcd_width - destination_x - frame_width) * lcd_bytes_per_pixel);
        }
    }

    return true;
}

static bool display_camera_frame(const ESPVideoBufferClass &buffer)
{
    const size_t next_buffer_index = (display_buffer_index + 1) % display_buffer_count;
    if (!copy_frame_to_display_buffer(buffer, display_buffers[next_buffer_index])) {
        return false;
    }

    if (!check_esp_err(esp_lcd_panel_draw_bitmap(lcd_panel, 0, 0, lcd_width, lcd_height, display_buffers[next_buffer_index]), "LCD frame switch failed")) {
        return false;
    }

    display_buffer_index = next_buffer_index;
    return true;
}

static bool wait_for_next_display_refresh()
{
    const uint32_t refresh_count_snapshot = display_refresh_count;
    while (display_refresh_count == refresh_count_snapshot) {
        if (xSemaphoreTake(display_refresh_semaphore, portMAX_DELAY) != pdTRUE) {
            return false;
        }
    }
    return true;
}

void setup()
{
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

void loop()
{
    if (!camera_started) {
        delay(1000);
        return;
    }

    if (!wait_for_next_display_refresh()) {
        Serial.println("LCD refresh synchronization failed");
        delay(10);
        return;
    }

    ESPVideoBufferClass buffer = capture_device.captureBuffer();
    if (!buffer.valid()) {
        Serial.println("Camera capture failed");
        delay(10);
        return;
    }

    if (!display_camera_frame(buffer)) {
        delay(10);
    }
}
