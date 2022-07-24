#ifndef __ORDER_H__
#define __ORDER_H__

#include "board.h"


extern uint16_t killers[MAX_DEPTH][2];
extern uint32_t history[64 * 64];

void clear_history();
void order_moves(MLIST *mlist, uint8_t ply);

#endif
