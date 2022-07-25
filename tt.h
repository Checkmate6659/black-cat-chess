#ifndef __TT_H__
#define __TT_H__

#include "board.h"


//TT size (default is 1 << 24 = 16777216)
#define TT_SIZE ((1 << 28) / sizeof(TT_ENTRY))

#define HF_EXACT 1
#define HF_BETA 2
#define HF_ALPHA 4

#define Z_TURN (zobrist_table[129]) //b8 entry on black WPAWN board (unused, since white pawns are not black)
#define Z_CRL(sq) (zobrist_table[(sq) | 128]) //black WPAWN board (unused); only uses corners of the board


typedef uint32_t TT_INDEX; //Change to uint64_t on large TT sizes (above ~28GB)

typedef struct {
    uint64_t key; //The position key
    uint8_t flag; //A flag to identify the score as exact, alpha or beta
    uint8_t depth; //The depth of the entry
    int16_t eval; //The evaluation of the position
    uint16_t move; //The best move of the previous search
} TT_ENTRY;

extern TT_ENTRY transpo_table[];


extern uint64_t prng_state; //PRNG state
extern uint64_t zobrist_table[]; //Zobrist table (uses [piece][square] indexing, similar to history table)
extern uint64_t tt_entries; //TEMP VARIABLE: to debug TT

uint64_t pseudo_rng64(); //64-bit pseudo-random number generation (Xorshift64)
void init_zobrist();
void clear_tt();


//Get a TT entry (if at or above required depth)
inline TT_ENTRY get_entry(uint64_t key)
{
    TT_INDEX tt_index = key % TT_SIZE;
    TT_ENTRY entry = transpo_table[tt_index];

    if (entry.key == key)
        return entry; //Hit!

    return TT_ENTRY { 0, 0, 0, 0, 0 }; //return an invalid entry (flag = 0)
}

//Set a TT entry (if improving depth)
inline void set_entry(uint64_t key, uint8_t flag, uint8_t depth, int16_t eval, MOVE move)
{
    TT_INDEX tt_index = key % TT_SIZE;
    TT_ENTRY entry = transpo_table[tt_index];

    //insufficient depth
    if (entry.depth > depth)
        return;
    
    //less precise flag (exact < upperbound < lowerbound)
    if (entry.flag && entry.flag < flag) //TODO: try inverting values of alpha and beta
        return;

    if (!entry.flag) //new entry
        tt_entries++;

    transpo_table[tt_index].key = key;
    transpo_table[tt_index].flag = flag;
    transpo_table[tt_index].depth = depth;
    transpo_table[tt_index].eval = eval;
    transpo_table[tt_index].move = MOVE_ID(move);
}

//A function that helps determine the hash of a move (aka the change in board hash it generates)
//Xoring the results before and after a move gives you the move hash ^ Z_TURN
inline uint64_t move_hash(MOVE move)
{
    uint8_t source_piece = board[move.src] & 15;
    uint8_t target_piece = board[move.tgt] & 15;
    uint16_t special_move_index_1 = 0;
    uint16_t special_move_index_2 = 0;
    uint64_t crl = 0;

    //No risk of overflow here! So we *can* use these offsetted squares without any risk
    if (move.flags & F_EP)
    {
        uint8_t special_square_1 = move.tgt - 16;
        uint8_t special_square_2 = move.tgt + 16;

        special_move_index_1 = (board[special_square_1] & 15) << 7 | special_square_1;
        special_move_index_2 = (board[special_square_2] & 15) << 7 | special_square_2;
    }
    else if (move.flags & F_CASTLE)
    {
        uint8_t special_square_1 = move.tgt - 1;
        uint8_t special_square_2 = move.tgt + 1;

        special_move_index_1 = (board[special_square_1] & 15) << 7 | special_square_1;
        special_move_index_2 = (board[special_square_2] & 15) << 7 | special_square_2;
    }

    //also do hashing for loss of castle rights (king/rook first move)
    //NOTE: after the piece moves, the piece will have the MOVED bit set, so the conditions would always fail
    if ((board[move.src] & (PTYPE | MOVED)) == KING) crl = Z_CRL(move.src);
    else if ((board[move.src] & (PTYPE | MOVED)) == ROOK) crl = Z_CRL(move.src);

    return
        zobrist_table[source_piece << 7 | move.src] ^ //Piece move
        zobrist_table[target_piece << 7 | move.tgt] ^
        zobrist_table[special_move_index_1] ^ //Special moves (castling/en passant)
        zobrist_table[special_move_index_2] ^
        crl; //Loss of a castling right
}

#endif
