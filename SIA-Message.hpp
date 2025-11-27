#if !defined SIA_DC_09_MSG_HPP
#define SIA_DC_09_MSG_HPP

#include "Arduino.h"

#include "SIA-Utility.hpp"

#include "SIA-Account.hpp"

enum CSR_Message_Error {
    MESSAGE_ERR_NONE       = 0,
    MESSAGE_ERR_INVALID    = 1,
    MESSAGE_ERR_CRC        = 2,
    MESSAGE_ERR_LEN        = 3,
    MESSAGE_ERR_ID_TOKEN   = 4,
    MESSAGE_ERR_SEQUENCE   = 5,
    MESSAGE_ERR_RECEIVER   = 6,
    MESSAGE_ERR_ACC_PREFIX = 7,
    MESSAGE_ERR_ACC_NUMBER = 8,
    MESSAGE_ERR_TIMEOUT    = 9,
};

enum CSR_Message_Type {
    MESSAGE_TYPE_ERROR   = 0,
    MESSAGE_TYPE_ACK     = 1,
    MESSAGE_TYPE_ACK_ENC = 2,
    MESSAGE_TYPE_NAK     = 3,
    MESSAGE_TYPE_DUH     = 4,
    MESSAGE_TYPE_RSP     = 5
};

struct CSR_Message {
    CSR_Message_Error error = MESSAGE_ERR_INVALID;
    CSR_Message_Type type   = MESSAGE_TYPE_ERROR;

    uint16_t sequence = 0;

    uint32_t receiver_number = 0;
    uint32_t account_prefix  = 0;
    uint64_t account_number  = 0;

    String data     = "";
    String ext_data = "";

    time_t timestamp = 0;
};

// Message composition functions
static String compose_message(String id_token,
                              uint16_t sequence,
                              uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                              String data, String ext_data,
                              bool is_encrypted,
                              time_t &timestamp) {
    if (validate_account(account_number, account_prefix, receiver_number) != ACCOUNT_ERR_NONE) {
        return "";
    }

    // Start message with encryption flag
    String message = is_encrypted ? "\"*" : "\"";

    // Set DC-07 protocol ID
    message += id_token + "\"";

    // Set current message sequence
    String temp_str = String(sequence);
    while (temp_str.length() < 4) {
        temp_str = "0" + temp_str;
    }
    message += temp_str;

    // Set receiver number
    message += get_receiver_number_string(receiver_number);

    // Set account prefix
    message += get_account_prefix_string(account_prefix);

    // Set account number
    message += get_account_number_string(account_number);

    // Uppercase all details
    message.toUpperCase();

    // Add data
    message += "[" + data + "]";

    // Add extended data (if available))
    if (!ext_data.isEmpty()) {
        message += "[" + ext_data + "]";
    }

    // Add timestamp
    if (timestamp != 0) {
        message += generate_timestamp(timestamp); // Use current time if not provided
    }

    // Generate message length data
    if (message.length() > 0xFFF) {
        return "";
    }

    temp_str = String(message.length(), HEX);
    while (temp_str.length() < 4) {
        temp_str = "0" + temp_str;
    }
    temp_str.toUpperCase();

    // Generate CRC-16 ARC
    String crc = generate_CRC16_ARC_string(message);

    // Add start marker, CRC, length, and end marker
    message = String(char(0x0A)) + crc + temp_str + message + String(char(0x0D));

    return message;
}

static String compose_event_message(uint8_t id_token,
                                    uint16_t sequence,
                                    uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                                    String data, String ext_data,
                                    bool is_encrypted,
                                    time_t &timestamp) {
    String token;
    if (id_token == 0) {
        token = "SIA-DCS";
    } else {
        token = "ADM-CID";
    }

    return compose_message(token, sequence, account_number, account_prefix, receiver_number, data, ext_data, is_encrypted, timestamp);
}

