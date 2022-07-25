#ifndef __TT_H__
#define __TT_H__

#include "board.h"


#define Z_TURN (zobrist_table[0])

extern uint64_t prng_state; //PRNG state
extern uint64_t zobrist_table[2048]; //Zobrist table (uses [piece][square] indexing, similar to history table)

uint64_t pseudo_rng64(); //64-bit pseudo-random number generation (Xorshift64)
void init_zobrist();

uint64_t move_hash(MOVE move, MOVE_RESULT result); //The hash of a move (the hash of the position after making that move)

#endif
