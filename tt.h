//TODO: try use weights from http://hgm.nubati.net/book_format.html (low collision rate)

#ifndef __TT_H__
#define __TT_H__

#include "board.h"
#include "eval.h" //mate scores (TODO)


//TT size (default is 1 << 24 = 16777216)
#define TT_SIZE ((1 << 28) / sizeof(TT_ENTRY))
// #define TT_SIZE (1 << 24) //power of 2 TT size

#define HF_EXACT 1
#define HF_BETA 2
#define HF_ALPHA 4

#define Z_TURN (zobrist_table[129]) //b8 entry on black WPAWN board (unused, since white pawns are not black)
#define Z_CRL(sq) (zobrist_table[(sq) | 128]) //black WPAWN board (unused); only uses corners of the board
#define Z_DPP(lt) ((lt == (uint8_t)-2) ? 0 : (zobrist_table[(lt) | 128])) //black WPAWN board (unused); uses central 2 rows of the board

#define REPLACEMENT_SCHEME(depth, entry_depth) ((uint16_t)(depth) * REPLACEMENT_DEN >= (uint16_t)(entry_depth) * REPLACEMENT_NUM) //only replace when depth * DEN > entry_depth * NUM
#define REPLACEMENT_NUM 2 //replacement scheme numerator
#define REPLACEMENT_DEN 3 //replacement scheme denominator


typedef uint32_t TT_INDEX; //Change to uint64_t on large TT sizes (above ~28GB)

typedef struct {
    uint64_t key; //The position key
    uint8_t flag; //A flag to identify the score as exact, alpha or beta
    uint8_t depth; //The depth of the entry
    int16_t eval; //The evaluation of the position
    uint16_t move; //The best move of the previous search
} TT_ENTRY;

extern TT_ENTRY transpo_table[TT_SIZE];


extern uint64_t prng_state; //PRNG state
extern uint64_t zobrist_table[]; //Zobrist table (uses [piece][square] indexing, similar to history table)

uint64_t pseudo_rng64(); //64-bit pseudo-random number generation (Xorshift64)
void init_zobrist();
void clear_tt();
uint64_t board_hash(uint8_t stm, uint8_t last_target); //DEBUG: returns a hash of the board


//Get a TT entry (if at or above required depth)
inline TT_ENTRY get_entry(uint64_t key)
{
    // TT_INDEX tt_index = key & (TT_SIZE - 1);
    TT_INDEX tt_index = key % TT_SIZE;
    TT_ENTRY entry = transpo_table[tt_index];

    // assert(tt_index < TT_SIZE);
    // if (entry.key == key && entry.flag)
    //     if (entry.eval %5 != 0 && !IS_MATE(entry.eval)) printf("BAD EVAL %d\n", entry.eval);

    if (entry.key == key && entry.flag) //A valid entry with the right key
        return entry; //Hit!
        // return TT_ENTRY { entry.key, entry.flag, entry.depth, entry.eval, entry.move };

    return TT_ENTRY { 0, 0, 0, 0, 0 }; //return an invalid entry (flag = 0)
}

//Set a TT entry (if improving depth)
inline void set_entry(uint64_t key, uint8_t flag, uint8_t depth, int16_t eval, MOVE move)
{
    // TT_INDEX tt_index = key & (TT_SIZE - 1);
    TT_INDEX tt_index = key % TT_SIZE;
    TT_ENTRY entry = transpo_table[tt_index];

    //insufficient depth
    // if (entry.depth > depth)

    if (entry.flag && !REPLACEMENT_SCHEME(depth, entry.depth))
        return;
    
    //less precise flag (exact < upperbound < lowerbound)
    if (entry.flag && entry.flag < flag) //TODO: try inverting values of alpha and beta
        return;

    transpo_table[tt_index].key = key;
    transpo_table[tt_index].flag = flag;
    transpo_table[tt_index].depth = depth;
    transpo_table[tt_index].eval = eval;
    transpo_table[tt_index].move = MOVE_ID(move);
}

