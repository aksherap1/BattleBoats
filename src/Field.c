/**
 * @file    Field.c
 *
 * @author  Caitelyn Huang (chuan188@ucsc.edu)
 *
 * @date    1 May 2025
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "Field.h"
#include "BOARD.h"

/*  MODULE-LEVEL DEFINITIONS, MACROS    */

/**
 * Define the dimensions of the game field. They can be overridden by
 * compile-time specifications. All references to the dimensions of the field
 * should use these constants instead of hard-coding a numeric value so that the
 * field dimensions can be changed with minimal coding changes.
 */
#define FIELD_COLS 10
#define FIELD_ROWS 6

/**
 * Specify how many boats there exist on the field. There is 1 boat of each of
 * the 4 types, so 4 total.
 */
#define FIELD_NUM_BOATS 4

/*  PROTOTYPES  */

/** FieldPrint_UART(*ownField, *oppField)
 *
 * This function is optional, but recommended. It prints a representation o both
 * fields, similar to the OLED display.
 *
 * @param   f   The field to initialize.
 * @param   p   The data to initialize the entire field to, should be a member
 *              of enum SquareStatus.
 */
void FieldPrint_UART(Field *ownField, Field *oppField);

/** FieldInit(*ownField, *oppField)
 *
 * FieldInit() initializes two passed Field structs for the beginning of play.
 * Each field's grid should be filled with the appropriate SquareStatus (
 * FIELD_SQUARE_EMPTY for your own field, FIELD_SQUARE_UNKNOWN for opponent's).
 * Additionally, your opponent's field's boatLives parameters should be filled
 *  (your own field's boatLives will be filled when boats are added).
 *
 * FieldAI_PlaceAllBoats() should NOT be called in this function.
 *
 * For the enemy Field, we start by giving them full lives.
 * For our Field (which does not yet have boats placed on it), we set boat
 * lives on boat placement.
 *
 * @param   *ownField   A field representing the agents own ships.
 * @param   *oppField   A field representing the opponent's ships.
 */
void FieldInit(Field *ownField, Field *oppField)
{
    for (uint8_t row = 0; row < FIELD_ROWS; row++)
    {
        for (uint8_t col = 0; col < FIELD_COLS; col++)
        {
            ownField->grid[row][col] = FIELD_SQUARE_EMPTY;
            oppField->grid[row][col] = FIELD_SQUARE_UNKNOWN;
        }
    }

    // Player's boat lives initialized to 0 (set later upon placement)
    ownField->smallBoatLives = 0;
    ownField->mediumBoatLives = 0;
    ownField->largeBoatLives = 0;
    ownField->hugeBoatLives = 0;

    // Opponent's boat lives set to max
    oppField->smallBoatLives = FIELD_BOAT_SIZE_SMALL;
    oppField->mediumBoatLives = FIELD_BOAT_SIZE_MEDIUM;
    oppField->largeBoatLives = FIELD_BOAT_SIZE_LARGE;
    oppField->hugeBoatLives = FIELD_BOAT_SIZE_HUGE;
}

/** FieldGetSquareStatus(*f, row, col)
 *
 * Retrieves the value at the specified field position.
 *
 * @param   *f  The Field being referenced.
 * @param   row The row-component of the location to retrieve
 * @param   col The column-component of the location to retrieve
 * @return  FIELD_SQUARE_INVALID if row and col are not valid field locations
 *          Otherwise, return the status of the referenced square.
 */
SquareStatus FieldGetSquareStatus(const Field *f, uint8_t row, uint8_t col)
{
    if (row >= FIELD_ROWS || col >= FIELD_COLS)
    {
        return FIELD_SQUARE_INVALID;
    }
    return (SquareStatus)f->grid[row][col];
}

/** FieldSetSquareStatus(*f, row, col, p)
 *
 * This function provides an interface for setting individual locations within a
 * Field struct. This is useful when FieldAddBoat() doesn't do exactly what you
 * need. For example, if you'd like to use FIELD_SQUARE_CURSOR, this is the
 * function to use.
 *
 * @param   *f  The Field to modify.
 * @param   row The row-component of the location to modify.
 * @param   col The column-component of the location to modify.
 * @param   p   The new value of the field location.
 * @return  The old value at that field location.
 */