static String compose_supervision_message(
#if SET_SEQUENCE_FOR_SUPERVISION
    uint16_t sequence,
#endif
    uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
    String data, String ext_data,
    bool is_encrypted,
    time_t &timestamp) {

    String token = "";
    if (ext_data.isEmpty()) {
        token += "NULL";
    } else {
        token += "XNM";
    }

    uint16_t sequence_value;
#if SET_SEQUENCE_FOR_SUPERVISION
    sequence_value = sequence;
#else
    sequence_value = 0;
#endif

    return compose_message(token, sequence_value, account_number, account_prefix, receiver_number, data, ext_data, is_encrypted, timestamp);
}

static String compose_ack_message(uint8_t id_token,
                                  uint16_t sequence,
                                  uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                                  String data, String ext_data,
                                  bool is_encrypted,
                                  time_t &timestamp) {

    String token;
    if (id_token == 0) {
        token = "ACK";
    } else if (id_token == 1) {
        token = "NAK";
    } else if (id_token == 2) {
        token = "DUH";
    } else {
        return "";
    }

    return compose_message(token, sequence, account_number, account_prefix, receiver_number, data, ext_data, is_encrypted, timestamp);
}

// Check if the given message is empty
static bool message_is_empty(CSR_Message message) {
    return (message.error == MESSAGE_ERR_INVALID && message.type == MESSAGE_TYPE_ERROR &&
            message.sequence == 0 && message.receiver_number == 0 && message.account_prefix == 0 && message.account_number == 0 &&
            message.data.isEmpty() && message.ext_data.isEmpty() && message.timestamp == 0);
}

// Error codes to string conversion
static String message_error_code_to_string(CSR_Message_Error error) {
    switch (error) {
        case MESSAGE_ERR_NONE:
            return "No Error";
        case MESSAGE_ERR_INVALID:
            return "Invalid Message";
        case MESSAGE_ERR_CRC:
            return "CRC Error";
        case MESSAGE_ERR_LEN:
            return "Length Error";
        case MESSAGE_ERR_ID_TOKEN:
            return "Invalid ID Token";
        case MESSAGE_ERR_SEQUENCE:
            return "Invalid Sequence";
        case MESSAGE_ERR_RECEIVER:
            return "Invalid Receiver";
        case MESSAGE_ERR_ACC_PREFIX:
            return "Invalid Account Prefix";
        case MESSAGE_ERR_ACC_NUMBER:
            return "Invalid Account Number";
        case MESSAGE_ERR_TIMEOUT:
            return "Message Timeout";
        default:
            return "Unknown Error";
    }
}

static String message_type_code_to_string(CSR_Message_Type type) {
    switch (type) {
        case MESSAGE_TYPE_ERROR:
            return "Error";
        case MESSAGE_TYPE_ACK:
            return "Acknowledgment";
        case MESSAGE_TYPE_ACK_ENC:
            return "Encrypted Acknowledgment";
        case MESSAGE_TYPE_NAK:
            return "Negative Acknowledgment";
        case MESSAGE_TYPE_DUH:
            return "DUH Message";
        case MESSAGE_TYPE_RSP:
            return "Response";
        default:
            return "Unknown Type";
    }
}

