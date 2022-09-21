//NNUE wrapping functions
#ifndef __NNUE_H__
#define __NNUE_H__

#include <inttypes.h>

#include "./nnue/nnue.h"


//TODO: incremental update
//Dirty piece update (Cfish) starts from https://github.com/syzygy1/Cfish/blob/master/src/position.c#L780
//Very Important Part (VIP): https://github.com/syzygy1/Cfish/blob/master/src/position.c#L904
//End: https://github.com/syzygy1/Cfish/blob/master/src/position.c#L934
//all the #ifdef NNUE macros = dirty piece
//definition: https://github.com/syzygy1/Cfish/blob/master/src/position.c#L783
//do_move function in position.c
//btw all the stuff like SQ_NONE and make_piece and likely/unlikely is in types.h
//for accumulator: enum { ACC_EMPTY, ACC_COMPUTED, ACC_INIT };
//and state corresponds with computedAccumulation (altho in cfish its int[2] and here its int)
//A very important info is that nnue_evaluate_pos (and so nnue_evaluate_incremental) update the NNUEdata they are given
//The accumulator is computed and accessible/storable after a call to the evaluation function


/* Cfish reverse engineering
at start: (DONE)
  Stack *st = ++pos->st;
  st->accumulator.state[WHITE] = ACC_EMPTY; //ACC_EMPTY = 0
  st->accumulator.state[BLACK] = ACC_EMPTY; //ACC_EMPTY = 0
  DirtyPiece *dp = &(st->dirtyPiece);
  dp->dirtyNum = 1;
if castling: (DONE)
  dp->dirtyNum = 2;
  dp->pc[1] = captured; //friendly rook (castling = king x rook)
  dp->from[1] = rfrom; //rook's original square
  dp->to[1] = rto; //rook's final square
if capturing (not castling, includes ep):
  dp->dirtyNum = 2; // captured piece goes off the board
  dp->pc[1] = captured; //captured piece (in NNUE type)
  dp->from[1] = capsq; //square of captured piece (in NNUE square); it's the pawn's square in EP
  dp->to[1] = SQ_NONE; //SQ_NONE = 64
in all cases: (DONE)
  dp->pc[0] = piece;
  dp->from[0] = from;
  dp->to[0] = to;
if promoting: (DONE)
  dp->to[0] = SQ_NONE;   // pawn to SQ_NONE, promoted piece from SQ_NONE
  dp->pc[dp->dirtyNum] = promotion;
  dp->from[dp->dirtyNum] = SQ_NONE;
  dp->to[dp->dirtyNum] = to;
  dp->dirtyNum++;
*/

#define NNUE_SQ(sq) (((sq) + ((sq) & 7)) >> 1) ^ 56


typedef struct NNUEstack {
    NNUEdata nnue_data[256]; //Max depth is 127, but we want some extra for qsearch
    uint8_t ply;
} NNUEstack;

extern const uint8_t NNUE_PIECE_INDEX[];

//Wrapper functions
void init_nnue();
void init_nnue (char *fname);
int eval_nnue (uint8_t stm);
int eval_nnue_inc (uint8_t stm, NNUEstack *stack);

#endif
