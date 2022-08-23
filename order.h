#ifndef __ORDER_H__
#define __ORDER_H__

#include "board.h"
#include "tt.h"

#define HIST_LENGTH 2048
#define CHIST_LENGTH 1048576 //1MB conthist table (colors required to differentiate counters/followups): not the most efficient
#define MAX_HISTORY (uint32_t)(0x7FFFFFFF - MAX_DEPTH * MAX_DEPTH)


extern uint16_t killers[MAX_DEPTH][2];
extern uint32_t history[HIST_LENGTH];
extern uint32_t conthist[CHIST_LENGTH];

void clear_history();
void score_moves(MLIST *mlist, uint8_t stm, uint16_t hash_move, MOVE prevmove, MOVE_RESULT prevres, bool use_see, uint8_t ply);
MOVE pick_move(MLIST *mlist);
int16_t see(uint8_t stm, uint8_t square);

#endif
