#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include <math.h>
#include <time.h>
#include "board.h"

#define MIN_SEARCH_TIME 2 //minimum time to search in ms
#define OVERHEAD 10 //extra time in ms to account for communication delays

// #define TUNING_TM

//Cut off if next iteration will probably not finish before time runs out (larger values = more aggressive)
#ifdef TUNING_TM
extern int default_mtg;
#define DEFAULT_MOVESTOGO default_mtg //default number of moves to go

extern double tm_mul, tm_mul2;
extern int tm_const;
#define TM_CUTOFF_MUL tm_mul //coef of previous search's length
#define TM_CUTOFF_MUL2 tm_mul2 //coef of search at depth-2 length
#define TM_CUTOFF_CONST tm_const //constant time, in ms

extern double tm_nodefrac_mul, tm_nodefrac_const;
#define TM_NODEFRAC_MUL tm_nodefrac_mul
#define TM_NODEFRAC_CONST tm_nodefrac_const
#else
#define DEFAULT_MOVESTOGO 29 //default number of moves to go

#define TM_CUTOFF_MUL 0.3855 //coef of previous search's length
#define TM_CUTOFF_MUL2 0.5417 //coef of search at depth-2 length
#define TM_CUTOFF_CONST -10 //constant time, in ms

#define TM_NODEFRAC_MUL 0.0869 //coef of previous search's length
#define TM_NODEFRAC_CONST 0.4784 //coef of search at depth-2 length
#endif


extern clock_t total_remaining_time;

uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo); //allocates time for the current move

#endif
