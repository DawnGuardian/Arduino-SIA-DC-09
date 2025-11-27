#if !defined SIA_DC_09_UTL_HPP
#define SIA_DC_09_UTL_HPP

#include "Arduino.h"

#include "TimeLib.h"

static uint8_t calculate_day_of_week(uint16_t year, uint8_t month, uint8_t day) {
    static uint8_t mCorr[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    year -= month < 3;
    return uint8_t((year + (year / 4) - (year / 100) + (year / 400) + mCorr[month - 1] + day) % 7) + 1; // 1 - 7, 1 = Sunday
}

static String generate_timestamp(time_t time = now()) {
    uint16_t years  = (uint16_t)year(time);
    uint8_t months  = (uint8_t)month(time);
    uint8_t days    = (uint8_t)day(time);
    uint8_t hours   = (uint8_t)hour(time);
    uint8_t minutes = (uint8_t)minute(time);
    uint8_t seconds = (uint8_t)second(time);

    char timestamp_buffer[21];
    snprintf(timestamp_buffer, sizeof(timestamp_buffer), "_%02d:%02d:%02d,%02d-%02d-%04d",
             hours, minutes, seconds,
             months, days, years);

    return String(timestamp_buffer);
}

static time_t time_from_timestamp(String timestamp) {
    // Parse the timestamp string and return the corresponding time_t value
    if (timestamp.length() != 20) {
        return 0; // Invalid timestamp
    }

    uint8_t hour   = (timestamp.substring(1, 3).toInt() & 0xFF);
    uint8_t minute = (timestamp.substring(4, 6).toInt() & 0xFF);
    uint8_t second = (timestamp.substring(7, 9).toInt() & 0xFF);
    uint8_t day    = (timestamp.substring(13, 15).toInt() & 0xFF);
    uint8_t month  = (timestamp.substring(10, 12).toInt() & 0xFF);

    uint16_t year = (timestamp.substring(16, 20).toInt() & 0xFFFF);

    uint8_t day_of_week = calculate_day_of_week(year, month, day);

    if (year > 1970) {
        year -= 1970; // Adjust year to offset from 1970
    }

    tmElements_t time_elements = {second, minute, hour, day_of_week, day, month, (uint8_t)(year & 0xFF)};

    return makeTime(time_elements);
}

static uint16_t generate_CRC16_ARC(String data) {
    // Set the initial CRC value
    uint16_t crc = 0x0000;

    uint8_t length = data.length();
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint8_t)data[i];

        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x0001) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }

    return crc;
}

static String generate_CRC16_ARC_string(String data) {
    // Generate CRC-16 ARC for the given data
    uint16_t crc = generate_CRC16_ARC(data);

    String crc_string = String(crc, HEX);
    // Pad CRC to 4 characters long
    while (crc_string.length() < 4) {
        crc_string = "0" + crc_string;
    }

    crc_string.toUpperCase();

    return crc_string;
}

#endif