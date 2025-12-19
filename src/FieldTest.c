/**
 * @file    FieldTest.c
 *
 * @author  Akshera Paladhi
 *
 * @date    June 8, 2025
 */

 // Standard C headers
#include <stdint.h>     // For fixed-size integer types (e.g., uint8_t)
#include <stdlib.h>     // For standard library functions
#include <assert.h>     // For assert macro (not used in this file)
#include <stdio.h>      // For printf
#include <stdbool.h>    // For boolean types (true/false)

// Project headers
#include "Field.h"      // Declares Field structure and game-related functions
#include "BOARD.h"      // Project-specific initialization and support
#include "BattleBoats.h"// Defines constants like boat sizes and statuses

/**
 * FieldPrint_UART(*ownField, *oppField)
 * Optional helper function (not implemented here) for printing the game fields via UART.
 */
void FieldPrint_UART(Field* ownField, Field* oppField);

// ------------------------- FIELD INITIALIZATION TEST -------------------------

/**
 * Tests the FieldInit function for proper grid and boat lives initialization.
 */
void TestFieldInit() {
    Field own;  // Player's own field
    Field opp;  // Opponent's field

    printf("Running FieldInit tests...\n");

    FieldInit(&own, &opp);  // Initialize both fields

    // --- Test 1: Check own grid is empty ---
    int pass = 1;
    for (int row = 0; row < FIELD_ROWS; row++) {
        for (int col = 0; col < FIELD_COLS; col++) {
            if (own.grid[row][col] != FIELD_SQUARE_EMPTY) {
                // Fail if any square isn't empty
                printf("Test 1 Failed: own.grid[%d][%d] = %d (expected FIELD_SQUARE_EMPTY)\n", row, col, own.grid[row][col]);
                pass = 0;
            }
        }
    }
    if (pass) printf("Test 1 Passed: own field grid initialized to FIELD_SQUARE_EMPTY\n");

    // --- Test 2: Check opponent grid is UNKNOWN ---
    pass = 1;
    for (int row = 0; row < FIELD_ROWS; row++) {
        for (int col = 0; col < FIELD_COLS; col++) {
            if (opp.grid[row][col] != FIELD_SQUARE_UNKNOWN) {
                printf("Test 2 Failed: opp.grid[%d][%d] = %d (expected FIELD_SQUARE_UNKNOWN)\n", row, col, opp.grid[row][col]);
                pass = 0;
            }
        }
    }
    if (pass) printf("Test 2 Passed: opponent field grid initialized to FIELD_SQUARE_UNKNOWN\n");

    // --- Test 3: Own boat lives should be 0 ---
    if (own.smallBoatLives == 0 && own.mediumBoatLives == 0 &&
        own.largeBoatLives == 0 && own.hugeBoatLives == 0) {
        printf("Test 3 Passed: own field boat lives initialized to 0\n");
    }
    else {
        printf("Test 3 Failed: own boat lives incorrect: small=%d, medium=%d, large=%d, huge=%d\n",
            own.smallBoatLives, own.mediumBoatLives, own.largeBoatLives, own.hugeBoatLives);
    }

    // --- Test 4: Opponent boat lives should be full size ---
    if (opp.smallBoatLives == FIELD_BOAT_SIZE_SMALL &&
        opp.mediumBoatLives == FIELD_BOAT_SIZE_MEDIUM &&
        opp.largeBoatLives == FIELD_BOAT_SIZE_LARGE &&
        opp.hugeBoatLives == FIELD_BOAT_SIZE_HUGE) {
        printf("Test 4 Passed: opponent boat lives initialized to full sizes\n");
    }
    else {
        printf("Test 4 Failed: opponent boat lives incorrect: small=%d, medium=%d, large=%d, huge=%d\n",
            opp.smallBoatLives, opp.mediumBoatLives, opp.largeBoatLives, opp.hugeBoatLives);
    }

    printf("FieldInit tests complete.\n");
}

// ---------------------- FIELD SET SQUARE STATUS TEST ------------------------

/**
 * Tests FieldSetSquareStatus to set/get individual square statuses.
 */
