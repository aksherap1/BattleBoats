/**
 * @file    AgentTest.c
 *
 * @author Caitelyn Huang (chuan188@ucsc.edu)
 *
 * @date June 6, 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Agent.h"
#include "Message.h"
#include "BattleBoats.h"
#include "Field.h"
#include "Negotiation.h"
#include "BOARD.h"

// --- Test utility macro ---

#define PRINT_TEST(desc, cond)                                    \
    do                                                            \
    {                                                             \
        printf(" - %-40s: %s\n", desc, (cond) ? "PASS" : "FAIL"); \
        if (!(cond))                                              \
            allTestsPassed = 0;                                   \
    } while (0)

typedef struct
{
    int passed;
    int failed;
} TestStats;

static int allTestsPassed = 1;

// =======================
// Test Section: AgentInit()
// =======================

void test_AgentInit(void)
{
    TestStats stats = {0, 0};
    printf("Testing AgentInit()\n");

    // Check initial state after AgentInit() call
    AgentInit();
    AgentState st = AgentGetState();
    PRINT_TEST("State after init == AGENT_STATE_START", st == AGENT_STATE_START);
    stats.passed += (st == AGENT_STATE_START);
    stats.failed += (st != AGENT_STATE_START);

    // Change state, re-init, confirm reset to AGENT_STATE_START
    AgentSetState(AGENT_STATE_ATTACKING);
    AgentInit();
    st = AgentGetState();
    PRINT_TEST("State reset after AgentInit()", st == AGENT_STATE_START);
    stats.passed += (st == AGENT_STATE_START);
    stats.failed += (st != AGENT_STATE_START);

    printf("AgentInit summary: Passed %d, Failed %d\n\n", stats.passed, stats.failed);
}

// ==================================
// Test Section: AgentSetState/AgentGetState()
// ==================================

void test_AgentSetGetState(void)
{
    TestStats stats = {0, 0};
    printf("Testing AgentSetState() and AgentGetState()\n");

    // Test setting/getting DEFENDING state
    AgentSetState(AGENT_STATE_DEFENDING);
    AgentState st = AgentGetState();
    PRINT_TEST("Set/Get DEFENDING", st == AGENT_STATE_DEFENDING);
    stats.passed += (st == AGENT_STATE_DEFENDING);
    stats.failed += (st != AGENT_STATE_DEFENDING);

    // Test setting/getting END_SCREEN state
    AgentSetState(AGENT_STATE_END_SCREEN);
    st = AgentGetState();
    PRINT_TEST("Set/Get END_SCREEN", st == AGENT_STATE_END_SCREEN);
    stats.passed += (st == AGENT_STATE_END_SCREEN);
    stats.failed += (st != AGENT_STATE_END_SCREEN);

    // Test setting/getting CHALLENGING state
    AgentSetState(AGENT_STATE_CHALLENGING);
    st = AgentGetState();
    PRINT_TEST("Set/Get CHALLENGING", st == AGENT_STATE_CHALLENGING);
    stats.passed += (st == AGENT_STATE_CHALLENGING);
    stats.failed += (st != AGENT_STATE_CHALLENGING);

    printf("AgentSetState/AgentGetState summary: Passed %d, Failed %d\n\n", stats.passed, stats.failed);
}

// =============================
// Test Section: AgentRun() - START state
// =============================

// Verify transition from START on BB_EVENT_START_BUTTON event
void test_AgentRun_start_startButton(void)
{
    printf("Testing AgentRun() - START state, BB_EVENT_START_BUTTON\n");
    AgentInit();
    BB_Event event = {.type = BB_EVENT_START_BUTTON, .param0 = 0, .param1 = 0};
    Message msg = AgentRun(event);

    PRINT_TEST("Returns MESSAGE_CHA", msg.type == MESSAGE_CHA);
    PRINT_TEST("State transitions to CHALLENGING",
               AgentGetState() == AGENT_STATE_CHALLENGING);
    printf("\n");
}

// Verify transition from START on BB_EVENT_CHA_RECEIVED event
void test_AgentRun_start_chaReceived(void)
{
    printf("Testing AgentRun() - START state, BB_EVENT_CHA_RECEIVED\n");
    AgentInit();
    BB_Event event = {.type = BB_EVENT_CHA_RECEIVED, .param0 = 0, .param1 = 0};
    Message msg = AgentRun(event);

    PRINT_TEST("Returns MESSAGE_ACC", msg.type == MESSAGE_ACC);
    PRINT_TEST("State transitions to ACCEPTING",
               AgentGetState() == AGENT_STATE_ACCEPTING);
    printf("\n");
}

// =====================================
// Test Section: AgentRun() - CHALLENGING state
// =====================================

// Verify AgentRun on BB_EVENT_ACC_RECEIVED moves to WAITING_TO_SEND or DEFENDING
void test_AgentRun_challenging_accReceived(void)
{
    printf("Testing AgentRun() - CHALLENGING state, BB_EVENT_ACC_RECEIVED\n");
    AgentInit();
    AgentSetState(AGENT_STATE_CHALLENGING);
    BB_Event event = {.type = BB_EVENT_ACC_RECEIVED, .param0 = 1234, .param1 = 0};
    Message msg = AgentRun(event);

    PRINT_TEST("Returns MESSAGE_REV", msg.type == MESSAGE_REV);
    PRINT_TEST("State moves to WAITING_TO_SEND or DEFENDING",
               AgentGetState() == AGENT_STATE_WAITING_TO_SEND || AgentGetState() == AGENT_STATE_DEFENDING);
    printf("\n");
}

// ===================================
// Test Section: AgentRun() - ACCEPTING state
// ===================================

// Verify AgentRun on BB_EVENT_REV_RECEIVED with valid verification
void test_AgentRun_accepting_revReceived_valid(void)
{
    printf("Testing AgentRun() - ACCEPTING state, BB_EVENT_REV_RECEIVED (valid)\n");

    // Reset Agent
    AgentInit();

    // Choose a known A value and compute hashA
    NegotiationData A = rand() % 65536;
    NegotiationData hashA_computed = NegotiationHash(A);

    // Simulate receiving CHA_RECEIVED with that hash
    BB_Event chaEvent = {.type = BB_EVENT_CHA_RECEIVED, .param0 = hashA_computed, .param1 = 0};
    AgentRun(chaEvent); // should store hashA and move to ACCEPTING

    // Now send REV_RECEIVED with the same A to pass verification
    BB_Event revEvent = {.type = BB_EVENT_REV_RECEIVED, .param0 = A, .param1 = A};
    Message msg = AgentRun(revEvent);

    // Validate returned message
    PRINT_TEST("Returns MESSAGE_SHO", msg.type == MESSAGE_SHO);

    AgentState st = AgentGetState();
    PRINT_TEST("State is ATTACKING or DEFENDING",
               st == AGENT_STATE_ATTACKING || st == AGENT_STATE_DEFENDING);

    printf("\n");
}

// =====================================
// Test Section: AgentRun() - WAITING_TO_SEND state
// =====================================

// Verify AgentRun on BB_EVENT_MESSAGE_SENT sends MESSAGE_SHO and moves to ATTACKING
void test_AgentRun_waitingToSend_messageSent(void)
{
    printf("Testing AgentRun() - WAITING_TO_SEND state, BB_EVENT_MESSAGE_SENT\n");
    AgentInit();
    AgentSetState(AGENT_STATE_WAITING_TO_SEND);
    BB_Event event = {.type = BB_EVENT_MESSAGE_SENT, .param0 = 0, .param1 = 0};
    Message msg = AgentRun(event);

    PRINT_TEST("Returns MESSAGE_SHO", msg.type == MESSAGE_SHO);
    PRINT_TEST("State transitions to ATTACKING", AgentGetState() == AGENT_STATE_ATTACKING);
    printf("\n");
}

// ====================
// Main test runner
// ====================

int main(void)
{
    BOARD_Init();

    HAL_Delay(1000);
    printf("=== Starting Agent Module Tests ===\n\n");

    test_AgentInit();        // works
    test_AgentSetGetState(); // works

    test_AgentRun_start_startButton();           // works
    test_AgentRun_start_chaReceived();           /// works
    test_AgentRun_challenging_accReceived();     // works
    test_AgentRun_accepting_revReceived_valid(); // works
    test_AgentRun_waitingToSend_messageSent();   // works

    printf("=== Agent Module Tests %s ===\n", allTestsPassed ? "PASSED" : "FAILED");

    return allTestsPassed ? 0 : 1;
}
