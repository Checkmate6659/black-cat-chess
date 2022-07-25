#include "tt.h"


uint64_t prng_state = 0x12345678;
uint64_t zobrist_table[2048]; //NOTE: this Zobrist table is larger than it has to be (only 781 entries minimum)

uint64_t pseudo_rng()
{
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 7;
    prng_state ^= prng_state << 17;

    return prng_state;
}

void init_zobrist()
{
    //Initialize Zobrist table
    for (uint16_t i = 0; i < 2048; i++)
        zobrist_table[i] = pseudo_rng();
}

//The hash of a move (the hash of the position after making that move)
//WARNING: only call AFTER the move has been made!
uint64_t move_hash(MOVE move, MOVE_RESULT result)
{
    uint64_t hash = 0; //Cumulated hash

    //Remove source piece from its square, and place it onto the target square
    uint8_t source_piece = result.prev_state & 15;
    uint8_t target_piece = board[move.tgt] & 15;

    hash ^= zobrist_table[source_piece << 7 | move.src];
    hash ^= zobrist_table[target_piece << 7 | move.tgt];

    //Remove captured piece from the target square
    hash ^= zobrist_table[(result.piece & 15) << 7 | move.tgt];

    //TODO: do castling rights and en passant!

    return hash ^ Z_TURN;
}