//TODO: improve this function!
//It's not completely collision-proof, and doesn't work with promotions and accepts some illegal pawn moves and castling moves
inline bool is_acceptable(uint16_t move_id)
{
    uint8_t src = move_id >> 8;
    uint8_t diff = 0x77 + move_id - src;
    uint8_t ptype = board[src] & PTYPE;

    if (ptype == KING && std::abs((int8_t)diff) == 2) //castling move
        return true; //sometimes (but rarely) allows illegal castling

    uint8_t ray = RAYS[diff]; //a ray has to exist
    uint8_t mask = RAYMSK[ptype]; //the piece on the source square has to be able to perform that move
    if (!(ray ^ mask)) return false; //the piece cannot perform that move; the move is illegal

    if (ptype < BISHOP) //leaper
        return true;

    //compute for sliding pieces
    uint8_t offset = RAY_OFFSETS[diff];
    if (!offset) return false; //the move is illegal (this should already have been filtered out!!!)
    for (uint8_t cur_sq = move_id - offset; cur_sq != src; cur_sq -= offset)
        if (board[cur_sq] || (cur_sq & OFFBOARD)) return false; //there is a piece in the way

    //nothing is in the way: the move is acceptable
    return true;
}

//same as prev function, but for captures in qsearch
inline bool is_acceptable_capt(uint16_t move_id)
{
    uint8_t tgt = move_id;
    if (!board[tgt]) //empty square
        return false; //not a capture!

    uint8_t src = move_id >> 8;
    uint8_t diff = tgt - src;
    uint8_t ptype = board[src] & PTYPE;
    uint8_t ray = RAYS[diff]; //a ray has to exist
    uint8_t mask = RAYMSK[ptype]; //the piece on the source square has to be able to perform that move
    if (!(ray ^ mask)) return false; //the piece cannot perform that move; the move is illegal

    if (ptype < BISHOP) //leaper
        return true;

    //compute for sliding pieces
    uint8_t offset = RAY_OFFSETS[diff];
    if (!offset) return false; //the move is illegal (this should already have been filtered out!!!)
    for (uint8_t cur_sq = move_id - offset; cur_sq != src; cur_sq -= offset)
        if (board[cur_sq] || (cur_sq & OFFBOARD)) return false; //there is a piece in the way

    //nothing is in the way: the move is acceptable
    return true;
}

//A function that helps determine the hash of a move (aka the change in board hash it generates)
//Xoring the results before and after a move gives you the move hash ^ Z_TURN
//BUG: this function is probably not working properly, and it could very well be something sus with special moves (castling/en passant)
inline uint64_t move_hash(MOVE move)
{
    uint8_t source_piece = board[move.src] & 15; //saves piece's color (1 bit) and type (3 bits)
    uint8_t target_piece = board[move.tgt] & 15; //same here, but with the target
    uint16_t special_move_index_1 = 0; //actually it *might* be with the special moves?
    uint16_t special_move_index_2 = 0;
    uint64_t crl = 0;

    //No risk of overflow here! So we *can* use these offsetted squares without any risk
    if (move.flags & F_EP)
    {
        uint8_t special_square_1 = move.tgt - 16; //piece/pawn above (im doing both squares because i dont need to do fancy stuff for seeing whose turn it is)
        uint8_t special_square_2 = move.tgt + 16; //piece/pawn below

        special_move_index_1 = (board[special_square_1] & 15) << 7 | special_square_1;
        special_move_index_2 = (board[special_square_2] & 15) << 7 | special_square_2;
    }
    else if (move.flags & F_CASTLE)
    {
        uint8_t special_square_1 = move.tgt - 1; //queenside castling: is there a rook? (THIS LINE IS SUS AS HELL: not corresponding to rook on a1 when target is c1)
        uint8_t special_square_2 = move.tgt + 1; //kingside castling

        special_move_index_1 = (board[special_square_1] & 15) << 7 | special_square_1;
        special_move_index_2 = (board[special_square_2] & 15) << 7 | special_square_2;
    }

    //also do hashing for loss of castle rights (king/rook first move), also includes castling
    //NOTE: after the piece moves, the piece will have the MOVED bit set, so the conditions would always fail
    if ((board[move.src] & (PTYPE | MOVED)) == KING) crl = Z_CRL(move.src); //moving virgin king: loss of both castle rights (WARNING!!!)
    else if ((board[move.src] & (PTYPE | MOVED)) == ROOK) crl = Z_CRL(move.src); //moving virgin rook: loss of one castle right

    return
        zobrist_table[source_piece << 7 | move.src] ^ //Piece move
        zobrist_table[target_piece << 7 | move.tgt] ^
        zobrist_table[special_move_index_1] ^ //Special moves (castling/en passant)
        zobrist_table[special_move_index_2] ^
        crl; //Loss of a castling right
}

#endif
