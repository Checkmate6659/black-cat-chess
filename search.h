#ifndef __SEARCH_H__
#define __SEARCH_H__

#include <math.h>
#include "board.h"
#include "eval.h"
#include "tt.h"
#include "order.h"
#include "posix.h"

#define TUNING_MODE


#define RPT_BITCNT 18 //Repetition hash's size (default is 16-bit indices: lower 16 bits of Zobrist key)
#define RPT_SIZE (1 << RPT_BITCNT) //Repetition table size
#define RPT_MASK (RPT_SIZE - 1) //Repetition table mask (Zobrist key & mask = table index)

#ifdef TUNING_MODE

//Not tuning these immediately
#define STALEMATE 0 //Stalemate: side to move has no legal moves
#define REPETITION 0 //Draw score when we repeated moves (twofold)
#define FIFTY_MOVE 0 //Opponent has made the last move and ended the game

//Aspiration windows
extern int aspi_margin, max_aspi_margin, aspi_constant;
extern float aspi_mul;
#define ASPI_MARGIN aspi_margin //Starting aspiration window margin (30cp)
#define MAX_ASPI_MARGIN max_aspi_margin //Maximum aspiration window margin (2000cp); beyond this, expand all the way to MATE_SCORE
#define ASPI_MULTIPLIER (aspi_mul/100) //multiply corresponding margin by this each time we fail (can be integer or rational A/B)
#define ASPI_CONSTANT aspi_constant //add this to the margin each time we fail

extern int rfp_max_depth, rfp_margin, rfp_impr;
#define RFP_MAX_DEPTH rfp_max_depth //Max depth when RFP is active
#define RFP_MARGIN rfp_margin //Margin per ply (margin at depth N = N*RFP_MARGIN)
#define RFP_IMPR rfp_impr //Remove this from margin on improving (likely fail high) nodes, to increase fail high count (TEMP DISABLED)

extern int iid_reduction_d;
#define TT_FAIL_REDUCTION_MINDEPTH iid_reduction_d //Min depth to reduce PV-nodes where probing was unsuccessful

extern int dprune;
// #define DELTA dprune //delta pruning threshold

extern int nmp_const, nmp_depth, nmp_evalmin, nmp_evaldiv;
#define NULL_MOVE_REDUCTION_CONST nmp_const
#define NULL_MOVE_REDUCTION_DEPTH nmp_depth
// #define NULL_MOVE_REDUCTION_MIN nmp_evalmin
#define NULL_MOVE_REDUCTION_DIV nmp_evaldiv

extern float lmr_const, lmr_mul;
#define LMR_CONST (lmr_const/10000) //LMR constant (Ethereal: 0.75)
#define LMR_MUL (lmr_mul/10000) //LMR multiplier; original (from Ethereal) was 1/2.25 = 0.4444... (0.4444 gives same result)

extern bool lmr_do_pv, lmr_do_impr, lmr_do_chk_kmove, lmr_do_killer;
extern uint32_t lmr_history_threshold;
#define LMR_HISTORY_THRESHOLD lmr_history_threshold

extern float lmr_sqrt_mul, lmr_dist_mul, lmr_depth_mul;
#define LMR_SQRT_MUL (lmr_sqrt_mul/10000)
#define LMR_DIST_MUL (lmr_dist_mul/10000)
#define LMR_DEPTH_MUL (lmr_depth_mul/10000)

extern bool hlp_do_improving;
extern uint8_t hlp_movecount;
extern uint32_t hlp_reduce, hlp_prune;
#define HLP_MOVECOUNT hlp_movecount
#define HLP_REDUCE hlp_reduce
#define HLP_PRUNE hlp_prune

extern uint8_t chkext_depth;
#define CHKEXT_MINDEPTH chkext_depth

extern uint8_t see_depth;
extern int16_t see_noisy, see_quiet;
#define SEE_MAX_DEPTH see_depth
#define SEE_NOISY see_noisy //*depth^2
#define SEE_QUIET see_quiet //*depth

