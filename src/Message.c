/**
 * @file    Message.c
 *
 *
 * @author  Caitelyn Huang (chuan188@ucsc.edu)
 *
 * @date    6 June 2025
 */
#include <stdint.h>

#ifndef BOARD_H
#include <BOARD.h>
#endif /*  BOARD_H */

#include "BattleBoats.h"

#include "Message.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isxdigit()

// Internal state for Message_Decode()
typedef enum
{
    WAIT_FOR_START_DELIMITER,
    RECORDING_PAYLOAD,
    RECORDING_CHECKSUM
} DecodeState;

static DecodeState decode_state = WAIT_FOR_START_DELIMITER;
static char payload_buffer[MESSAGE_MAX_PAYLOAD_LEN + 1];
static char checksum_buffer[MESSAGE_CHECKSUM_LEN + 1];
static int payload_index = 0;
static int checksum_index = 0;

/**
 * According to the NMEA standard, messages cannot be longer than 82,
 * including the delimiters $, *, \r, and \n.
 */
#define MESSAGE_MAX_LEN 82

/**
 * The maximum payload length is the maximum message length,
 * -1 start delimiter ($),
 * -1 checksum delimiter (*),
 * -2 checksum characters,
 * -1 end delimiter #1 (\r), and
 * -1 end delimiter #2 (\n).
 */
#define MESSAGE_MAX_PAYLOAD_LEN (MESSAGE_MAX_LEN - 1 - 1 - 2 - 1 - 1)

/* NMEA also defines a specific  checksum length. */
#define MESSAGE_CHECKSUM_LEN 2

/**
 * Each (almost*) follows the NMEA0183 syntax for message payloads:
 *		The first three characters describe the message type;
 * 		zero or more comma-separated fields follow, containing various kinds of
 * 		    data.
 *
 * (* True NEMA0183 payloads begin with two characters that describe the
 *    "talker", which we omit from the BattleBoats protocol.)
 */
#define PAYLOAD_TEMPLATE_CHA "CHA,%u"       // Challenge message:  		hash_a (see protocol)
#define PAYLOAD_TEMPLATE_ACC "ACC,%u"       // Accept message:	 		B (see protocol)
#define PAYLOAD_TEMPLATE_REV "REV,%u"       // Reveal message: 			A (see protocol)
#define PAYLOAD_TEMPLATE_SHO "SHO,%d,%d"    // Shot (guess) message: 	row, col
#define PAYLOAD_TEMPLATE_RES "RES,%u,%u,%u" // Result message: 			row, col, GuessResult

/**
 * NEMA0183 messages wrap the payload with a start delimiter, a checksum to
 * verify the contents of the message in case of transmission errors, and an end
 * delimiter.
 * This template defines the wrapper.
 * Note that it uses printf-style tokens so that it can be used with sprintf().
 *
 * Here is an example message:
 *
 * 1 start delimiter    (a literal $)
 * 2 payload            (any string, represented by %s in the template)
 * 3 checksum delimiter (a literal *)
 * 4 checksum			(two ascii characters representing hex digits, %02x in
 *                       the template)
 * 5 end delimiter      (a literal \r\n)
 *
 * example message:      1       3  5
 *						 v       v  vvvv
 *                       $SHO,2,9*5F\r\n
 *                        ^^^^^^^ ^^
 *                          2     4
 *
 * Note that 2 and 4 correspond to %s and %02x in the template.
 *
 * Also note that valid BattleBoats messages use
 * strictly upper-case letters, so $SHO,2,3*5f\r\n is an invalid message.
 */
#define MESSAGE_TEMPLATE "$%s*%02X\r\n"

/**
 * Given a payload string, calculate its checksum.
 *
 * @param   payload The string whose checksum we wish to calculate.
 * @return  The resulting 8-bit checksum.
 */
uint8_t Message_CalculateChecksum(const char *payload)
{
    uint8_t checksum = 0;
    for (int i = 0; payload[i] != '\0'; i++)
    {
        checksum ^= (uint8_t)payload[i];
    }
    return checksum;
}

/** Message_ParseMessage(*payload, *checksum_string, *message_event)
 *
 * ParseMessage() converts a message string into a BB_Event.  The payload and
 * checksum of a message are passed into ParseMessage(), and it modifies a
 * BB_Event struct in place to reflect the contents of the message.
 *
 * @param   payload         The payload of a message.
 * @param   checksum        The checksum (in string form) of  a message,
 *                              should be exactly 2 chars long, plus a null
 *                              char.
 * @param   message_event   A BB_Event which will be modified by this function.
 *                          If the message could be parsed successfully,
 *                              message_event's type will correspond to the
 *                              message type and its parameters will match the
 *                              message's data fields.
 *                          If the message could not be parsed, message_events
 *                              type will be BB_EVENT_ERROR.
 *
 * @return  STANDARD_ERROR if:
 *              the payload does not match the checksum,
 *              the checksum string is not two characters long, or
 *              the message does not match any message template;
 *          SUCCESS otherwise.
 *
 * @note    Please note!  sscanf() has a couple compiler bugs that make it an
 *          unreliable tool for implementing this function.
 */
