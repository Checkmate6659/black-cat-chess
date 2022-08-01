#include "tt.h"


TT_ENTRY transpo_table[TT_SIZE]; //Transposition table

uint64_t prng_state = 0x12345678; //PRNG seed (not randomized, to give constant results and make debug easier)
uint64_t zobrist_table[2048] = { 0 }; //NOTE: this Zobrist table is larger than it has to be (only 781 entries minimum)

uint64_t pseudo_rng()
{
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 7;
    prng_state ^= prng_state << 17;

    return prng_state;
}

void init_zobrist()
{
    //Initialize Zobrist table: fill up with random, except for first 8x8x2 board (empty squares)
    for (uint16_t i = 128; i < 2048; i++)
        zobrist_table[i] = pseudo_rng();
}

void clear_tt()
{
    for (TT_INDEX i = 0; i < TT_SIZE; i++)
        transpo_table[i].flag = 0; //flag of 0 means invalid entry: make entire table invalid
}

//No recursion: this function is guaranteed to produce the same result every single time (unless the board is different)
uint64_t board_hash(uint8_t stm, uint8_t last_target)
{
    uint64_t hash = 0;
    if (stm & BLACK) hash = Z_TURN; //turn hash: black to move
    hash ^= Z_DPP(last_target); //if there is a possibility of en passant, add it to the hash

    for (uint8_t i = 0; i < 120; i++)
    {
        if (i & OFFBOARD) continue; //off the board square!

        uint8_t raw_piece = board[i];
        uint8_t piece = raw_piece & 15; //piece and color info
        hash ^= zobrist_table[(piece << 7) | i]; //hash with the corresponding table index

        uint8_t king_status = board[plist[(stm & 16) ^ 16]];
        if ((raw_piece & (PTYPE | MOVED)) == ROOK && !(king_status & MOVED)) //has 1 or 2 castling rights: rook hasn't moved, and its king also hasn't moved
            hash ^= Z_CASTLE(i);
    }

    return hash;
}