// Message deconstruction
static CSR_Message parse_message(String payload) {
    CSR_Message message;

    uint16_t payload_length = payload.length();

    // Check if the payload length is within valid range
    if (payload_length < 27 || payload_length > 0xFFF) {
        // Message too short or too long

        // It should send an appropriate empty message
        // Add assignments here if inconsistent
        return message;
    }

    // Check if the start and end frame are correct
    if ((payload[0] != char(0x0A)) || (payload[payload_length - 1] != char(0x0D))) {
        // Start and end frame incorrect

        // It should send an appropriate empty message
        // Add assignments here if inconsistent
        return message;
    }

    // Check if the CRC is valid
    String temp_str   = payload.substring(1, 5);
    uint16_t temp_u16 = strtol(temp_str.c_str(), nullptr, 16) & 0xFFFF;
    uint16_t calc_u16 = generate_CRC16_ARC(payload.substring(9, payload_length - 1));
#if SIA_DC_09_IGNORE_INVALID_CRC
#else
    if (calc_u16 != temp_u16) {
        // CRC error
        message.error = MESSAGE_ERR_CRC;
        message.type  = MESSAGE_TYPE_ERROR;
        return message;
    }
#endif

    // Check if the message length is valid
    temp_str = payload.substring(5, 9);
    temp_u16 = strtol(temp_str.c_str(), nullptr, 16) & 0xFFFF;
    calc_u16 = payload_length - 10;
#if SIA_DC_09_IGNORE_INVALID_LENGTH
#else
    if (calc_u16 != temp_u16) {
        // Message length error
        message.error = MESSAGE_ERR_LEN;
        message.type  = MESSAGE_TYPE_ERROR;
        return message;
    }
#endif

    // Check if the identifier is valid
    if (
        (payload[9] != '"') ||
        !(
            ((payload[10] == '*') && (payload[14] == '"')) ||
            ((payload[10] != '*') && (payload[13] == '"')))) {
        // Identifier invalid
        message.error = MESSAGE_ERR_ID_TOKEN;
        message.type  = MESSAGE_TYPE_ERROR;
        return message;
    }

    bool is_encrypted = (payload[10] == '*');

    temp_str = (is_encrypted ? payload.substring(10, 14) : payload.substring(10, 13));
    if (temp_str == "ACK") {
        message.type = MESSAGE_TYPE_ACK;
    } else if (temp_str == "*ACK") {
        message.type = MESSAGE_TYPE_ACK_ENC;
    } else if (temp_str == "NAK") {
        message.type = MESSAGE_TYPE_NAK;
    } else if (temp_str == "DUH") {
        message.type = MESSAGE_TYPE_DUH;
    } else if (temp_str == "RSP") {
        message.type = MESSAGE_TYPE_RSP;
    } else {
        // Message type invalid
        message.error = MESSAGE_ERR_ID_TOKEN;
        message.type  = MESSAGE_TYPE_ERROR;
        return message;
    }

    // Running index for parsing
    uint16_t begin_index = is_encrypted ? 15 : 14;
    uint16_t end_index   = begin_index + 4;
    uint32_t temp_u32;

    // Strip sequence number
    temp_str         = payload.substring(begin_index, end_index);
    temp_u16         = (uint16_t)(strtol(temp_str.c_str(), nullptr, 10) & 0xFFFF);
    message.sequence = temp_u16;

    // Strip and check receiver number
    begin_index = end_index;
    if (payload[begin_index] != 'R') {
        temp_str = "0";
    } else {
        begin_index++;
        end_index = begin_index + 1;

        while (payload[end_index] != 'L' && (end_index - begin_index) < ACCOUNT_RECEIVER_LEN_MAX && end_index < payload_length) {
            end_index++;
        }

        if (payload[end_index] != 'L') {
            // Receiver number invalid
            message.error = MESSAGE_ERR_RECEIVER;
            return message;
        }

        temp_str = payload.substring(begin_index, end_index);

        begin_index = end_index;
    }

#if ACCOUNT_RECEIVER_HEX
    temp_u32 = strtol(temp_str.c_str(), nullptr, 16) & 0xFFFFFFFF;
#else
    temp_u32 = strtol(temp_str.c_str(), nullptr, 10) & 0xFFFFFFFF;
#endif
    message.receiver_number = temp_u32;

    // Strip and check account prefix
    if (payload[begin_index] != 'L') {
        // Account prefix invalid
        message.error = MESSAGE_ERR_ACC_PREFIX;
        return message;
    }

    begin_index++;
    end_index = begin_index + 1;

    while (payload[end_index] != '#' && (end_index - begin_index) < ACCOUNT_PREFIX_LEN_MAX && end_index < payload_length) {
        end_index++;
    }

    if (payload[end_index] != '#') {
        // Account prefix invalid
        message.error = MESSAGE_ERR_ACC_PREFIX;
        return message;
    }

    temp_str = payload.substring(begin_index, end_index);
#if ACCOUNT_PREFIX_HEX
    temp_u32 = strtol(temp_str.c_str(), nullptr, 16) & 0xFFFFFFFF;
#else
    temp_u32 = strtol(temp_str.c_str(), nullptr, 10) & 0xFFFFFFFF;
#endif
    message.account_prefix = temp_u32;

    begin_index = end_index;

    // Strip and check account number
    if (payload[begin_index] != '#') {
        // Account number invalid
        message.error = MESSAGE_ERR_ACC_NUMBER;
        return message;
    }

    begin_index++;
    end_index = begin_index + 1;

    while (payload[end_index] != '[' && (end_index - begin_index) < ACCOUNT_NUMBER_LEN_MAX && end_index < payload_length) {
        end_index++;
    }

    if (payload[end_index] != '[') {
        // Account number invalid
        message.error = MESSAGE_ERR_ACC_NUMBER;
        return message;
    }

    temp_str = payload.substring(begin_index, end_index);
#if ACCOUNT_NUMBER_HEX
    uint64_t temp_u64 = strtol(temp_str.c_str(), nullptr, 16) & 0xFFFFFFFFFFFFFFFF;
#else
    uint64_t temp_u64 = strtol(temp_str.c_str(), nullptr, 10) & 0xFFFFFFFFFFFFFFFF;
#endif
    message.account_number = temp_u64;

    // Strip data
    begin_index = end_index;
    end_index   = begin_index + 1;

    if (payload[begin_index] != '[') {
        message.data = "";
    } else {
        while (payload[end_index] != ']' && payload[end_index] != '[' && payload[end_index] != '_' && end_index < payload_length) {
            end_index++;
        }

        if (end_index != (begin_index + 1)) {
            message.data = payload.substring(begin_index + 1, end_index);
        } else {
            message.data = "";
        }
    }

    // Strip extended data
    begin_index = end_index;
    end_index   = begin_index + 1;

    if (payload[begin_index] != '[') {
        message.ext_data = "";
    } else {
        while (payload[end_index] != ']' && payload[end_index] != '_' && end_index < payload_length) {
            end_index++;
        }

        if (end_index != (begin_index + 1)) {
            message.ext_data = payload.substring(begin_index + 1, end_index);
        } else {
            message.ext_data = "";
        }
    }

    // Strip timestamp
    begin_index = end_index;
    end_index   = begin_index + 1;

    if (payload[begin_index] != '_') {
        message.timestamp = 0;
    } else {
        while (payload[end_index] != char(0x0D) && end_index < payload_length) {
            end_index++;
        }

        if (end_index != (begin_index + 1)) {
            message.timestamp = time_from_timestamp(payload.substring(begin_index, end_index));
        } else {
            message.timestamp = 0;
        }
    }

    message.error = MESSAGE_ERR_NONE;

    // NAK should have all fields set to 0 or empty
    if (message.type == MESSAGE_TYPE_NAK) {
        if (!message.data.isEmpty() &&
            !message.ext_data.isEmpty() &&
            message.account_number != 0 &&
            message.receiver_number != 0 &&
            message.account_prefix != 0) {
            message.error = MESSAGE_ERR_INVALID;
        }
    }

    return message;
}

