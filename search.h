#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <math.h>
#include "board.h"
#include "eval.h"
#include "tt.h"
#include "order.h"
#include "posix.h"

//Draw scores for different endings to implement different contempt factors (TODO: insufficient mating material)
#define STALEMATE 0 //Stalemate: side to move has no legal moves
#define REPETITION 0 //Draw score when we repeated moves (twofold)
#define FIFTY_MOVE 0 //Opponent has made the last move and ended the game

#define RPT_BITCNT 18 //Repetition hash's size (default is 16-bit indices: lower 16 bits of Zobrist key)
#define RPT_SIZE (1 << RPT_BITCNT) //Repetition table size
#define RPT_MASK (RPT_SIZE - 1) //Repetition table mask (Zobrist key & mask = table index)

#define RFP_MAX_DEPTH 8 //Max depth when RFP is active
#define RFP_MARGIN 75 //Margin per ply (margin at depth N = N*RFP_MARGIN)

#define NULL_MOVE_REDUCTION 2
#define NULL_MOVE_COOLDOWN 1

//SEE activation (has yielded me bad results so far because of significant slowness)
#define SEE_SEARCH false //activate during main search
#define SEE_QSEARCH false //activate during qsearch

#define DELTA 300 //delta pruning threshold
#define LMR_MINDEPTH 3 //LMR minimum depth (included)
#define LMR_THRESHOLD 2 //search first 2 legal moves with full depth
#define LMR_MAXSCORE SCORE_CHECK //do not reduce moves at or above this score (NOT IMPLEMENTED; didn't matter, maybe that will change later)


typedef uint32_t RPT_INDEX; //Repetition table index

extern uint8_t lmr_table[MAX_DEPTH][MAX_MOVE]; //LMR table
extern int8_t repetition_table[RPT_SIZE]; //Repetition table

void init_lmr();

uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth);

bool nullmove_safe(uint8_t stm);
int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, int8_t nullmove, uint8_t ply, int8_t last_zeroing_ply);
int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta);
void search_root(uint32_t time_ms);

#endif