void Test_FieldSetSquareStatus() {
    Field testField = { 0 };        // Initialize empty field
    uint8_t testRow = 2, testCol = 3;

    printf("Running FieldSetSquareStatus tests...\n");

    // --- Test 1: Set (2,3) to HIT from EMPTY ---
    testField.grid[testRow][testCol] = FIELD_SQUARE_EMPTY;
    SquareStatus returned = FieldSetSquareStatus(&testField, testRow, testCol, FIELD_SQUARE_HIT);
    if (returned == FIELD_SQUARE_EMPTY && testField.grid[testRow][testCol] == FIELD_SQUARE_HIT) {
        printf("Test 1 Passed: Valid set and return value.\n");
    }
    else {
        printf("Test 1 Failed: Expected return %d, got %d. Grid value is %d\n",
            FIELD_SQUARE_EMPTY, returned, testField.grid[testRow][testCol]);
    }

    // --- Test 2: Invalid row index (out-of-bounds) ---
    returned = FieldSetSquareStatus(&testField, FIELD_ROWS, testCol, FIELD_SQUARE_SMALL_BOAT);
    if (returned == FIELD_SQUARE_INVALID) {
        printf("Test 2 Passed: Out-of-bounds row handled.\n");
    }
    else {
        printf("Test 2 Failed: Expected FIELD_SQUARE_INVALID, got %d\n", returned);
    }

    // --- Test 3: Invalid column index ---
    returned = FieldSetSquareStatus(&testField, testRow, FIELD_COLS, FIELD_SQUARE_SMALL_BOAT);
    if (returned == FIELD_SQUARE_INVALID) {
        printf("Test 3 Passed: Out-of-bounds column handled.\n");
    }
    else {
        printf("Test 3 Failed: Expected FIELD_SQUARE_INVALID, got %d\n", returned);
    }

    // --- Test 4: Change square from HIT to CURSOR ---
    testField.grid[testRow][testCol] = FIELD_SQUARE_HIT;
    returned = FieldSetSquareStatus(&testField, testRow, testCol, FIELD_SQUARE_CURSOR);
    if (returned == FIELD_SQUARE_HIT && testField.grid[testRow][testCol] == FIELD_SQUARE_CURSOR) {
        printf("Test 4 Passed: Setting to CURSOR works.\n");
    }
    else {
        printf("Test 4 Failed: Expected return %d, got %d. Grid value is %d\n",
            FIELD_SQUARE_HIT, returned, testField.grid[testRow][testCol]);
    }

    printf("FieldSetSquareStatus tests complete.\n");
}

// -------------------------- FIELD ADD BOAT TEST -----------------------------

/**
 * Tests placing boats in valid and invalid positions.
 */
void TestFieldAddBoat() {
    Field f;
    Field g;
    FieldInit(&f, &g);  // Re-initialize field

    printf("Running FieldAddBoat tests...\n");

    // --- Test 1: Place small boat EAST at (0,0) ---
    // BoatType boat = FIELD_BOAT_TYPE_SMALL;
    int result = FieldAddBoat(&f, 0, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_SMALL);
    if (result == SUCCESS) {
        printf("Test 1 Passed: Small boat placed EAST at (0,0)\n");
    }
    else {
        printf("Test 1 Failed: Small boat could not be placed at (0,0)\n");
    }

    //--- Test 2: Place medium boat EAST at (1,0) ---
    result = FieldAddBoat(&f, 1, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_MEDIUM);
    if (result == SUCCESS) {
        printf("Test 2 Passed: Medium boat placed EAST at (1,0)\n");
    }
    else {
        printf("Test 2 Failed: Medium boat could not be placed at (1,0)\n");
    }

    // --- Test 3: Attempt overlapping boat at (0,0) going SOUTH ---
    result = FieldAddBoat(&f, 0, 0, FIELD_DIR_SOUTH, FIELD_BOAT_TYPE_LARGE);
    if (result == STANDARD_ERROR) {
        printf("Test 3 Passed: Overlapping large boat correctly rejected at (0,0)\n");
    }
    else {
        printf("Test 3 Failed: Overlapping large boat incorrectly allowed at (0,0)\n");
    }

    // --- Test 4: Place boat out of bounds (will not fit) ---
    result = FieldAddBoat(&f, 5, 8, FIELD_DIR_EAST, FIELD_BOAT_TYPE_MEDIUM);
    if (result == STANDARD_ERROR) {
        printf("Test 4 Passed: Boat placement out-of-bounds correctly rejected\n");
    }
    else {
        printf("Test 4 Failed: Out-of-bounds boat placement incorrectly allowed\n");
    }

    // --- Optional: Print lives to confirm ---
    printf("Boat lives (should be small=%d, medium=%d): small=%d, medium=%d, large=%d, huge=%d\n",
        FIELD_BOAT_SIZE_SMALL, FIELD_BOAT_SIZE_MEDIUM,
        f.smallBoatLives, f.mediumBoatLives, f.largeBoatLives, f.hugeBoatLives);

    printf("FieldAddBoat tests complete.\n");
}