SquareStatus FieldSetSquareStatus(
    Field *f,
    uint8_t row,
    uint8_t col,
    SquareStatus p)
{
    if (row >= FIELD_ROWS || col >= FIELD_COLS)
    {
        return FIELD_SQUARE_INVALID;
    }
    SquareStatus oldStatus = (SquareStatus)f->grid[row][col];
    f->grid[row][col] = p;
    return oldStatus;
}

/** FieldAddBoat(*ownField, row, col, dir, boatType)
 *
 * FieldAddBoat() places a single ship on the player's field based on arguments
 * 2-5. Arguments 2, 3 represent the x, y coordinates of the pivot point of the
 * ship.  Argument 4 represents the * direction of the ship, and argument 5 is
 * the length of the ship being placed.
 *
 * All spaces that the boat would occupy are checked to be clear before the
 * field is modified so that if the boat can fit in the desired position, the
 * field is modified and SUCCESS is returned. Otherwise the field is unmodified
 * and STANDARD_ERROR is returned. There is no hard-coded limit to how many
 * times a boat can be added to a field within this function.
 *
 * In addition, this function should update the appropriate boatLives parameter
 * of the field.
 *
 * So this is valid test code:
 * {
 *   Field myField;
 *   FieldInit(&myField, FIELD_SQUARE_EMPTY);
 *   FieldAddBoat(&myField, 0, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_SMALL);
 *   FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_MEDIUM);
 *   FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_TYPE_HUGE);
 *   FieldAddBoat(&myField, 0, 6, FIELD_BOAT_DIRECTION_SOUTH, FIELD_BOAT_TYPE_SMALL);
 * }
 *
 * should result in a field like:
 *      0 1 2 3 4 5 6 7 8 9
 *     ---------------------
 *  0 [ 1 1 1 . . . 1 . . . ]
 *  1 [ 2 2 2 2 . . 1 . . . ]
 *  2 [ . . . . . . 1 . . . ]
 *  3 [ . . . . . . . . . . ]
 *  4 [ . . . . . . . . . . ]
 *  5 [ . . . . . . . . . . ]
 *
 * @param   f           The field to grab data from.
 * @param   row         The row that the boat will start from, valid range is
 *                      from 0 and to FIELD_ROWS - 1.
 * @param   col         The column that the boat will start from, valid range is
 *                      from 0 and to FIELD_COLS - 1.
 * @param   dir         The direction that the boat will face once places, from
 *                      the BoatDirection enum.
 * @param   boatType    The type of boat to place. Relies on the
 *                      FIELD_SQUARE_*_BOAT values from the SquareStatus enum.
 * @return  SUCCESS for success, STANDARD_ERROR for failure.
 */
