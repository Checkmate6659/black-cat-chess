#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <math.h>
#include "board.h"
#include "eval.h"
#include "tt.h"
#include "order.h"
#include "posix.h"

// #define TUNING_MODE


#define RPT_BITCNT 18 //Repetition hash's size (default is 16-bit indices: lower 16 bits of Zobrist key)
#define RPT_SIZE (1 << RPT_BITCNT) //Repetition table size
#define RPT_MASK (RPT_SIZE - 1) //Repetition table mask (Zobrist key & mask = table index)

#ifdef TUNING_MODE

//Not tuning these immediately
#define STALEMATE 0 //Stalemate: side to move has no legal moves
#define REPETITION 0 //Draw score when we repeated moves (twofold)
#define FIFTY_MOVE 0 //Opponent has made the last move and ended the game

//Aspiration windows
extern int aspi_margin, max_aspi_margin, aspi_mul, aspi_constant;
#define ASPI_MARGIN aspi_margin //Starting aspiration window margin (30cp)
#define MAX_ASPI_MARGIN max_aspi_margin //Maximum aspiration window margin (2000cp); beyond this, expand all the way to MATE_SCORE
#define ASPI_MULTIPLIER aspi_mul //multiply corresponding margin by this each time we fail (can be integer or rational A/B)
#define ASPI_CONSTANT aspi_constant //add this to the margin each time we fail

extern int rfp_max_depth, rfp_margin, rfp_impr;
#define RFP_MAX_DEPTH rfp_max_depth //Max depth when RFP is active
#define RFP_MARGIN rfp_margin //Margin per ply (margin at depth N = N*RFP_MARGIN)
#define RFP_IMPR rfp_impr //Remove this from margin on improving (likely fail high) nodes, to increase fail high count (TEMP DISABLED)

extern int iid_reduction_d;
#define TT_FAIL_REDUCTION_MINDEPTH iid_reduction_d //Min depth to reduce PV-nodes where probing was unsuccessful

//SEE activation (has yielded me bad results so far because of significant slowness)
#define SEE_SEARCH false //activate during main search
#define SEE_QSEARCH false //activate during qsearch

extern int dprune;
#define DELTA dprune //delta pruning threshold

extern int nmp_const, nmp_depth, nmp_evalmin, nmp_evaldiv;
#define NULL_MOVE_REDUCTION_CONST nmp_const
#define NULL_MOVE_REDUCTION_DEPTH nmp_depth
#define NULL_MOVE_REDUCTION_MIN nmp_evalmin
#define NULL_MOVE_REDUCTION_DIV nmp_evaldiv
#else

//Draw scores for different endings to implement different contempt factors (TODO: insufficient mating material)
#define STALEMATE 0 //Stalemate: side to move has no legal moves
#define REPETITION 0 //Draw score when we repeated moves (twofold)
#define FIFTY_MOVE 0 //Opponent has made the last move and ended the game

#define ASPI_MARGIN 30 //Starting aspiration window margin (30cp)
#define MAX_ASPI_MARGIN 2000 //Maximum aspiration window margin (2000cp); beyond this, expand all the way to MATE_SCORE
#define ASPI_MULTIPLIER 3 //multiply corresponding margin by this each time we fail (can be integer or rational A/B)
#define ASPI_CONSTANT 10 //add this to the margin each time we fail

#define RFP_MAX_DEPTH 8 //Max depth when RFP is active
#define RFP_MARGIN 150 //Margin per ply (margin at depth N = N*RFP_MARGIN)
#define RFP_IMPR 0 //Remove this from margin on improving (likely fail high) nodes, to increase fail high count (TEMP DISABLED)

#define TT_FAIL_REDUCTION_MINDEPTH 5 //Min depth to reduce PV-nodes where probing was unsuccessful

//SEE activation (has yielded me bad results so far because of significant slowness)
#define SEE_SEARCH false //activate during main search
#define SEE_QSEARCH false //activate during qsearch

#define DELTA 300 //delta pruning threshold

#define NULL_MOVE_REDUCTION_CONST 2
#define NULL_MOVE_REDUCTION_DEPTH 6
#define NULL_MOVE_REDUCTION_MIN 3
#define NULL_MOVE_REDUCTION_DIV 200
#endif

// #define LMR_MINDEPTH 1 //LMR minimum depth (included)
// #define LMR_THRESHOLD 2 //search first 2 legal moves with full depth
#define LMR_MAXSCORE SCORE_CHECK //do not reduce moves at or above this score (NOT IMPLEMENTED; didn't matter, maybe that will change later)


typedef uint32_t RPT_INDEX; //Repetition table index

extern uint8_t lmr_table[MAX_DEPTH][MAX_MOVE]; //LMR table
extern int8_t repetition_table[RPT_SIZE]; //Repetition table

extern uint64_t node_count; //node count for benchmarking
extern bool benchmark; //benchmarking mode: don't panic!

void init_lmr();

uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth);

bool nullmove_safe(uint8_t stm);
int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, int8_t nullmove, uint8_t ply, int8_t last_zeroing_ply);
int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta);
void search_root(uint32_t time_ms, uint8_t fixed_depth);

#endif
