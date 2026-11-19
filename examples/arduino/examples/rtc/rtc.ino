#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <Wire.h>

static constexpr uint8_t rtc_i2c_address = 0x51;
static constexpr uint8_t rtc_seconds_register = 0x04;
static constexpr uint8_t rtc_datetime_size = 7;
static constexpr uint32_t rtc_i2c_frequency = 100000;

// Set this to true once to write the firmware build time to the RTC.
static constexpr bool rtc_set_time_on_boot = false;

struct rtc_datetime_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

static uint8_t bcd_to_decimal(uint8_t value)
{
    return static_cast<uint8_t>((value >> 4) * 10 + (value & 0x0F));
}

static uint8_t decimal_to_bcd(uint8_t value)
{
    return static_cast<uint8_t>((value / 10) << 4 | (value % 10));
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static constexpr uint8_t days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month == 0 || month > 12) {
        return 0;
    }
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        return 29;
    }
    return days[month - 1];
}

static bool is_valid_datetime(const rtc_datetime_t &datetime)
{
    return datetime.year >= 2000 && datetime.year <= 2099 &&
           datetime.month >= 1 && datetime.month <= 12 &&
           datetime.day >= 1 && datetime.day <= days_in_month(datetime.year, datetime.month) &&
           datetime.weekday <= 6 && datetime.hour <= 23 &&
           datetime.minute <= 59 && datetime.second <= 59;
}

static void print_datetime(const char *label, const rtc_datetime_t &datetime)
{
    Serial.printf(
        "%s: %04u-%02u-%02u weekday=%u %02u:%02u:%02u\n",
        label,
        static_cast<unsigned>(datetime.year),
        static_cast<unsigned>(datetime.month),
        static_cast<unsigned>(datetime.day),
        static_cast<unsigned>(datetime.weekday),
        static_cast<unsigned>(datetime.hour),
        static_cast<unsigned>(datetime.minute),
        static_cast<unsigned>(datetime.second)
    );
}

static bool read_datetime(rtc_datetime_t &datetime, bool &oscillator_stopped)
{
    Wire.beginTransmission(rtc_i2c_address);
    Wire.write(rtc_seconds_register);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(rtc_i2c_address, rtc_datetime_size, true) != rtc_datetime_size) {
        return false;
    }

    const uint8_t seconds = Wire.read();
    datetime.second = bcd_to_decimal(seconds & 0x7F);
    datetime.minute = bcd_to_decimal(Wire.read() & 0x7F);
    datetime.hour = bcd_to_decimal(Wire.read() & 0x3F);
    datetime.day = bcd_to_decimal(Wire.read() & 0x3F);
    datetime.weekday = Wire.read() & 0x07;
    datetime.month = bcd_to_decimal(Wire.read() & 0x1F);
    datetime.year = static_cast<uint16_t>(2000 + bcd_to_decimal(Wire.read()));
    oscillator_stopped = (seconds & 0x80) != 0;

    return is_valid_datetime(datetime) && !oscillator_stopped;
}

static bool write_datetime(const rtc_datetime_t &datetime)
{
    if (!is_valid_datetime(datetime)) {
        return false;
    }

    Wire.beginTransmission(rtc_i2c_address);
    Wire.write(rtc_seconds_register);
    Wire.write(decimal_to_bcd(datetime.second));
    Wire.write(decimal_to_bcd(datetime.minute));
    Wire.write(decimal_to_bcd(datetime.hour));
    Wire.write(decimal_to_bcd(datetime.day));
    Wire.write(datetime.weekday & 0x07);
    Wire.write(decimal_to_bcd(datetime.month));
    Wire.write(decimal_to_bcd(static_cast<uint8_t>(datetime.year - 2000)));

    return Wire.endTransmission(true) == 0;
}

static uint8_t month_from_text(const char *month_text)
{
    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *month_position = strstr(months, month_text);
    if (month_position == nullptr) {
        return 0;
    }
    return static_cast<uint8_t>((month_position - months) / 3 + 1);
}

static uint8_t calculate_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    static constexpr uint8_t days_before_month[] = {
        0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4
    };
    const uint16_t adjusted_year = year - (month < 3 ? 1 : 0);
    return static_cast<uint8_t>(
        (adjusted_year + adjusted_year / 4 - adjusted_year / 100 +
         adjusted_year / 400 + days_before_month[month - 1] + day) % 7
    );
}

static rtc_datetime_t build_datetime(void)
{
    char month_text[4] = {0};
    rtc_datetime_t datetime = {};
    int day = 0;
    int year = 0;

    sscanf(__DATE__, "%3s %d %d", month_text, &day, &year);
    sscanf(__TIME__, "%hhu:%hhu:%hhu", &datetime.hour, &datetime.minute, &datetime.second);

    datetime.year = static_cast<uint16_t>(year);
    datetime.month = month_from_text(month_text);
    datetime.day = static_cast<uint8_t>(day);
    datetime.weekday = calculate_weekday(datetime.year, datetime.month, datetime.day);
    return datetime;
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("PCF85063A RTC Arduino example");
    Serial.printf("I2C SDA: GPIO%u, SCL: GPIO%u, address: 0x%02X\n", SDA, SCL, rtc_i2c_address);

    Wire.begin(SDA, SCL);
    Wire.setClock(rtc_i2c_frequency);

    if (rtc_set_time_on_boot) {
        const rtc_datetime_t build_time = build_datetime();
        if (write_datetime(build_time)) {
            Serial.println("RTC time was set from firmware build time");
        } else {
            Serial.println("Failed to set RTC time");
        }
    }

    rtc_datetime_t datetime = {};
    bool oscillator_stopped = false;
    if (!read_datetime(datetime, oscillator_stopped)) {
        if (oscillator_stopped) {
            Serial.println("RTC oscillator stopped; writing build time");
        } else {
            Serial.println("RTC time is invalid or PCF85063A is unavailable");
        }

        datetime = build_datetime();
        if (!write_datetime(datetime)) {
            Serial.println("Failed to initialize RTC time");
            return;
        }
    }

    print_datetime("RTC time", datetime);
}

void loop()
{
    rtc_datetime_t datetime = {};
    bool oscillator_stopped = false;
    if (read_datetime(datetime, oscillator_stopped)) {
        print_datetime("RTC time", datetime);
    } else if (oscillator_stopped) {
        Serial.println("RTC oscillator stopped");
    } else {
        Serial.println("Failed to read PCF85063A");
    }
    delay(1000);
}
