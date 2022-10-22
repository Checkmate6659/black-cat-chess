#include "time_manager.h"

#ifdef TUNING_TM
int default_mtg = 30;
double tm_mul = 0.4, tm_mul2 = 0.5;
int tm_const = -10;
#endif

uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo)
{
    uint32_t allocated_time = time / movestogo + increment; //extremely basic time manager
    return std::fmax(allocated_time, MIN_SEARCH_TIME + OVERHEAD) - OVERHEAD;
}
