#ifndef __EVAL_H__
#define __EVAL_H__

#include "board.h"

#define MATE_SCORE -30000 //Score of a checkmate (loss); not -32398, since that could result in overflows
#define IS_MATE(score) (-MATE_SCORE - std::abs(score) < 256) //is the score a mate score?
#define NULLMOVE_MATE (-MATE_SCORE - MAX_DEPTH) //mate score for nullmove: avoid returning erroneous mates, but still trigger IS_MATE
#define TOTAL_PHASE 24 //phase of starting position (readjust if changing game_phase array)


extern const int16_t mg_piece_values[], eg_piece_values[];
extern const uint8_t game_phase[];
extern const int16_t mg_mobility[8][28], eg_mobility[8][28];
extern uint8_t king_attack[], king_defense[]; //TUNING: this stuff has to be accessible!
extern uint8_t short_castling, long_castling;

void init_eval();
int16_t evaluate(uint8_t stm);

#endif
