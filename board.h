#ifndef __BOARD_H__
#define __BOARD_H__

#include <inttypes.h>
#include <iostream>
#include <cassert> //DEBUGGING ONLY!

#include "attacks.h"


//#define NSQUARES 0x40
#define OFFBOARD 0x88
#define MAX_MOVE 218

#define PTYPE 0b000111
#define WHITE 0b001000
#define BLACK 0b010000
#define ENEMY 0b011000
#define PLIST 0b011111
#define MOVED 0b100000 //for castling

#define EMPTY 0
#define WPAWN 1
#define BPAWN 2
#define KNIGHT 3 //maybe i should swap these (knight = 4, king = 3)?
#define KING 4
#define BISHOP 5
#define ROOK 6
#define QUEEN 7

#define F_PROMO 0x80 //Promotion flag
#define F_CAPT 0x40 //Capture flag
#define F_DPP 0x20
#define F_EP 0x04 //En passant flag
#define F_CASTLE 0x03 //Castling flags
#define F_KCASTLE 0x02
#define F_QCASTLE 0x01

//Move scores (I will have to rework this later)
#define SCORE_HASH 0xFF //Hash move
#define SCORE_PROMO_Q 0xFE //Promotion to queen (only promo valued before captures)
#define SCORE_PROMO_N 0xC5 //Promotion to knight
#define SCORE_PROMO_R 0xC4 //Promotion to rook
#define SCORE_PROMO_B 0xC3 //Promotion to bishop
#define SCORE_CAPT 0xC6 //Capturing a piece (with MVV-LVA: add (VICTIM << 3) - AGGRESSOR => spans from 0xC7 to 0xFD, since victim is minimum 0x01, a white pawn) NOTE: value 0xc6 itself is unused
#define SCORE_DCHECK 0xC2 //Double check: extremely dangerous move (TODO: should even be ranking that before captures)
#define SCORE_CHECK 0xC1 //Simple check: can be dangerous, don't look for those at low depth (3 or less) since it's slow
#define SCORE_EVADE 0xC0 //Evasion: move that frees itself from an enemy attack detected by NMH (TODO)
#define SCORE_CASTLE 0xBF //Castling: high score since it protects king
#define SCORE_QUIET 0x00 //Quiet move: add relative history and null move evasion in ordering


typedef struct
{
	uint8_t src; //Source square
	uint8_t tgt; //Target square
	uint8_t flags; //Move flags for doing castling, en passant, promotion, capture, etc.
	uint8_t promo; //Promotion piece (0 = no promotion)
	uint8_t score; //Score of the move (use in move ordering later)
}
MOVE;

/* typedef union //Maybe i don't care about this one...
{
	uint8_t u8[4];
	int8_t i8[4];
	uint16_t u16[2];
	int16_t i16[2];
	uint32_t u32;
	int32_t i32;
}
Types32; */

typedef struct
{
	MOVE moves[MAX_MOVE];
	uint8_t count;
}
MLIST;

typedef struct { //TODO: implement promotion flag!!! (to undo promotions)
	uint8_t piece;
	uint8_t plist_idx;
	uint8_t prev_state = 0; //Promotion pawn before promotion (0 = no promotion, 1 = white pawn, 2 = black pawn)
}
MOVE_RESULT;


void print_board(uint8_t *b); //Print the board.
void print_board_full(uint8_t *b); //Print the fking BOARD!!! And the piece lists. And everything you can think of.
void load_fen(std::string fen); //A fen-loading function. Safety not included in the warranty. Not sure what that even is tbh.
void print_move(MOVE move); //Print a move.

extern uint8_t board[128];
extern uint8_t plist[32];
extern uint8_t board_stm;
extern uint8_t board_last_target;

const uint8_t PIECE_PROMO[] = {QUEEN, ROOK, BISHOP, KNIGHT};
const uint8_t SCORE_PROMO[] = {SCORE_PROMO_Q, SCORE_PROMO_R, SCORE_PROMO_B, SCORE_PROMO_N};
const uint8_t SCORE_PROMO_CAPT[] = {SCORE_PROMO_Q, SCORE_PROMO_R, SCORE_PROMO_B, SCORE_PROMO_N};


uint8_t sq_attacked(uint8_t target, uint8_t attacker); //Square Attacked By function with 0x88 Vector Attacks; returns square of first attacker

void generate_moves(MLIST *mlist, uint8_t stm, uint8_t last_target); //Generating all moves
void generate_loud_moves(MLIST *mlist, uint8_t stm); //Generating "loud" moves only (just captures for now)

MOVE_RESULT make_move(uint8_t stm, MOVE move); //Make a move, and give a MOVE_RESULT struct with the takeback info
void unmake_move(uint8_t stm, MOVE move, MOVE_RESULT move_result); //Use the MOVE_RESULT given out by a make_move function to unmake that move

#endif
