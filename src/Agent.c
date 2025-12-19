/**
 * @file    Agent.c
 * 
 * @brief   Controls the BattleBoats game agent logic and state transitions.
 *
 * This module manages the agent's behavior across all phases of the BattleBoats game,
 * including initiating and accepting challenges, performing coin flips to determine
 * turn order, placing boats, processing guesses and results, and displaying the OLED 
 * game field. The agent transitions through various states such as START, CHALLENGING,
 * ACCEPTING, ATTACKING, DEFENDING, WAITING_TO_SEND, and END_SCREEN. 
 * It communicates with the opponent using structured messages (CHA, ACC, REV, SHO, RES).
 * 
 * @author  Akshera Paladhi (apaladhi@ucsc.edu)
 * @date    June 6, 2025
 */

 #include <stdint.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "Message.h"
 #include "BattleBoats.h"
 #include "FieldOled.h"
 #include "Field.h"
 #include "Negotiation.h"
 #include "Oled.h"
 #include <string.h>
 #include <stdbool.h>
 
 // Number of boats in the game
 #define NUM_BOATS 4
 
 // Agent's internal states for managing game flow
 typedef enum {
     AGENT_STATE_START,
     AGENT_STATE_CHALLENGING,
     AGENT_STATE_ACCEPTING,
     AGENT_STATE_ATTACKING,
     AGENT_STATE_DEFENDING,
     AGENT_STATE_WAITING_TO_SEND,
     AGENT_STATE_END_SCREEN,
 } AgentState;
 
 // Static global variables for tracking game state and metadata
 static AgentState agentState;
 static uint8_t turn_counter;
 static Field oppField;
 static Field ownField;
 static uint16_t A, B, hashA;
 static GuessData own_guess;
 static FieldOledTurn playerTurn;
 static NegotiationOutcome turn_order;
 static bool endScreenDrawn = false;
 
 // Draws a single field (unused internal helper)
 void _FieldOledDrawField(const Field *f, int xOffset);
 
 // Initializes all Agent state and fields at the start or on reset
 void AgentInit(void) {
     OLED_Init();
     agentState = AGENT_STATE_START;
     turn_counter = 0;
     A = B = hashA = 0;
     playerTurn = FIELD_OLED_TURN_NONE;
     endScreenDrawn = false;
     FieldInit(&ownField, &oppField);
 }
 
 // Main function to run the Agent logic based on event input
 Message AgentRun(BB_Event event) {
     Message messageToSend = { .type = MESSAGE_NONE };
 
     // Handle reset event
     if (event.type == BB_EVENT_RESET_BUTTON) {
         AgentInit();
         agentState = AGENT_STATE_START;
         OLED_Clear(OLED_COLOR_BLACK);
         OLED_DrawString("BATTLEBOATS:\n\nPress BTN4 to begin\nor wait for PLAYER2.");
         OLED_Update();
         return messageToSend;
     }
 
     switch (agentState) {
 
         // Start state: wait for challenge or accept
         case AGENT_STATE_START:
             OLED_Clear(OLED_COLOR_BLACK);
             OLED_DrawString("BATTLEBOATS:\n\nPress BTN4 to begin\nor wait for PLAYER2.");
             _FieldOledDrawField(&ownField, 0);
             OLED_Update();
 
             // If challenge initiated
             if (event.type == BB_EVENT_START_BUTTON) {
                 A = rand() % 65536;
                 hashA = NegotiationHash(A);
                 messageToSend.type = MESSAGE_CHA;
                 messageToSend.param0 = hashA;
 
                 // Place boats and transition to CHALLENGING
                 if (FieldAIPlaceAllBoats(&ownField) == SUCCESS) {
                     agentState = AGENT_STATE_CHALLENGING;
                     OLED_Clear(OLED_COLOR_BLACK);
                     char buf[96];
                     sprintf(buf, "CHALLENGING\n%u = A\n%u = hashA", A, hashA);
                     OLED_DrawString(buf);
                     OLED_Update();
 
                     HAL_Delay(100); 
                     OLED_Clear(OLED_COLOR_BLACK);
                     FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                 }
 
             // If opponent's challenge received
             } else if (event.type == BB_EVENT_CHA_RECEIVED) {
                 hashA = event.param0;
                 B = rand() % 65536;
                 messageToSend.type = MESSAGE_ACC;
                 messageToSend.param0 = B;
 
                 // Place boats and transition to ACCEPTING
                 if (FieldAIPlaceAllBoats(&ownField) == SUCCESS) {
                     agentState = AGENT_STATE_ACCEPTING;
                     printf("START -> ACCEPTING\n");
                     OLED_Clear(OLED_COLOR_BLACK);
                     char buf[96];
                     sprintf(buf, "ACCEPTING\n%u = hashA\n%u = B", hashA, B);
                     OLED_DrawString(buf);
                     OLED_Update();
 
                     HAL_Delay(100);
                     OLED_Clear(OLED_COLOR_BLACK);
                     FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                 }
             }
             break;
 
         // Challenging state: wait for acceptance and then determine turn
         case AGENT_STATE_CHALLENGING:
             if (event.type == BB_EVENT_ACC_RECEIVED) {
                 messageToSend.type = MESSAGE_REV;
                 messageToSend.param0 = A;
                 turn_order = NegotiateCoinFlip(A, event.param0);
 
                 // Set turn order and transition to respective state
                 if (turn_order == HEADS) {
                     OLED_Clear(OLED_COLOR_BLACK);
                     playerTurn = FIELD_OLED_TURN_MINE;
                     OLED_Update();
                     agentState = AGENT_STATE_WAITING_TO_SEND;
                     printf("CHALLENGING -> WAITING TO SEND\n");
                     FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                 } else {
                     OLED_Clear(OLED_COLOR_BLACK);
                     playerTurn = FIELD_OLED_TURN_THEIRS;
                     OLED_Update(); 
                     agentState = AGENT_STATE_DEFENDING;
                     printf("CHALLENGING -> DEFENDING\n");
                     FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                 }
             }
             break;
 
         // Accepting state: wait for REV and verify challenge integrity
         case AGENT_STATE_ACCEPTING:
             if (event.type == BB_EVENT_REV_RECEIVED) {
 
                 // Verification failed -> cheat detected
                 if (!NegotiationVerify(event.param0, hashA)) {
                     agentState = AGENT_STATE_END_SCREEN;
                     printf("ACCEPTING -> END SCREEN\n");
                     OLED_Clear(OLED_COLOR_BLACK);
                     OLED_DrawString("ERROR: Cheating Detected");
                     OLED_Update();
 
                 // Verification successful -> determine turn
                 } else {
                     turn_order = NegotiateCoinFlip(event.param1, B);
                     own_guess = FieldAIDecideGuess(&oppField);
                     messageToSend.type = MESSAGE_SHO;
                     messageToSend.param0 = own_guess.row;
                     messageToSend.param1 = own_guess.col;
 
                     if (turn_order == TAILS) {
                         OLED_Clear(OLED_COLOR_BLACK);
                         playerTurn = FIELD_OLED_TURN_MINE;
                         OLED_Update();
                         agentState = AGENT_STATE_ATTACKING;
                         printf("ACCEPTING -> ATTACKING\n");
                         FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                     } else {
                         OLED_Clear(OLED_COLOR_BLACK);
                         playerTurn = FIELD_OLED_TURN_THEIRS;
                         OLED_Update(); 
                         agentState = AGENT_STATE_DEFENDING;
                         printf("ACCEPTING -> DEFENDING\n");
                         FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                     }
                 }
             }
             break;
 
         // Attacking state: handle result of our attack
         case AGENT_STATE_ATTACKING:
             if (event.type == BB_EVENT_RES_RECEIVED) {
                 own_guess.result = event.param2;
                 FieldUpdateKnowledge(&oppField, &own_guess);
                 uint8_t oppState = FieldGetBoatStates(&oppField);
                 printf("DEBUG: Opponent boat state after update = %u\n", oppState);
 
                 FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                 OLED_Update(); 
 
                 // Check for victory
                 if (oppState == 0) {
                     OLED_Clear(OLED_COLOR_BLACK);
                     OLED_DrawString("VICTORY");
                     OLED_Update();
                     HAL_Delay(1000);
                     agentState = AGENT_STATE_END_SCREEN;
                     printf("ATTACKING -> END SCREEN\n");
                 } else {
                     agentState = AGENT_STATE_DEFENDING;
                     printf("ATTACKING -> DEFENDING\n");
                 }
             }
             break;
 
         // Defending state: handle incoming opponent attack
         case AGENT_STATE_DEFENDING:
             if (event.type == BB_EVENT_SHO_RECEIVED) {
                 GuessData incomingGuess;
                 incomingGuess.row = event.param0;
                 incomingGuess.col = event.param1;
                 incomingGuess.result = FieldRegisterEnemyAttack(&ownField, &incomingGuess);
 
                 messageToSend.type = MESSAGE_RES;
                 messageToSend.param0 = incomingGuess.row;
                 messageToSend.param1 = incomingGuess.col;
                 messageToSend.param2 = incomingGuess.result;
 
                 FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
                 OLED_Update(); 
 
                 // Check for defeat
                 if (FieldGetBoatStates(&ownField) == 0) {
                     agentState = AGENT_STATE_END_SCREEN;
                     printf("DEFENDING -> END SCREEN\n");
                 } else {
                     agentState = AGENT_STATE_WAITING_TO_SEND;
                     printf("DEFENDING -> WAITING TO SEND\n");
                 }
             }
             break;
 
         // Waiting to send state: send our next guess
         case AGENT_STATE_WAITING_TO_SEND:
             if (event.type == BB_EVENT_MESSAGE_SENT) {
                 turn_counter++;
                 own_guess = FieldAIDecideGuess(&oppField);
                 messageToSend.type = MESSAGE_SHO;
                 messageToSend.param0 = own_guess.row;
                 messageToSend.param1 = own_guess.col;
                 agentState = AGENT_STATE_ATTACKING;
                 printf("WAITING TO SEND -> ATTACKING\n");
                 FieldOledDrawScreen(&ownField, &oppField, playerTurn, turn_counter);
             }
             break;
 
         // End screen: display game result
         case AGENT_STATE_END_SCREEN: {
             if (!endScreenDrawn) {
                 OLED_Clear(OLED_COLOR_BLACK);
                 uint8_t oppState = FieldGetBoatStates(&oppField);
                 uint8_t ownState = FieldGetBoatStates(&ownField);
 
                 printf("DEBUG: Opponent boat state = %u\n", oppState);
                 printf("DEBUG: Own boat state = %u\n", ownState);
 
                 // Decide game outcome
                 if (oppState == 0 && ownState != 0) {
                     OLED_DrawString("VICTORY");
                     printf("GAME OVER: YOU WIN\n");
                 } else if (ownState == 0 && oppState != 0) {
                     OLED_DrawString("ALAS DEFEAT...");
                     printf("GAME OVER: YOU LOSE\n");
                 } else if (ownState == 0 && oppState == 0) {
                     OLED_DrawString("DRAW");
                     printf("GAME OVER: DRAW\n");
                 }
 
                 OLED_Update();
                 endScreenDrawn = true;
             }
             break;
         }
     }
 
     return messageToSend;
 }
 
 // Getter for Agent's current state
 AgentState AgentGetState(void) {
     return agentState;
 }
 
 // Setter for Agent's current state
 void AgentSetState(AgentState newState) {
     agentState = newState;
 }
 