// ----------------- FIELD REGISTER ENEMY ATTACK TEST -------------------------

/**
 * Tests FieldRegisterEnemyAttack for accuracy and state updates.
 */
void TestFieldRegisterEnemyAttack() {
    Field field;
    GuessData guess;

    FieldInit(&field, NULL);  // Init field

    // Place a small boat at (0,0) going EAST
    if (FieldAddBoat(&field, 0, 0, FIELD_DIR_EAST, FIELD_BOAT_TYPE_SMALL) != SUCCESS) {
        printf("Failed to add small boat at (0,0)\n");
        return;
    }

    printf("Running FieldRegisterEnemyAttack tests...\n");

    // --- Test 1: Attack a boat at (0,0) ---
    guess.row = 0;
    guess.col = 0;
    SquareStatus before = FieldRegisterEnemyAttack(&field, &guess);
    if (before == FIELD_SQUARE_SMALL_BOAT && guess.result == RESULT_HIT &&
        field.smallBoatLives == FIELD_BOAT_SIZE_SMALL - 1) {
        printf("Test 1 Passed: Hit on small boat at (0,0)\n");
    }
    else {
        printf("Test 1 Failed: Unexpected attack result or boat lives\n");
    }

    // --- Test 2: Attack water at (5,5) ---
    guess.row = 5;
    guess.col = 5;
    before = FieldRegisterEnemyAttack(&field, &guess);
    if (before == FIELD_SQUARE_EMPTY && guess.result == RESULT_MISS) {
        printf("Test 2 Passed: Miss at (5,5)\n");
    }
    else {
        printf("Test 2 Failed: Unexpected attack result at (5,5)\n");
    }

    // --- Test 3: Attack out-of-bounds ---
    guess.row = FIELD_ROWS;
    guess.col = FIELD_COLS;
    before = FieldRegisterEnemyAttack(&field, &guess);
    if (before == FIELD_SQUARE_INVALID) {
        printf("Test 3 Passed: Out-of-bounds attack handled\n");
    }
    else {
        printf("Test 3 Failed: Out-of-bounds attack not handled properly\n");
    }

    // --- Test 4: Sink remaining parts of small boat ---
    for (int col = 1; col < FIELD_BOAT_SIZE_SMALL; col++) {
        guess.row = 0;
        guess.col = col;
        FieldRegisterEnemyAttack(&field, &guess);
    }
    if (field.smallBoatLives == 0) {
        printf("Test 4 Passed: Small boat sunk correctly\n");
    }
    else {
        printf("Test 4 Failed: Small boat lives after sinking: %d\n", field.smallBoatLives);
    }

    printf("FieldRegisterEnemyAttack tests complete.\n");
}

// --------------------------- HELPER FUNCTION -------------------------------

/**
 * Helper function to print test result based on condition.
 */
void Check(bool condition, const char* testName) {
    if (condition) {
        printf(" %s passed\n", testName);
    }
    else {
        printf(" %s FAILED\n", testName);
    }
}

// -------------------- FIELD UPDATE KNOWLEDGE TEST ---------------------------