uint8_t FieldAddBoat(Field *ownField, uint8_t row, uint8_t col,
                     BoatDirection dir, BoatType boatType)
{
    uint8_t length;
    SquareStatus boatStatus;
    // printf("%u\n", boatType);
    switch (boatType)
    {
    case FIELD_BOAT_TYPE_SMALL:
        // printf("small\n");
        length = FIELD_BOAT_SIZE_SMALL;
        boatStatus = FIELD_SQUARE_SMALL_BOAT;
        break;
    case FIELD_BOAT_TYPE_MEDIUM:
        length = FIELD_BOAT_SIZE_MEDIUM;
        boatStatus = FIELD_SQUARE_MEDIUM_BOAT;
        // printf("medium\n");
        break;
    case FIELD_BOAT_TYPE_LARGE:
        length = FIELD_BOAT_SIZE_LARGE;
        boatStatus = FIELD_SQUARE_LARGE_BOAT;
        // printf("large\n");
        break;
    case FIELD_BOAT_TYPE_HUGE:
        length = FIELD_BOAT_SIZE_HUGE;
        boatStatus = FIELD_SQUARE_HUGE_BOAT;
        // printf("huge\n");
        break;
    default:
        return STANDARD_ERROR;
    }

    // Check boundaries
    if ((dir == FIELD_DIR_EAST && (col + length > FIELD_COLS)) ||
        (dir == FIELD_DIR_SOUTH && (row + length > FIELD_ROWS)))
    {
        // printf("a\n");
        return STANDARD_ERROR;
    }

    // Check overlap
    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t r = row + ((dir == FIELD_DIR_SOUTH) ? i : 0);
        uint8_t c = col + ((dir == FIELD_DIR_EAST) ? i : 0);
        if (ownField->grid[r][c] != FIELD_SQUARE_EMPTY)
        {
            // printf("b\n");
            return STANDARD_ERROR;
        }
    }

    // Place boat squares
    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t r = row + ((dir == FIELD_DIR_SOUTH) ? i : 0);
        uint8_t c = col + ((dir == FIELD_DIR_EAST) ? i : 0);
        ownField->grid[r][c] = boatStatus;
    }

    // Increment boat lives (supports multiple boats of the same type)
    switch (boatType)
    {
    case FIELD_BOAT_TYPE_SMALL:
        ownField->smallBoatLives += length;
        break;
    case FIELD_BOAT_TYPE_MEDIUM:
        ownField->mediumBoatLives += length;
        break;
    case FIELD_BOAT_TYPE_LARGE:
        ownField->largeBoatLives += length;
        break;
    case FIELD_BOAT_TYPE_HUGE:
        ownField->hugeBoatLives += length;
        break;
    }
    // printf("succeeding\n");
    return SUCCESS;
}

/** FieldRegisterEnemyAttack(*ownField, *opp_guess)
 *
 * This function registers an attack at the gData coordinates on the provided
 * field. This means that 'f' is updated with a FIELD_SQUARE_HIT or
 * FIELD_SQUARE_MISS depending on what was at the coordinates indicated in
 * 'gData'. 'gData' is also updated with the proper HitStatus value depending on
 * what happened AND the value of that field position BEFORE it was attacked.
 * Finally this function also reduces the lives for any boat that was hit from
 * this attack.
 *
 * @param   f       The field to check against and update.
 * @param   gData   The coordinates that were guessed. The result is stored in
 *                  gData->result as an output.  The result can be a RESULT_HIT,
 *                  RESULT_MISS, or RESULT_***_SUNK.
 * @return  The data that was stored at the field position indicated by gData
 *          before this attack.
 */
SquareStatus FieldRegisterEnemyAttack(Field *ownField, GuessData *opp_guess)
{
    uint8_t row = opp_guess->row;
    uint8_t col = opp_guess->col;

    // Out-of-bounds attack
    if (row >= FIELD_ROWS || col >= FIELD_COLS)
    {
        opp_guess->result = RESULT_MISS;
        return FIELD_SQUARE_INVALID;
    }

    SquareStatus current = ownField->grid[row][col];

    switch (current)
    {
    case FIELD_SQUARE_SMALL_BOAT:
        ownField->grid[row][col] = FIELD_SQUARE_HIT;
        if (ownField->smallBoatLives > 0)
            ownField->smallBoatLives--;
        opp_guess->result = (ownField->smallBoatLives == 0) ? RESULT_SMALL_BOAT_SUNK : RESULT_HIT;
        break;

    case FIELD_SQUARE_MEDIUM_BOAT:
        ownField->grid[row][col] = FIELD_SQUARE_HIT;
        if (ownField->mediumBoatLives > 0)
            ownField->mediumBoatLives--;
        opp_guess->result = (ownField->mediumBoatLives == 0) ? RESULT_MEDIUM_BOAT_SUNK : RESULT_HIT;
        break;

    case FIELD_SQUARE_LARGE_BOAT:
        ownField->grid[row][col] = FIELD_SQUARE_HIT;
        if (ownField->largeBoatLives > 0)
            ownField->largeBoatLives--;
        opp_guess->result = (ownField->largeBoatLives == 0) ? RESULT_LARGE_BOAT_SUNK : RESULT_HIT;
        break;

    case FIELD_SQUARE_HUGE_BOAT:
        ownField->grid[row][col] = FIELD_SQUARE_HIT;
        if (ownField->hugeBoatLives > 0)
            ownField->hugeBoatLives--;
        opp_guess->result = (ownField->hugeBoatLives == 0) ? RESULT_HUGE_BOAT_SUNK : RESULT_HIT;
        break;

    case FIELD_SQUARE_EMPTY:
        ownField->grid[row][col] = FIELD_SQUARE_MISS;
        opp_guess->result = RESULT_MISS;
        break;

    case FIELD_SQUARE_HIT:
    case FIELD_SQUARE_MISS:
    default:
        // Already attacked this position or invalid, count as miss
        opp_guess->result = RESULT_MISS;
        break;
    }

    return current;
}