// Message composition functions with account
static String account_compose_event_message(SIA_DC_09_Account account, String data, String ext_data, time_t &timestamp) {
    return compose_event_message(account.get_id_token(), account.get_sequence(),
                                 account.get_account_number(), account.get_account_prefix(), account.get_receiver_number(),
                                 data, ext_data,
                                 account.get_encrypted(),
                                 timestamp);
}

static String account_compose_supervision_message(SIA_DC_09_Account account, String data, String ext_data, time_t &timestamp) {
#if SET_SEQUENCE_FOR_SUPERVISION
    return compose_supervision_message(account.get_sequence(),
                                       account.get_account_number(), account.get_account_prefix(), account.get_receiver_number(),
                                       data, ext_data,
                                       account.get_encrypted(),
                                       timestamp);
#else
    return compose_supervision_message(account.get_account_number(), account.get_account_prefix(), account.get_receiver_number(),
                                       data, ext_data,
                                       account.get_encrypted(),
                                       timestamp);
#endif
}

static String account_compose_ack_message(SIA_DC_09_Account account, uint8_t id_token, String data, String ext_data, time_t &timestamp) {
    return compose_ack_message(id_token, account.get_sequence(),
                               account.get_account_number(), account.get_account_prefix(), account.get_receiver_number(),
                               data, ext_data,
                               account.get_encrypted(),
                               timestamp);
}

#endif