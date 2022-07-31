#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <math.h>
#include "board.h"
#include "eval.h"
#include "tt.h"
#include "order.h"
#include "posix.h"

#define DELTA 300 //delta pruning threshold
#define LMR_MINDEPTH 3 //LMR minimum depth (included)
#define LMR_THRESHOLD 250 //search first 2 legal moves with full depth
#define LMR_MAXSCORE SCORE_CHECK //do not reduce moves at or above this score


extern uint8_t lmr_table[MAX_DEPTH][MAX_MOVE]; //LMR table

void init_lmr();

uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth);

int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, uint8_t ply);
int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta/* , uint64_t hash */);
void search_root(uint32_t time_ms); //Assuming the position is legal (enemy king not under attack)

#endif