int Message_ParseMessage(
    const char *payload,
    const char *checksum_string,
    BB_Event *message_event)
{

    if (strlen(checksum_string) != MESSAGE_CHECKSUM_LEN)
    {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    // Calculate and verify checksum
    uint8_t computed_checksum = Message_CalculateChecksum(payload);
    unsigned int parsed_checksum;
    if (sscanf(checksum_string, "%2X", &parsed_checksum) != 1)
    {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    if (computed_checksum != (uint8_t)parsed_checksum)
    {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    // Parse payload message type (first three chars)
    char message_type[4] = {0};
    if (sscanf(payload, "%3s", message_type) != 1)
    {
        message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }

    // Now parse parameters based on message_type
    if (strcmp(message_type, "CHA") == 0)
    {
        unsigned int p0;
        if (sscanf(payload, "CHA,%u", &p0) == 1)
        {
            message_event->type = BB_EVENT_CHA_RECEIVED;
            message_event->param0 = p0;
            return SUCCESS;
        }
    }
    else if (strcmp(message_type, "ACC") == 0)
    {
        unsigned int p0;
        if (sscanf(payload, "ACC,%u", &p0) == 1)
        {
            message_event->type = BB_EVENT_ACC_RECEIVED;
            message_event->param0 = p0;
            return SUCCESS;
        }
    }
    else if (strcmp(message_type, "REV") == 0)
    {
        unsigned int p0;
        if (sscanf(payload, "REV,%u", &p0) == 1)
        {
            message_event->type = BB_EVENT_REV_RECEIVED;
            message_event->param0 = p0;
            return SUCCESS;
        }
    }
    else if (strcmp(message_type, "SHO") == 0)
    {
        int row, col;
        if (sscanf(payload, "SHO,%d,%d", &row, &col) == 2)
        {
            message_event->type = BB_EVENT_SHO_RECEIVED;
            message_event->param0 = (unsigned int)row;
            message_event->param1 = (unsigned int)col;
            return SUCCESS;
        }
    }
    else if (strcmp(message_type, "RES") == 0)
    {
        unsigned int row, col, guess_result;
        if (sscanf(payload, "RES,%u,%u,%u", &row, &col, &guess_result) == 3)
        {
            message_event->type = BB_EVENT_RES_RECEIVED;
            message_event->param0 = row;
            message_event->param1 = col;
            message_event->param2 = guess_result;
            return SUCCESS;
        }
    }

    // If none matched
    message_event->type = BB_EVENT_ERROR;
    return STANDARD_ERROR;
}

/** Message_Encode(*message_string, message_to_encode)
 *
 * Encodes the coordinate data for a guess into the string `message`. This
 * string must be big enough to contain all of the necessary data. The format is
 * specified in PAYLOAD_TEMPLATE_*, which is then wrapped within the message as
 * defined by MESSAGE_TEMPLATE.
 *
 * The final length of this message is then returned. There is no failure mode
 * for this function as there is no checking for NULL pointers.
 *
 * @param   message             The character array used for storing the output.
 *                                  Must be long enough to store the entire
 *                                  string, see MESSAGE_MAX_LEN.
 * @param   message_to_encode   A message to encode
 * @return  The length of the string stored into 'message_string'. Return 0 if
 *          message type is MESSAGE_NONE.
 */
int Message_Encode(char *message_string, Message message_to_encode)
{
    if (message_to_encode.type == MESSAGE_NONE)
    {
        return 0;
    }

    char payload[MESSAGE_MAX_PAYLOAD_LEN + 1];

    switch (message_to_encode.type)
    {
    case MESSAGE_CHA:
        snprintf(payload, sizeof(payload), PAYLOAD_TEMPLATE_CHA, message_to_encode.param0);
        break;
    case MESSAGE_ACC:
        snprintf(payload, sizeof(payload), PAYLOAD_TEMPLATE_ACC, message_to_encode.param0);
        break;
    case MESSAGE_REV:
        snprintf(payload, sizeof(payload), PAYLOAD_TEMPLATE_REV, message_to_encode.param0);
        break;
    case MESSAGE_SHO:
        snprintf(payload, sizeof(payload), PAYLOAD_TEMPLATE_SHO, (int)message_to_encode.param0, (int)message_to_encode.param1);
        break;
    case MESSAGE_RES:
        snprintf(payload, sizeof(payload), PAYLOAD_TEMPLATE_RES, message_to_encode.param0, message_to_encode.param1, message_to_encode.param2);
        break;
    default:
        return 0;
    }

    uint8_t checksum = Message_CalculateChecksum(payload);

    int len = snprintf(message_string, MESSAGE_MAX_LEN + 1, MESSAGE_TEMPLATE, payload, checksum);

    return len;
}

/** Message_Decode(char_in, *decoded_message_event)
 *
 * Message_Decode reads one character at a time.  If it detects a full NMEA
 * message, it translates that message into a BB_Event struct, which can be
 * passed to other services.
 *
 * @param   char_in         The next character in the NMEA0183 message to be
 *                              decoded.
 * @param   decoded_message A pointer to a message struct, used to "return" a
 *                              message:
 *                          If char_in is the last character of a valid message,
 *                              then decoded_message
 *                              should have the appropriate message type.
 *                          If char_in is the last character of an invalid
 *                              message,
 *                              then decoded_message should have an ERROR type.
 *                          otherwise, it should have type NO_EVENT.
 * @return  SUCCESS if no error was detected,
 *          STANDARD_ERROR if an error was detected.
 *
 * @note    ANY call to Message_Decode may modify decoded_message.
 * @todo    Make "returned" event variable name consistent.
 */
int Message_Decode(unsigned char char_in, BB_Event *decoded_message_event)
{

    switch (decode_state)
    {
    case WAIT_FOR_START_DELIMITER:
        if (char_in == '$')
        {
            // reset buffers and indexes
            payload_index = 0;
            checksum_index = 0;
            memset(payload_buffer, 0, sizeof(payload_buffer));
            memset(checksum_buffer, 0, sizeof(checksum_buffer));
            decode_state = RECORDING_PAYLOAD;
        }
        decoded_message_event->type = BB_EVENT_NO_EVENT;
        return SUCCESS;

    case RECORDING_PAYLOAD:
        if (char_in == '*')
        {
            decode_state = RECORDING_CHECKSUM;
        }
        else if (char_in == '\r' || char_in == '$')
        {
            decoded_message_event->type = BB_EVENT_ERROR;
            decode_state = WAIT_FOR_START_DELIMITER;
            return STANDARD_ERROR;
        }
        else if (payload_index >= MESSAGE_MAX_PAYLOAD_LEN)
        {
            decoded_message_event->type = BB_EVENT_ERROR;
            decode_state = WAIT_FOR_START_DELIMITER;
            return STANDARD_ERROR;
        }
        else
        {
            payload_buffer[payload_index++] = (char)char_in;
            payload_buffer[payload_index] = '\0'; // keep null terminated
        }
        decoded_message_event->type = BB_EVENT_NO_EVENT;
        return SUCCESS;

    case RECORDING_CHECKSUM:
        if (checksum_index < MESSAGE_CHECKSUM_LEN)
        {
            // Collect exactly two hex digits for checksum
            if (isxdigit(char_in))
            {
                checksum_buffer[checksum_index++] = (char)char_in;
                checksum_buffer[checksum_index] = '\0';
                decoded_message_event->type = BB_EVENT_NO_EVENT;
                return SUCCESS;
            }
            else
            {
                // Invalid character in checksum
                decoded_message_event->type = BB_EVENT_ERROR;
                decode_state = WAIT_FOR_START_DELIMITER;
                return STANDARD_ERROR;
            }
        }
        else if (checksum_index == MESSAGE_CHECKSUM_LEN)
        {
            // Expect \r immediately after checksum
            if (char_in == '\r')
            {
                checksum_index++; // advance index to track \r
                decoded_message_event->type = BB_EVENT_NO_EVENT;
                return SUCCESS;
            }
            else
            {
                decoded_message_event->type = BB_EVENT_ERROR;
                decode_state = WAIT_FOR_START_DELIMITER;
                return STANDARD_ERROR;
            }
        }
        else if (checksum_index == MESSAGE_CHECKSUM_LEN + 1)
        {
            // After \r, expect \n to end the message
            if (char_in == '\n')
            {
                // Complete message received, parse it
                int parse_result = Message_ParseMessage(payload_buffer, checksum_buffer, decoded_message_event);
                decode_state = WAIT_FOR_START_DELIMITER;
                if (parse_result == SUCCESS)
                {
                    return SUCCESS;
                }
                else
                {
                    decoded_message_event->type = BB_EVENT_ERROR;
                    return STANDARD_ERROR;
                }
            }
            else
            {
                decoded_message_event->type = BB_EVENT_ERROR;
                decode_state = WAIT_FOR_START_DELIMITER;
                return STANDARD_ERROR;
            }
        }
        else
        {
            // Any other condition is an error, reset
            decoded_message_event->type = BB_EVENT_ERROR;
            decode_state = WAIT_FOR_START_DELIMITER;
            return STANDARD_ERROR;
        }

    default:
        // Should never happen, reset state
        decode_state = WAIT_FOR_START_DELIMITER;
        decoded_message_event->type = BB_EVENT_ERROR;
        return STANDARD_ERROR;
    }
}
