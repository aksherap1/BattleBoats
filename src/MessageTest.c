// /**
//  * @file    MessageTest.c
//  *
//  * @author  Akshera Paladhi
//  *
//  * @date    June 8 2025
//  */
// #include <stdio.h>
// #include <string.h>
// #include "BOARD.h"
// #include "Message.h"  // your header
// #include "BattleBoats.h" // for BB_Event definitions

// // Helper to print test result
// void Check(int condition, const char* testName) {
//     if (condition) {
//         printf(" %s passed\n", testName);
//     } else {
//         printf(" %s FAILED\n", testName);
//     }
// }

// // Test checksum calculation
// void Test_Message_CalculateChecksum() {
//     const char* payload = "SHO,2,9";
//     uint8_t checksum = Message_CalculateChecksum(payload);
//     // Pre-calculated checksum for "SHO,2,9" is 0x5F (from your example)
//     Check(checksum == 0x5F, "Message_CalculateChecksum");
// }

// // Test parsing a valid SHO message
// void Test_Message_ParseMessage_ValidSHO() {
//     const char* payload = "SHO,2,9";
//     const char* checksum_str = "5F";
//     BB_Event event = {0};
//     int res = Message_ParseMessage(payload, checksum_str, &event);

//     Check(res == SUCCESS, "ParseMessage SHO returns SUCCESS");
//     Check(event.type == BB_EVENT_SHO_RECEIVED, "ParseMessage SHO type check");
//     Check(event.param0 == 2 && event.param1 == 9, "ParseMessage SHO parameters");
// }

// // Test parsing an invalid checksum
// void Test_Message_ParseMessage_InvalidChecksum() {
//     const char* payload = "SHO,2,9";
//     const char* checksum_str = "00";  // Wrong checksum
//     BB_Event event = {0};
//     int res = Message_ParseMessage(payload, checksum_str, &event);

//     Check(res == STANDARD_ERROR, "ParseMessage invalid checksum returns error");
//     Check(event.type == BB_EVENT_ERROR, "ParseMessage invalid checksum event type");
// }

// // Test encoding a SHO message and then decoding it back
// void Test_Message_EncodeDecode_SHO() {
//     Message msg_to_encode = {0};
//     msg_to_encode.type = MESSAGE_SHO;
//     msg_to_encode.param0 = 3;
//     msg_to_encode.param1 = 7;

//     char buffer[MESSAGE_MAX_LEN + 1] = {0};
//     int len = Message_Encode(buffer, msg_to_encode);

//     Check(len > 0, "Message_Encode SHO length > 0");

//     // Now decode char by char
//     BB_Event decoded_event = {0};
//     int decode_status = SUCCESS;
//     for (int i = 0; i < len; i++) {
//         int res = Message_Decode(buffer[i], &decoded_event);
//         if (res != SUCCESS) {
//             decode_status = res;
//             break;
//         }
//     }

//     Check(decode_status == SUCCESS, "Message_Decode SHO decode status");
//     Check(decoded_event.type == BB_EVENT_SHO_RECEIVED, "Message_Decode SHO event type");
//     Check(decoded_event.param0 == 3 && decoded_event.param1 == 7, "Message_Decode SHO parameters");
// }

// // Test Message_Decode error detection on malformed message
// void Test_Message_Decode_Error() {
//     const char* bad_msg = "$SHO,3,7*ZZ\r\n";  // ZZ is invalid checksum chars

//     BB_Event event = {0};
//     int res = SUCCESS;
//     for (int i = 0; i < (int)strlen(bad_msg); i++) {
//         res = Message_Decode(bad_msg[i], &event);
//         if (res != SUCCESS) break;
//     }

//     Check(res == STANDARD_ERROR, "Message_Decode error status");
//     Check(event.type == BB_EVENT_ERROR, "Message_Decode error event type");
// }

// int main(void) {
//     BOARD_Init();

//     HAL_Delay(5000);

//     printf("Running Message module tests...\n\n");

//     Test_Message_CalculateChecksum();
//     Test_Message_ParseMessage_ValidSHO();
//     Test_Message_ParseMessage_InvalidChecksum();
//     Test_Message_EncodeDecode_SHO();
//     Test_Message_Decode_Error();

//     printf("\nTesting completed.\n");

//     return 0;
// }
