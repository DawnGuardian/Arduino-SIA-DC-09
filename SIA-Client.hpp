#if !defined SIA_DC_09_CLIENT_HPP
#define SIA_DC_09_CLIENT_HPP

#include "Arduino.h"

#include "SIA-Message.hpp"

class SIA_DC_09_Client {
    // Constructor/Destructor
  public:
    SIA_DC_09_Client(Client &client) : _client(client), _account(nullptr) {
        _last_message_time = 0;
    }

    SIA_DC_09_Client(Client &client, SIA_DC_09_Account *account) : _client(client), _account(account) {
        _last_message_time = 0;
    }

    ~SIA_DC_09_Client() {}

  private:
    Client &_client;
    SIA_DC_09_Account *_account;

    time_t _last_message_time;

    template <typename T>
    inline void stream_write(T last) {
        _client.print(last);
    }

    template <typename T, typename... Args>
    inline void stream_write(T head, Args... tail) {
        _client.print(head);
        stream_write(tail...);
    }

    // Client methods
  public:
    CSR_Message listen_for_message(uint32_t timeout, bool listen_for_ack) {
        String payload;
        time_t start_millis = millis();

        CSR_Message message;

        do {
            while (_client.available()) {
                int8_t c = _client.read();
                if (c <= 0) {
                    continue; // Skip leading bytes that are 0x00
                }

                payload += static_cast<char>(c);

                // Check for end of message
                if (c == char(0x0D)) {
                    if (payload.length() > 10) {
                        _client.flush();
                        message = parse_message(payload);
                        goto finish;
                    } else {
                        // TODO: remove this inner if block
                        // test server is sending extraneous \r at the beginning
                        payload = "";
                    }
                }
            }
        } while (millis() - start_millis < timeout);

    finish:
        if (message_is_empty(message)) {
            return message;
        }

        if (_account != nullptr) {
#if SIA_DC_09_IGNORE_INVALID_SEQUENCE
#else
            if (message.sequence != _account->get_sequence() && message.type != MESSAGE_TYPE_NAK) {
                // Sequence number invalid
                message.error = MESSAGE_ERR_SEQUENCE;
            }
#endif

            if (message.receiver_number != _account->get_receiver_number() && message.type != MESSAGE_TYPE_NAK) {
                // Receiver number invalid
                message.error = MESSAGE_ERR_RECEIVER;
            }

            if (message.account_prefix != _account->get_account_prefix() && message.type != MESSAGE_TYPE_NAK) {
                // Account prefix invalid
                message.error = MESSAGE_ERR_ACC_PREFIX;
            }

            if (message.account_number != _account->get_account_number()) {
                // Account number invalid
                message.error = MESSAGE_ERR_ACC_NUMBER;
            }
        }

#if SIA_DC_09_IGNORE_INVALID_TIMESTAMP
#else
        // Validate timestamp for message types with timestamp
        if (message.type == MESSAGE_TYPE_ACK_ENC || message.type == MESSAGE_TYPE_RSP || message.type == MESSAGE_TYPE_DUH) {
            if (message.timestamp == 0 || (_last_message_time > message.timestamp) || (message.timestamp - _last_message_time > MESSAGE_RESPONSE_TIMEOUT)) {
                // Timestamp invalid
                message.error = MESSAGE_ERR_TIMEOUT;
            }
        }
#endif

        if (message.error == MESSAGE_ERR_NONE && message.type == MESSAGE_TYPE_ACK && listen_for_ack && _account != nullptr) {
            _account->increment_sequence();
        }

        return message;
    }

    bool send_message(String id_token,
                      uint16_t sequence,
                      uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                      String data, String ext_data,
                      bool is_encrypted,
                      time_t &timestamp) {
        String message = compose_message(id_token, sequence, account_number, account_prefix, receiver_number, data, ext_data, is_encrypted, timestamp);

        if (message == "" || message.length() < 27) {
            return false; // Failed to compose message
        }

        uint32_t bytes_written = _client.print(message);
        bool success           = (bytes_written != 0);
        _client.flush();

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }

    bool send_event_message(uint8_t id_token,
                            uint16_t sequence,
                            uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                            String data, String ext_data,
                            bool is_encrypted,
                            time_t &timestamp) {
        String message = compose_event_message(id_token, sequence, account_number, account_prefix, receiver_number, data, ext_data, is_encrypted, timestamp);

        if (message == "" || message.length() < 27) {
            return false; // Failed to compose message
        }

        uint32_t bytes_written = _client.print(message);
        bool success           = (bytes_written != 0);

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }

    bool send_event_message(String data, String ext_data, time_t &timestamp) {
        if (_account == nullptr) {
            return false;
        }

        bool success = send_event_message(_account->get_id_token(), _account->get_sequence(), _account->get_account_number(), _account->get_account_prefix(), _account->get_receiver_number(), data, ext_data, _account->get_encrypted(), timestamp);

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }

    bool send_supervision_message(
#if SET_SEQUENCE_FOR_SUPERVISION
        uint16_t sequence,
#endif
        uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
        String data, String ext_data,
        bool is_encrypted,
        time_t &timestamp) {
        String message = compose_supervision_message(
#if SET_SEQUENCE_FOR_SUPERVISION
            sequence,
#endif
            account_number, account_prefix, receiver_number,
            data, ext_data,
            is_encrypted,
            timestamp);

        if (message == "" || message.length() < 27) {
            return false; // Failed to compose message
        }

        uint32_t bytes_written = _client.print(message);
        bool success           = (bytes_written != 0);

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }

    bool send_supervision_message(String data, String ext_data, time_t &timestamp) {
        if (_account == nullptr) {
            return false;
        }

#if SET_SEQUENCE_FOR_SUPERVISION
        bool success = send_supervision_message(_account->get_sequence(), _account->get_account_number(), _account->get_account_prefix(), _account->get_receiver_number(), data, ext_data, _account->get_encrypted(), timestamp);
#else
        bool success = send_supervision_message(_account->get_account_number(), _account->get_account_prefix(), _account->get_receiver_number(), data, ext_data, _account->get_encrypted(), timestamp);
#endif

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }

    bool send_ack_message(uint8_t id_token,
                          uint16_t sequence,
                          uint64_t account_number, uint32_t account_prefix, uint32_t receiver_number,
                          String data, String ext_data,
                          bool is_encrypted,
                          time_t &timestamp) {
        String message = compose_ack_message(id_token, sequence, account_number, account_prefix, receiver_number, data, ext_data, is_encrypted, timestamp);

        if (message == "" || message.length() < 27) {
            return false; // Failed to compose message
        }

        uint32_t bytes_written = _client.print(message);
        bool success           = (bytes_written != 0);

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }

    bool send_ack_message(uint8_t id_token, String data, String ext_data, time_t &timestamp) {
        if (_account == nullptr) {
            return false;
        }

        bool success = send_ack_message(id_token, _account->get_sequence(), _account->get_account_number(), _account->get_account_prefix(), _account->get_receiver_number(), data, ext_data, _account->get_encrypted(), timestamp);

        if (success) {
            _last_message_time = timestamp; // Update last message time on successful send
        }

        return success;
    }
};

#endif