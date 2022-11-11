//TODO: try use weights from http://hgm.nubati.net/book_format.html (low collision rate)

#ifndef __TT_H__
#define __TT_H__

#include "board.h"
#include "eval.h" //mate scores (TODO)


//TT size (default is 1 << 24 = 16777216)
#define TT_SIZE ((1 << 24) / sizeof(TT_ENTRY))
// #define TT_SIZE (1 << 24) //power of 2 TT size

#define HF_EXACT 1
#define HF_BETA 2
#define HF_ALPHA 4

#define Z_TURN (zobrist_table[129]) //b8 entry on black WPAWN board (unused, since white pawns are not black)
#define Z_CASTLE(sq) (zobrist_table[(sq) | 128]) //black WPAWN board (unused); only uses corners of the board
#define Z_DPP(lt) ((lt == (uint8_t)-2) ? 0 : (zobrist_table[(lt) | 128])) //black WPAWN board (unused); uses central 2 rows of the board
#define Z_NULLMOVE (zobrist_table[130]) //c8 entry on black WPAWN board (unused, since white pawns are not black); use for null move pruning


typedef uint32_t TT_INDEX; //Change to uint64_t on large TT sizes (above ~28GB)

typedef struct {
    uint64_t key; //The position key
    uint8_t flag; //A flag to identify the score as exact, alpha or beta
    uint8_t depth; //The depth of the entry
    int16_t eval; //The evaluation of the position
    uint16_t move; //The best move of the previous search
} TT_ENTRY;

extern TT_ENTRY *transpo_table;
extern TT_INDEX tt_size;


extern uint64_t prng_state; //PRNG state
extern uint64_t zobrist_table[]; //Zobrist table (uses [piece][square] indexing, similar to history table)

uint64_t pseudo_rng(); //64-bit pseudo-random number generation (Xorshift64)
void init_tt();
void reallocate_tt(TT_INDEX size);
void clear_tt();
uint64_t board_hash(uint8_t stm, uint8_t last_target); //DEBUG: returns a hash of the board


//Get a TT entry (if at or above required depth)
inline TT_ENTRY get_entry(uint64_t key, uint8_t ply)
{
    // TT_INDEX tt_index = key & (TT_SIZE - 1);
    TT_INDEX tt_index = key % tt_size;
    TT_ENTRY entry = transpo_table[tt_index];

    // assert(tt_index < TT_SIZE);
    // if (entry.key == key && entry.flag)
    //     if (entry.eval %5 != 0 && !IS_MATE(entry.eval)) printf("BAD EVAL %d\n", entry.eval);

    if (entry.key == key && entry.flag) //A valid entry with the right key
        {
            //undo mate score storage stuff
            if (entry.eval < MATE_SCORE + 256) //getting mated
                entry.eval += ply;
            else if (entry.eval > -MATE_SCORE - 256) //mating
                entry.eval -= ply;

            return entry; //Hit!
        }

    return TT_ENTRY { 0, 0, 0, 0, 0 }; //return an invalid entry (flag = 0)
}

//Set a TT entry (if improving depth)
inline void set_entry(uint64_t key, uint8_t flag, bool is_pv, uint8_t depth, int16_t eval, MOVE move, uint8_t ply)
{
    // TT_INDEX tt_index = key & (TT_SIZE - 1);
    TT_INDEX tt_index = key % tt_size;
    TT_ENTRY entry = transpo_table[tt_index];

    //insufficient depth
    // if (entry.depth > depth)

    if (entry.flag)
    {
        // if (depth < entry.depth)
        if(depth + 2 * is_pv <= entry.depth - 4)
            return;
    }
    
    //less precise flag (exact < upperbound < lowerbound)
    if (entry.flag && entry.flag < flag) //TODO: try inverting values of alpha and beta
        return;

    //Storage of mate scores
    if (eval < MATE_SCORE + 256) //getting mated
        eval -= ply;
    else if (eval > -MATE_SCORE - 256) //mating
        eval += ply;

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
    if (!(ray & mask)) return false; //the piece cannot perform that move; the move is illegal

    if (ptype < BISHOP) //leaper
        return true;

    //compute for sliding pieces
    uint8_t offset = RAY_OFFSETS[diff];
    if (!offset) return false; //the move is illegal (this should already have been filtered out!!!)
    for (uint8_t cur_sq = move_id - offset; cur_sq != src; cur_sq -= offset)
        if ((cur_sq & OFFBOARD) || board[cur_sq]) return false; //there is a piece in the way

    //nothing is in the way: the move is acceptable
    return true;
}

//same as prev function, but for captures in qsearch (FIX BUFFER OVERFLOWS!!! as line 102 and line 113 when im gonna try use this)
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
    if (!(ray & mask)) return false; //the piece cannot perform that move; the move is illegal

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
inline uint64_t move_hash(uint8_t stm, MOVE move)
{
    uint64_t hash = 0;

    uint8_t source_piece = board[move.src] & 15; //saves piece's color (1 bit) and type (3 bits)
    uint8_t target_piece = board[move.tgt] & 15; //same here, but with the target
    
    hash ^= zobrist_table[(source_piece << 7) | move.src]; //source piece
    hash ^= zobrist_table[(target_piece << 7) | move.tgt]; //target piece

    //Other squares to save for special moves: target + 16, target - 16, target + 1, target - 1, target - 2
    if (move.flags & F_EP) //move.tgt is in an "en passant range": 4 middle rows
    {
        //target +- 16 definitely on the board: we can save those
        uint8_t current_piece = board[move.tgt + 16] & 15;
        hash ^= zobrist_table[current_piece << 7 | (move.tgt + 16)];

        current_piece = board[move.tgt - 16] & 15;
        hash ^= zobrist_table[current_piece << 7 | (move.tgt - 16)];
    }

    if (move.flags & F_CASTLE) //castling move
    {
        //target +- 1 and target - 2 definitely on the board: we can save those
        uint8_t current_piece = board[move.tgt + 1] & 15;
        hash ^= zobrist_table[current_piece << 7 | (move.tgt + 1)]; //kingside rook on h-file, queenside rook after castling on d-file

        current_piece = board[move.tgt - 1] & 15;
        hash ^= zobrist_table[current_piece << 7 | (move.tgt - 1)]; //kingside rook after castling on f-file

        current_piece = board[move.tgt - 2] & 15;
        hash ^= zobrist_table[current_piece << 7 | (move.tgt - 2)]; //queenside rook on a-file
    }

    //calculate castling rights
    uint8_t king_square = plist[(stm & 16) ^ 16];
    uint8_t king_status = board[king_square];

    if (!(king_status & MOVED)) //king hasn't moved: castling rights may exist
    {
        if (!(board[king_square + 3] & MOVED)) //kingside rook hasn't moved: kingside castle right
            hash ^= Z_CASTLE(king_square + 3);
        if (!(board[king_square - 4] & MOVED)) //queenside rook hasn't moved: queenside castle right
            hash ^= Z_CASTLE(king_square - 4);
    }

    return hash;
}

#endif
