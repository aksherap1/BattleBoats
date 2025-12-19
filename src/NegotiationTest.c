/**
 * @file    NegotiationTest.c
 *
 * @author Caitelyn Huang (chuan188@ucsc.edu)
 *
 * @date June 6, 2025
 */

#include <stdio.h>
#include "Negotiation.h"
#include "BOARD.h"

// Utility macro for test output
#define TEST_RESULT(cond) ((cond) ? "PASS" : "FAIL")

// Track pass/fail counts
typedef struct
{

    int passed;
    int failed;
} TestStats;

// Helper to print function test summary
void print_summary(const char *func_name, TestStats stats)
{
    printf("%s tests: %d passed, %d failed\n\n", func_name, stats.passed, stats.failed);
}

// === Tests for NegotiationHash ===
void test_NegotiationHash()
{
    TestStats stats = {0, 0};

    // Test case 1: Known hash for small number
    NegotiationData input1 = 3;
    NegotiationData expected1 = 9;
    NegotiationData actual1 = NegotiationHash(input1);
    printf("NegotiationHash(%u) == %u? %s\n", input1, expected1, TEST_RESULT(actual1 == expected1));
    if (actual1 == expected1)
        stats.passed++;
    else
        stats.failed++;

    // Test case 2: Known hash for large number (example from spec)
    NegotiationData input2 = 12345;
    NegotiationData expected2 = 43182;
    NegotiationData actual2 = NegotiationHash(input2);
    printf("NegotiationHash(%u) == %u? %s\n", input2, expected2, TEST_RESULT(actual2 == expected2));
    if (actual2 == expected2)
        stats.passed++;
    else
        stats.failed++;

    // Test case 3: Check hash output is less than PUBLIC_KEY
    NegotiationData input3 = 0xFFFF;
    NegotiationData actual3 = NegotiationHash(input3);
    printf("NegotiationHash(%u) < %u? %s\n", input3, (unsigned)PUBLIC_KEY, TEST_RESULT(actual3 < PUBLIC_KEY));
    if (actual3 < PUBLIC_KEY)
        stats.passed++;
    else
        stats.failed++;

    print_summary("NegotiationHash", stats);
}

// === Tests for NegotiationVerify ===
void test_NegotiationVerify()
{
    TestStats stats = {0, 0};

    NegotiationData secret = 1000;
    NegotiationData good_commitment = NegotiationHash(secret);
    NegotiationData bad_commitment = good_commitment + 1;

    // Test 1: Correct secret and commitment
    int result1 = NegotiationVerify(secret, good_commitment);
    printf("NegotiationVerify(correct secret): %s\n", TEST_RESULT(result1 == 1));
    if (result1 == 1)
        stats.passed++;
    else
        stats.failed++;

    // Test 2: Correct secret but wrong commitment
    int result2 = NegotiationVerify(secret, bad_commitment);
    printf("NegotiationVerify(wrong commitment): %s\n", TEST_RESULT(result2 == 0));
    if (result2 == 0)
        stats.passed++;
    else
        stats.failed++;

    // Test 3: Different secret, correct commitment for original secret
    int result3 = NegotiationVerify(secret + 1, good_commitment);
    printf("NegotiationVerify(wrong secret): %s\n", TEST_RESULT(result3 == 0));
    if (result3 == 0)
        stats.passed++;
    else
        stats.failed++;

    print_summary("NegotiationVerify", stats);
}

