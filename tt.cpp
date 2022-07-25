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