#define LMP_MAXDEPTH 9 //cannot be tuned easily! just leaving it as it is
extern double lmp_noimpr_const, lmp_noimpr_linear, lmp_noimpr_quad, lmp_impr_const, lmp_impr_linear, lmp_impr_quad; //all the params (x10000)
#else

//Draw scores for different endings to implement different contempt factors (TODO: insufficient mating material)
#define STALEMATE 0 //Stalemate: side to move has no legal moves
#define REPETITION 0 //Draw score when we repeated moves (twofold)
#define FIFTY_MOVE 0 //Opponent has made the last move and ended the game

#define ASPI_MARGIN 22 //Starting aspiration window margin
#define MAX_ASPI_MARGIN 2000 //Maximum aspiration window margin (2000cp); beyond this, expand all the way to MATE_SCORE
#define ASPI_MULTIPLIER 1.68 //multiply corresponding margin by this each time we fail (can be integer, rational A/B or float)
#define ASPI_CONSTANT 39 //add this to the margin each time we fail

// #define RFP_MAX_DEPTH 17 //Max depth when RFP is active (maybe it should be uncapped?)
#define RFP_MARGIN 100 //Margin per ply (margin at depth N = N*RFP_MARGIN)
#define RFP_IMPR 75 //Remove this from margin on improving (likely fail high) nodes, to increase fail high count

#define TT_FAIL_REDUCTION_MINDEPTH 3 //Min depth to reduce PV-nodes where probing was unsuccessful

// #define DELTA 2069 //delta pruning threshold (TODO: test without any delta pruning)

#define NULL_MOVE_REDUCTION_CONST 3
#define NULL_MOVE_REDUCTION_DEPTH 16
// #define NULL_MOVE_REDUCTION_MIN 44 //TODO: test uncapped
#define NULL_MOVE_REDUCTION_DIV 509

#define LMR_CONST 0.5512 //LMR constant (Ethereal: 0.75)
#define LMR_MUL 0 //LMR multiplier; original (from Ethereal) was 1/2.25 = 0.4444... (0.4444 gives same result; 0.44 gives less nodes but untested)
#define LMR_SQRT_MUL 0.1262
#define LMR_DIST_MUL 0.0882
#define LMR_DEPTH_MUL 0

#define LMR_HISTORY_THRESHOLD 6434

#define SEE_MAX_DEPTH 9 //TODO: tune!
#define SEE_NOISY 19 //*depth^2
#define SEE_QUIET 64 //*depth

#define CHKEXT_MINDEPTH 6

//TODO: tune LMP!
#define LMP_MAXDEPTH 9 //cannot be tuned easily
#endif

// #define LMR_MINDEPTH 1 //LMR minimum depth (included)
// #define LMR_THRESHOLD 2 //search first 2 legal moves with full depth
#define LOUD_MOVE SCORE_CHECK //do not reduce moves at or above this score (NOT IMPLEMENTED; didn't matter, maybe that will change later)


typedef uint32_t RPT_INDEX; //Repetition table index

extern uint8_t lmr_table[MAX_DEPTH][MAX_MOVE]; //LMR table
extern int8_t repetition_table[RPT_SIZE]; //Repetition table

extern uint64_t node_count; //node count for benchmarking
extern bool benchmark; //benchmarking mode: don't panic!

void init_search();

uint64_t perft(uint8_t stm, uint8_t last_target, uint8_t depth);

bool nullmove_safe(uint8_t stm);
int16_t search(uint8_t stm, uint8_t depth, uint8_t last_target, int16_t alpha, int16_t beta, uint64_t hash, int8_t nullmove, uint8_t ply, int8_t last_zeroing_ply);
int16_t qsearch(uint8_t stm, int16_t alpha, int16_t beta);
void search_root(uint32_t time_ms, uint8_t fixed_depth);

#endif