// === Tests for NegotiateCoinFlip ===
void test_NegotiateCoinFlip()
{
    TestStats stats = {0, 0};

    // Test 1: A=0x0000, B=0x0000 => parity 0 => TAILS
    NegotiationData A1 = 0x0000, B1 = 0x0000;
    NegotiationOutcome out1 = NegotiateCoinFlip(A1, B1);
    printf("NegotiateCoinFlip(0x%X, 0x%X) == TAILS? %s\n", A1, B1, TEST_RESULT(out1 == TAILS));
    if (out1 == TAILS)
        stats.passed++;
    else
        stats.failed++;

    // Test 2: A=0x0001, B=0x0000 => parity 1 => HEADS
    NegotiationData A2 = 0x0001, B2 = 0x0000;
    NegotiationOutcome out2 = NegotiateCoinFlip(A2, B2);
    printf("NegotiateCoinFlip(0x%X, 0x%X) == HEADS? %s\n", A2, B2, TEST_RESULT(out2 == HEADS));
    if (out2 == HEADS)
        stats.passed++;
    else
        stats.failed++;

    // Test 3: Random numbers with odd parity (manually calculated)
    NegotiationData A3 = 0xAAAA; // 1010101010101010
    NegotiationData B3 = 0x5555; // 0101010101010101
    // A3 XOR B3 = 0xFFFF, which has 16 ones => parity 0 => TAILS
    NegotiationOutcome out3 = NegotiateCoinFlip(A3, B3);
    printf("NegotiateCoinFlip(0x%X, 0x%X) == TAILS? %s\n", A3, B3, TEST_RESULT(out3 == TAILS));
    if (out3 == TAILS)
        stats.passed++;
    else
        stats.failed++;

    print_summary("NegotiateCoinFlip", stats);
}

// === Tests for NegotiateGenerateBGivenHash ===
void test_NegotiateGenerateBGivenHash()
{
    TestStats stats = {0, 0};

    NegotiationData secret = 0x1234;
    NegotiationData hash = NegotiationHash(secret);

    // Test 1: B returned should produce HEADS with recovered A
    NegotiationData B = NegotiateGenerateBGivenHash(hash);
    NegotiationData A = 0;

    // Find A that matches hash (should be secret)
    for (NegotiationData guess = 0; guess <= 0xFFFF; guess++)
    {
        if (NegotiationHash(guess) == hash)
        {
            A = guess;
            break;
        }
    }

    NegotiationOutcome outcome = NegotiateCoinFlip(A, B);
    printf("NegotiateGenerateBGivenHash() ensures HEADS? %s\n", TEST_RESULT(outcome == HEADS));
    if (outcome == HEADS)
        stats.passed++;
    else
        stats.failed++;

    // Test 2: A ^ B has odd parity (which implies HEADS)
    NegotiationData combined = A ^ B;
    int parity = 0;
    for (NegotiationData temp = combined; temp; temp >>= 1)
    {
        parity ^= (temp & 1);
    }
    printf("A ^ B has odd parity? %s\n", TEST_RESULT(parity == 1));
    if (parity == 1)
        stats.passed++;
    else
        stats.failed++;

    print_summary("NegotiateGenerateBGivenHash", stats);
}

// === Tests for NegotiateGenerateAGivenB ===
void test_NegotiateGenerateAGivenB()
{
    TestStats stats = {0, 0};

    NegotiationData B = 0x4321;

    // Test 1: Returned A ensures HEADS outcome
    NegotiationData A = NegotiateGenerateAGivenB(B);
    NegotiationOutcome outcome = NegotiateCoinFlip(A, B);
    printf("NegotiateGenerateAGivenB() ensures HEADS? %s\n", TEST_RESULT(outcome == HEADS));
    if (outcome == HEADS)
        stats.passed++;
    else
        stats.failed++;

    // Test 2: A should be in range (<= 0xFFFF)
    printf("Returned A in valid range? %s\n", TEST_RESULT(A <= 0xFFFF));
    if (A <= 0xFFFF)
        stats.passed++;
    else
        stats.failed++;

    print_summary("NegotiateGenerateAGivenB", stats);
}

// === Main test runner ===
int main(void)
{
    BOARD_Init();

    HAL_Delay(1000);
    printf("Running Negotiation module tests...\n\n");

    test_NegotiationHash();
    test_NegotiationVerify();
    test_NegotiateCoinFlip();
    test_NegotiateGenerateBGivenHash();
    test_NegotiateGenerateAGivenB();

    printf("All tests completed.\n");
    return 0;
}
