#ifndef __EVAL_H__
#define __EVAL_H__

#include "board.h"

#define MATE_SCORE -32398 //a max depth of 256 or less is already forced by the uint8_t depths
#define IS_MATE(score) (-MATE_SCORE - std::abs(score) < 256) //is the score a mate score?
#define TOTAL_PHASE 24 //phase of starting position (readjust if changing game_phase array)


extern const int16_t mg_piece_values[], eg_piece_values[];
extern const uint8_t game_phase[];
int16_t evaluate();

#endif
