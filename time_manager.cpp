#include "time_manager.h"

#ifdef TUNING_TM
int default_mtg = 29;
double tm_mul = 0.3763, tm_mul2 = 0.5358;
int tm_const = -10;
#endif

uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo)
{
    uint32_t allocated_time = time / movestogo + increment; //extremely basic time manager

    //hard cap on time usage (under no circumstance use more than 7/8th of available time)
    allocated_time = std::fmin(allocated_time, 7 * time / 8);

    //lower hard cap on time to avoid crashing (not sure if necessary)
    return std::fmax(allocated_time, MIN_SEARCH_TIME + OVERHEAD) - OVERHEAD;
}
