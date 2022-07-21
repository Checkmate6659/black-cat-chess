#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include <math.h>
#include "board.h"

#define DEFAULT_MOVESTOGO 30 //default number of moves to go
#define DEFAULT_ENGINE_TIME (1000 * DEFAULT_MOVESTOGO) //default engine time in ms: 1 second per move (may need modification if i change time manager)
#define MIN_SEARCH_TIME 100 //minimum time to search in ms
#define OVERHEAD 20 //extra time in ms to account for communication delays


uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo); //allocates time for the current move

#endif
