#if !defined SIA_DC_09_ACC_HPP
#define SIA_DC_09_ACC_HPP

#include "Arduino.h"

#include "SIA-Configuration.hpp"

enum Account_Error {
    ACCOUNT_ERR_NOT_SET    = 0,
    ACCOUNT_ERR_NONE       = 1,
    ACCOUNT_ERR_ACC_NUMBER = 2,
    ACCOUNT_ERR_PREFIX     = 3,
    ACCOUNT_ERR_RECEIVER   = 4
};

static Account_Error validate_account(uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number) {
    String temp_str;
    uint8_t temp_uint8;

    // Validate the account number
#if ACCOUNT_NUMBER_HEX
    temp_str = String(account_number, HEX);
#else
    temp_str = String(account_number);
#endif
    temp_uint8 = temp_str.length();
    if (temp_uint8 > ACCOUNT_NUMBER_LEN_MAX) {
        return ACCOUNT_ERR_ACC_NUMBER;
    }
    if (account_number > 999999) {
        return ACCOUNT_ERR_ACC_NUMBER;
    }

    // Validate the account prefix
#if ACCOUNT_PREFIX_HEX
    temp_str = String(account_prefix, HEX);
#else
    temp_str = String(account_prefix);
#endif
    temp_uint8 = temp_str.length();
    if (temp_uint8 > ACCOUNT_PREFIX_LEN_MAX) {
        return ACCOUNT_ERR_PREFIX;
    }
    if (account_prefix > 999999) {
        return ACCOUNT_ERR_PREFIX;
    }

    // Validate the receiver number
#if ACCOUNT_RECEIVER_HEX
    temp_str = String(receiver_number, HEX);
#else
    temp_str = String(receiver_number);
#endif
    temp_uint8 = temp_str.length();
    if (temp_uint8 > ACCOUNT_RECEIVER_LEN_MAX) {
        return ACCOUNT_ERR_RECEIVER;
    }
    if (receiver_number > 999999) {
        return ACCOUNT_ERR_RECEIVER;
    }

    return ACCOUNT_ERR_NONE;
}

static String account_error_code_to_string(Account_Error error) {
    switch (error) {
        case ACCOUNT_ERR_NONE:
            return "No Error";
        case ACCOUNT_ERR_ACC_NUMBER:
            return "Invalid Account Number";
        case ACCOUNT_ERR_PREFIX:
            return "Invalid Account Prefix";
        case ACCOUNT_ERR_RECEIVER:
            return "Invalid Receiver Number";
        default:
            return "Unknown Error";
    }
}

static String get_account_number_string(uint64_t account_number) {
#if ACCOUNT_NUMBER_HEX
    String account_number_string = String(account_number, HEX);
#else
    String account_number_string = String(account_number);
#endif
    if (account_number_string.length() > ACCOUNT_NUMBER_LEN_MAX) {
        return "";
    }

#if ACCOUNT_NUMBER_PAD
    while (account_number_string.length() < ACCOUNT_NUMBER_LEN_MIN) {
        account_number_string = "0" + account_number_string;
    }
#endif

    return ("#" + account_number_string);
}

static String get_account_prefix_string(uint32_t account_prefix) {
#if ACCOUNT_PREFIX_HEX
    String account_prefix_string = String(account_prefix, HEX);
#else
    String account_prefix_string = String(account_prefix);
#endif
    if (account_prefix_string.length() > ACCOUNT_PREFIX_LEN_MAX) {
        return "";
    }

#if ACCOUNT_PREFIX_PAD
    while (account_prefix_string.length() < ACCOUNT_PREFIX_LEN_MIN) {
        account_prefix_string = "0" + account_prefix_string;
    }
#endif

    return ("L" + account_prefix_string);
}

static String get_receiver_number_string(uint32_t receiver_number) {
#if !ACCOUNT_RECEIVER_SEND_0
    if (receiver_number == 0) {
        return "";
    }
#endif

#if ACCOUNT_RECEIVER_HEX
    String receiver_number_string = String(receiver_number, HEX);
#else
    String receiver_number_string = String(receiver_number);
#endif
    if (receiver_number_string.length() > ACCOUNT_RECEIVER_LEN_MAX) {
        return "";
    }

#if ACCOUNT_RECEIVER_PAD
    while (receiver_number_string.length() < ACCOUNT_RECEIVER_LEN_MIN) {
        receiver_number_string = "0" + receiver_number_string;
    }
#endif

    return ("R" + receiver_number_string);
}

