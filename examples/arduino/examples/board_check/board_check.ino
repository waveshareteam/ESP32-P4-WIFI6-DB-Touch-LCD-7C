#include <Arduino.h>
#include <inttypes.h>

#include "esp_arduino_version.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

static void printChipFeatures(uint32_t features)
{
    Serial.print("SoC features:");
    Serial.print((features & CHIP_FEATURE_WIFI_BGN) ? " Wi-Fi" : "");
    Serial.print((features & CHIP_FEATURE_BT) ? " BT" : "");
    Serial.print((features & CHIP_FEATURE_BLE) ? " BLE" : "");
    Serial.print((features & CHIP_FEATURE_IEEE802154) ? " 802.15.4" : "");
    Serial.print((features & CHIP_FEATURE_EMB_FLASH) ? " embedded-flash" : "");
    Serial.print((features & CHIP_FEATURE_EMB_PSRAM) ? " embedded-psram" : "");
    Serial.println(features ? "" : " none reported");
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    Serial.println();
    Serial.println("============================================================");
    Serial.println(" Waveshare ESP32-P4-WIFI6-DB Touch LCD 10.1 Board Check");
    Serial.println("============================================================");
    Serial.printf("IDF target: %s\n", CONFIG_IDF_TARGET);
    Serial.printf("Arduino-ESP32 Core version: %s\n", ESP_ARDUINO_VERSION_STR);
    Serial.printf("ESP-IDF version: %s\n", ESP.getSdkVersion());
    Serial.printf("Chip model: %s\n", ESP.getChipModel());
    Serial.printf("Silicon revision: v%d.%d\n", chipInfo.revision / 100, chipInfo.revision % 100);
    Serial.printf("CPU cores: %d\n", chipInfo.cores);
    Serial.printf("CPU frequency: %" PRIu32 " MHz\n", ESP.getCpuFreqMHz());
    printChipFeatures(chipInfo.features);

    uint32_t flashSize = 0;
    if (esp_flash_get_size(nullptr, &flashSize) == ESP_OK) {
        Serial.printf("Flash size: %" PRIu32 " MiB\n", flashSize / (1024U * 1024U));
    } else {
        Serial.println("Flash size: read failed");
    }

    if (psramFound()) {
        Serial.println("PSRAM: initialized");
        Serial.printf("PSRAM heap size: %" PRIu32 " bytes\n", ESP.getPsramSize());
        Serial.printf("PSRAM free: %" PRIu32 " bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("PSRAM: not initialized or not enabled");
    }

    Serial.printf("Internal heap free: %" PRIu32 " bytes\n", ESP.getFreeHeap());
    Serial.printf("Minimum free internal heap: %" PRIu32 " bytes\n", ESP.getMinFreeHeap());
    Serial.println();
    Serial.println("Board check is running. Status updates follow every 5 seconds.");
}

void loop()
{
    delay(5000);
    Serial.printf("alive: free internal heap=%" PRIu32 " bytes, free PSRAM=%" PRIu32 " bytes\n", ESP.getFreeHeap(),
                  ESP.getFreePsram());
}
