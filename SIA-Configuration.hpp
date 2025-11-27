#if !defined SIA_DC_09_CONFIG_HPP
#define SIA_DC_09_CONFIG_HPP

#define MESSAGE_RESPONSE_TIMEOUT (15 * 1000ULL)

/*
// While there is a defined SIA-DC-09 documentation with fully defined guidelines,
// there are many limitations to it.
//
// Practical implementation of the SIA-DC-09 protocol is not always compliant with the
// documentation, and this configuration file allows for some flexibility.
*/

// Configuration for account number, prefix, and receiver
// For minimum and maximum lengths and padding options
#define ACCOUNT_NUMBER_LEN_MAX 12 // default 12
#define ACCOUNT_NUMBER_LEN_MIN 3  // default 3
#define ACCOUNT_NUMBER_PAD true   // default true
#define ACCOUNT_NUMBER_HEX true   // default true

#define ACCOUNT_PREFIX_LEN_MAX 6 // default 6
#define ACCOUNT_PREFIX_LEN_MIN 1 // default 1
#define ACCOUNT_PREFIX_PAD false // default false
#define ACCOUNT_PREFIX_HEX true  // default true

#define ACCOUNT_RECEIVER_LEN_MAX 6    // default 6
#define ACCOUNT_RECEIVER_LEN_MIN 0    // default 0
#define ACCOUNT_RECEIVER_SEND_0 false // default false
#define ACCOUNT_RECEIVER_PAD false    // default false
#define ACCOUNT_RECEIVER_HEX true     // default true

#define SET_SEQUENCE_FOR_SUPERVISION false // default false

/*
// The following is because sometimes the protocol is not
// correctly implemented, but demand PE must work around it (testing, PoC demo, etc.)
//
// Final deployment/production should not use these options
// as they are invalidate the use of the SIA-DC-09 protocol.
//
// THE DEFAULT FOR THE FOLLOWING IS false
*/
#define SIA_DC_09_IGNORE_INVALID_CRC false
#define SIA_DC_09_IGNORE_INVALID_LENGTH false
#define SIA_DC_09_IGNORE_INVALID_SEQUENCE false
#define SIA_DC_09_IGNORE_INVALID_TIMESTAMP false

#endif