class SIA_DC_09_Account {
    /* Constructors/Destructor */
  public:
    SIA_DC_09_Account(uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number) : _id_token(0), _sequence(1),
                                                                                                    _account_number(account_number), _account_prefix(account_prefix), _receiver_number(receiver_number),
                                                                                                    _is_encrypted(false) {}

    SIA_DC_09_Account(uint16_t sequence, uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number) : _id_token(0), _sequence(sequence),
                                                                                                                       _account_number(account_number), _account_prefix(account_prefix), _receiver_number(receiver_number),
                                                                                                                       _is_encrypted(false) {}

    SIA_DC_09_Account(uint8_t token, uint16_t sequence, uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number) : _id_token(token), _sequence(sequence),
                                                                                                                                      _account_number(account_number), _account_prefix(account_prefix), _receiver_number(receiver_number),
                                                                                                                                      _is_encrypted(false) {}

    ~SIA_DC_09_Account() {}

  private:
    uint8_t _id_token;
    uint16_t _sequence;

    uint64_t _account_number;
    uint32_t _account_prefix;
    uint32_t _receiver_number;

    bool _is_encrypted;
    uint64_t _encryption_key[4] = {0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000};

  public:
    // Getters
    uint64_t get_account_number() const { return _account_number; }
    uint32_t get_account_prefix() const { return _account_prefix; }
    uint32_t get_receiver_number() const { return _receiver_number; }

    // Setters
    void set_account_number(uint64_t account_number) { _account_number = account_number; }
    void set_account_prefix(uint32_t account_prefix) { _account_prefix = account_prefix; }
    void set_receiver_number(uint32_t receiver_number) { _receiver_number = receiver_number; }

    // Encryption state
    bool set_encryption(bool encrypted, uint64_t key1, uint64_t key2, uint64_t key3, uint64_t key4) {
        _is_encrypted = encrypted;

        if (key1 == 0x0 && key2 == 0x0 && key3 == 0x0 && key4 == 0x0) {
            _is_encrypted = false; // Disable encryption if all keys are zero
        }

        if (!_is_encrypted) {
            _encryption_key[0] = 0x0;
            _encryption_key[1] = 0x0;
            _encryption_key[2] = 0x0;
            _encryption_key[3] = 0x0;

            return _is_encrypted;
        }

        _encryption_key[0] = key1;
        _encryption_key[1] = key2;
        _encryption_key[2] = key3;
        _encryption_key[3] = key4;
    }

    bool get_encrypted() const { return _is_encrypted; }

    // ID token
    uint8_t get_id_token() const { return _id_token; }
    void set_id_token(uint8_t id_token) { _id_token = id_token; }

    // Sequence
    uint8_t get_sequence() const { return _sequence; }

    uint8_t increment_sequence() {
        _sequence++;
        if (_sequence > 9999) {
            _sequence = 1; // Reset sequence if it exceeds 9999
        }
        return _sequence;
    }

    uint8_t set_sequence(uint16_t sequence) {
        if (sequence > 9999 || sequence == 0) {
            _sequence = 1; // Reset sequence if it exceeds 9999
        } else {
            _sequence = sequence;
        }
        return _sequence;
    }

    uint8_t reset_sequence() {
        _sequence = 1;
        return _sequence;
    }

    Account_Error validate_registered_account() {
        return validate_account(_account_number, _account_prefix, _receiver_number);
    }

    Account_Error set_account(uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number) {
        _account_number  = account_number;
        _account_prefix  = account_prefix;
        _receiver_number = receiver_number;

        // Validate the account details
        return validate_registered_account();
    }

    Account_Error initialise_account(uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                                     uint8_t token = 0, uint16_t sequence = 1,
                                     bool is_encrypted = false, uint64_t key1 = 0x0, uint64_t key2 = 0x0, uint64_t key3 = 0x0, uint64_t key4 = 0x0) {
        // Set account details
        set_account(account_number, account_prefix, receiver_number);

        // Set message configuration
        set_id_token(token);
        set_sequence(sequence);

        // Set encryption state
        set_encryption(is_encrypted, key1, key2, key3, key4);

        // Validate the account details
        return validate_registered_account();
    }
};

#endif