/** FieldUpdateKnowledge(*oppField, *own_guess)
 *
 * This function updates the FieldState representing the opponent's game board
 * with whether the guess indicated within gData was a hit or not. If it was a
 * hit, then the field is updated with a FIELD_SQUARE_HIT at that position. If
 * it was a miss, display a FIELD_SQUARE_EMPTY instead, as it is now known that
 * there was no boat there. The FieldState struct also contains data on how many
 * lives each ship has. Each hit only reports if it was a hit on any boat or if
 * a specific boat was sunk, this function also clears a boats lives if it
 * detects that the hit was a RESULT_*_BOAT_SUNK.
 *
 * @param   *f      The field to grab data from.
 * @param   gData   The coordinates that were guessed along with their
 *                  HitStatus.
 * @return  The previous value of that coordinate position in the field before
 *          the hit/miss was registered.
 */
/**
 * FieldUpdateKnowledge(*oppField, *own_guess)
 *
 * This function updates the FieldState representing the opponent's game board
 * with whether the guess indicated within gData was a hit or not. If it was a
 * hit, then the field is updated with a FIELD_SQUARE_HIT at that position. If
 * it was a miss, display a FIELD_SQUARE_EMPTY instead, as it is now known that
 * there was no boat there. The FieldState struct also contains data on how many
 * lives each ship has. Each hit only reports if it was a hit on any boat or if
 * a specific boat was sunk, this function also clears a boat’s lives if it
 * detects that the hit was a RESULT_*_BOAT_SUNK.
 *
 * @param   *oppField  The field to update (your knowledge of the opponent's board).
 * @param   *own_guess The coordinates that were guessed along with their
 *                     HitStatus.
 * @return  The previous value of that coordinate position in the field before
 *          the hit/miss was registered.
 */
SquareStatus FieldUpdateKnowledge(Field *oppField, const GuessData *own_guess)
{
    uint8_t row = own_guess->row;
    uint8_t col = own_guess->col;

    if (row >= FIELD_ROWS || col >= FIELD_COLS)
    {
        return FIELD_SQUARE_INVALID;
    }

    // Save the previous status to return it
    SquareStatus prevStatus = (SquareStatus)oppField->grid[row][col];

    // Always update the grid for a known hit or miss
    switch (own_guess->result)
    {
    case RESULT_HIT:
    case RESULT_SMALL_BOAT_SUNK:
    case RESULT_MEDIUM_BOAT_SUNK:
    case RESULT_LARGE_BOAT_SUNK:
    case RESULT_HUGE_BOAT_SUNK:
        oppField->grid[row][col] = FIELD_SQUARE_HIT;
        break;

    case RESULT_MISS:
        oppField->grid[row][col] = FIELD_SQUARE_MISS;
        break;

    default:
        // No update
        break;
    }

    // Independently update boat lives based on result
    switch (own_guess->result)
    {
    case RESULT_SMALL_BOAT_SUNK:
        oppField->smallBoatLives = 0;
        break;

    case RESULT_MEDIUM_BOAT_SUNK:
        oppField->mediumBoatLives = 0;
        break;

    case RESULT_LARGE_BOAT_SUNK:
        oppField->largeBoatLives = 0;
        break;

    case RESULT_HUGE_BOAT_SUNK:
        oppField->hugeBoatLives = 0;
        break;

    default:
        break;
    }

    return prevStatus;
}