/**
 * Test FieldUpdateKnowledge based on GuessData.
 */
void TestFieldUpdateKnowledge() {
    Field field = { 0 };
    field.grid[2][3] = FIELD_SQUARE_UNKNOWN;
    field.smallBoatLives = 3;

    GuessData guess = { 2, 3, RESULT_SMALL_BOAT_SUNK };
    SquareStatus prev = FieldUpdateKnowledge(&field, &guess);

    Check(prev == FIELD_SQUARE_UNKNOWN, "FieldUpdateKnowledge prev square status check");
    Check(field.grid[2][3] == FIELD_SQUARE_HIT, "FieldUpdateKnowledge updated square check");
    Check(field.smallBoatLives == 0, "FieldUpdateKnowledge small boat lives check");
}

// --------------------- FIELD GET BOAT STATES TEST ---------------------------

/**
 * Tests FieldGetBoatStates return bitmask based on alive boats.
 */
void TestFieldGetBoatStates() {
    Field field = { 0 };
    field.smallBoatLives = 1;
    field.mediumBoatLives = 0;
    field.largeBoatLives = 2;
    field.hugeBoatLives = 0;

    uint8_t status = FieldGetBoatStates(&field);
    uint8_t expected = FIELD_BOAT_STATUS_SMALL | FIELD_BOAT_STATUS_LARGE;
    Check(status == expected, "FieldGetBoatStates status check");
}

// ---------------------- FIELD AI PLACE BOATS TEST ---------------------------

/**
 * Tests FieldAIPlaceAllBoats populates boats with correct lives.
 */
void TestFieldAIPlaceAllBoats() {
    Field field;
    Field f;

    FieldInit(&field, &f);

    uint8_t result = FieldAIPlaceAllBoats(&field);
    Check(result == SUCCESS, "FieldAIPlaceAllBoats result check");
    Check(field.smallBoatLives == FIELD_BOAT_SIZE_SMALL, "FieldAIPlaceAllBoats small boat lives check");
    Check(field.mediumBoatLives == FIELD_BOAT_SIZE_MEDIUM, "FieldAIPlaceAllBoats medium boat lives check");
    Check(field.largeBoatLives == FIELD_BOAT_SIZE_LARGE, "FieldAIPlaceAllBoats large boat lives check");
    Check(field.hugeBoatLives == FIELD_BOAT_SIZE_HUGE, "FieldAIPlaceAllBoats huge boat lives check");
}

// ----------------------- FIELD AI GUESS TEST --------------------------------

/**
 * Tests FieldAIDecideGuess selects valid unknown square.
 */
void TestFieldAIDecideGuess() {
    Field opp = { 0 };
    for (int r = 0; r < FIELD_ROWS; r++) {
        for (int c = 0; c < FIELD_COLS; c++) {
            opp.grid[r][c] = FIELD_SQUARE_UNKNOWN;
        }
    }

    GuessData guess = FieldAIDecideGuess(&opp);

    bool inBounds = (guess.row >= 0 && guess.row < FIELD_ROWS) &&
        (guess.col >= 0 && guess.col < FIELD_COLS);
    bool isUnknown = (opp.grid[guess.row][guess.col] == FIELD_SQUARE_UNKNOWN);

    Check(inBounds, "FieldAIDecideGuess guess in bounds");
    Check(isUnknown, "FieldAIDecideGuess guess is unknown square");
}

// ------------------------------ MAIN FUNCTION -------------------------------

/**
 * Main test entry point.
 */
int main(void) {
    BOARD_Init();  // Initialize the system board (from BOARD.h)

    HAL_Delay(1000);

    printf("\n=== Battleship Field Tests ===\n\n");

    // Run all test suites
    TestFieldInit();
    Test_FieldSetSquareStatus();
    TestFieldAddBoat();
    TestFieldRegisterEnemyAttack();
    TestFieldUpdateKnowledge();
    TestFieldGetBoatStates();
    TestFieldAIPlaceAllBoats();
    TestFieldAIDecideGuess();

    printf("\n=== All tests finished ===\n");

    return 0;
}

