#ifndef __ORDER_H__
#define __ORDER_H__

#include "board.h"

#define HIST_LENGTH 2048


extern uint16_t killers[MAX_DEPTH][2];
extern uint32_t history[HIST_LENGTH];

void clear_history();
void order_moves(MLIST *mlist, uint8_t ply);

#endif