/** FieldGetBoatStates(*f)
 *
 * This function returns the alive states of all 4 boats as a 4-bit bitfield
 * (stored as a uint8). The boats are ordered from smallest to largest starting
 * at the least-significant bit. So that: 0b00001010 indicates that the small
 * boat and large boat are sunk, while the medium and huge boat are still alive.
 *
 * See the BoatStatus enum for the bit arrangement.
 *
 * @param   *f  The field to grab data from.
 * @return  A 4-bit value with each bit corresponding to whether each ship is
 *          alive or not.
 */
uint8_t FieldGetBoatStates(const Field *f)
{
    uint8_t status = 0;

    if (f->smallBoatLives > 0)
    {
        status |= FIELD_BOAT_STATUS_SMALL;
    }
    if (f->mediumBoatLives > 0)
    {
        status |= FIELD_BOAT_STATUS_MEDIUM;
    }
    if (f->largeBoatLives > 0)
    {
        status |= FIELD_BOAT_STATUS_LARGE;
    }
    if (f->hugeBoatLives > 0)
    {
        status |= FIELD_BOAT_STATUS_HUGE;
    }

    return status;
}

/** FieldAIPlaceAllBoats(*ownField)
 *
 * This function is responsible for placing all four of the boats on a field.
 *
 * @param   f   Agent's own field, to be modified in place.
 * @return  SUCCESS if all boats could be placed, STANDARD_ERROR otherwise.
 *
 * This function should never fail when passed a properly initialized field!
 */
uint8_t FieldAIPlaceAllBoats(Field *ownField)
{
    BoatType boatTypes[] = {
        FIELD_BOAT_TYPE_HUGE,
        FIELD_BOAT_TYPE_LARGE,
        FIELD_BOAT_TYPE_MEDIUM,
        FIELD_BOAT_TYPE_SMALL};

    for (uint8_t i = 0; i < FIELD_NUM_BOATS; i++)
    {
        // uint8_t placed = 0;
        // while (!placed)
        // {
        //     uint8_t row = rand() % FIELD_ROWS;
        //     uint8_t col = rand() % FIELD_COLS;
        //     BoatDirection dir = (rand() % 2 == 0) ? FIELD_DIR_SOUTH : FIELD_DIR_EAST;

        //     if (FieldAddBoat(ownField, row, col, dir, boatTypes[i]) == SUCCESS)
        //     {
        //         placed = 1;
        //     }
        // }

        uint8_t row = rand() % FIELD_ROWS;
        uint8_t col = rand() % FIELD_COLS;
        BoatDirection dir = (rand() % 2 == 0) ? FIELD_DIR_SOUTH : FIELD_DIR_EAST;

        while (!(FieldAddBoat(ownField, row, col, dir, boatTypes[i])))
        {
            row = rand() % FIELD_ROWS;
            col = rand() % FIELD_COLS;
            dir = (rand() % 2 == 0) ? FIELD_DIR_SOUTH : FIELD_DIR_EAST;
        }
    }

    return SUCCESS;
}

/** FieldAIDecideGuess(*oppField)
 *
 * Given a field, decide the next guess.
 *
 * This function should not attempt to shoot a square which has already been
 * guessed.
 *
 * You may wish to give this function static variables.  If so, that data should
 * be reset when FieldInit() is called.
 *
 * @param   *f  An opponent's field.
 * @return  A GuessData struct whose row and col parameters are the coordinates
 *          of the guess.  The result parameter is irrelevant.
 */
