#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include <math.h>
#include "board.h"

#define DEFAULT_MOVESTOGO 30 //default number of moves to go
#define DEFAULT_ENGINE_TIME (1000 * DEFAULT_MOVESTOGO) //default engine time in ms: 1 second per move (may need modification if i change time manager)
#define MIN_SEARCH_TIME 5 //minimum time to search in ms
#define OVERHEAD 20 //extra time in ms to account for communication delays

//Cut off if next iteration will probably not finish before time runs out (larger values = more aggressive)
#define TM_CUTOFF_MUL 0.9 //coef of previous search's length
#define TM_CUTOFF_CONST -10 //constant time, in ms


uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo); //allocates time for the current move

#endif
