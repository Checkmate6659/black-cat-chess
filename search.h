#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "board.h"
#include "eval.h"
#include "tt.h"
#include "order.h"
#include "posix.h"

#define DELTA 300 //delta pruning threshold


uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth);

int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, uint8_t ply);
int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta);
void search_root(uint32_t time_ms); //Assuming the position is legal (enemy king not under attack)

#endif