GuessData FieldAIDecideGuess(const Field *oppField)
{
    static uint8_t targeting = 0;
    static uint8_t orientationKnown = 0; // 0 = unknown, 1 = horiz, 2 = vert
    static int8_t direction = -1;        // 0=N,1=S,2=W,3=E
    static uint8_t reverse = 0;
    static uint8_t hitsInARow = 0;

    static uint8_t originRow = FIELD_ROWS;
    static uint8_t originCol = FIELD_COLS;

    static GuessData lastGuess = {.result = RESULT_MISS, .row = 0, .col = 0};

    static int8_t directions[4][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1} // N, S, W, E
    };

    GuessData guess;
    guess.result = RESULT_MISS;

    // If last result was a sunk boat, reset targeting mode
    if (lastGuess.result >= RESULT_SMALL_BOAT_SUNK &&
        lastGuess.result <= RESULT_HUGE_BOAT_SUNK)
    {
        targeting = 0;
        orientationKnown = 0;
        direction = -1;
        reverse = 0;
        hitsInARow = 0;
        originRow = FIELD_ROWS;
        originCol = FIELD_COLS;
    }

    // If last result was a hit (and not a sunk boat), enter or continue targeting
    if (lastGuess.result == RESULT_HIT)
    {
        if (!targeting)
        {
            targeting = 1;
            originRow = lastGuess.row;
            originCol = lastGuess.col;
        }
        hitsInARow++;
    }

    // ---------- Target Mode ----------
    if (targeting)
    {
        if (orientationKnown && direction != -1)
        {
            // Continue in known direction
            int8_t d = reverse ? (direction ^ 1) : direction; // flip direction if reversed
            int8_t newRow = originRow + directions[d][0] * (hitsInARow + (reverse ? 0 : 1));
            int8_t newCol = originCol + directions[d][1] * (hitsInARow + (reverse ? 0 : 1));
            if (newRow < FIELD_ROWS && newCol < FIELD_COLS &&
                oppField->grid[newRow][newCol] == FIELD_SQUARE_UNKNOWN)
            {
                guess.row = newRow;
                guess.col = newCol;
                return guess;
            }
            else if (!reverse)
            {
                // Try reversing direction
                reverse = 1;
                return FieldAIDecideGuess(oppField); // retry with reversed
            }
            else
            {
                // Reset if both directions failed
                targeting = 0;
                orientationKnown = 0;
                direction = -1;
                reverse = 0;
                hitsInARow = 0;
            }
        }
        else
        {
            // Orientation unknown: try all adjacent unknowns
            for (int8_t d = 0; d < 4; d++)
            {
                int8_t newRow = originRow + directions[d][0];
                int8_t newCol = originCol + directions[d][1];
                if (newRow >= 0 && newRow < FIELD_ROWS &&
                    newCol >= 0 && newCol < FIELD_COLS &&
                    oppField->grid[newRow][newCol] == FIELD_SQUARE_UNKNOWN)
                {
                    guess.row = newRow;
                    guess.col = newCol;

                    // If this hit confirms orientation later
                    if (d == 0 || d == 1)
                        orientationKnown = 2; // vertical
                    else
                        orientationKnown = 1; // horizontal

                    direction = d;
                    return guess;
                }
            }

            // If no adjacent options, fall back to hunt mode
            targeting = 0;
        }
    }

    // ---------- Hunt Mode ----------
    for (uint8_t row = 0; row < FIELD_ROWS; row++)
    {
        for (uint8_t col = 0; col < FIELD_COLS; col++)
        {
            // Smarter parity (skip 75% of squares for faster search)
            if ((row + col) % 4 == 0 && oppField->grid[row][col] == FIELD_SQUARE_UNKNOWN)
            {
                guess.row = row;
                guess.col = col;
                return guess;
            }
        }
    }

    // ---------- Fallback ----------
    for (uint8_t row = 0; row < FIELD_ROWS; row++)
    {
        for (uint8_t col = 0; col < FIELD_COLS; col++)
        {
            if (oppField->grid[row][col] == FIELD_SQUARE_UNKNOWN)
            {
                guess.row = row;
                guess.col = col;
                return guess;
            }
        }
    }

    return guess;
}

/************************************************************
 * FOR EXTRA CREDIT:  Make the two "AI" functions above     *
 * smart enough to beat our AI in more than 55% of games.   *
 ************************************************************/