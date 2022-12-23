#include "time_manager.h"

#ifdef TUNING_TM
int default_mtg = 27;
double tm_mul = 0.3882, tm_mul2 = 0.5644;
int tm_const = -9;
double tm_nodefrac_mul = 0.1001, tm_nodefrac_const = 0.4832;
#endif

clock_t total_remaining_time;

uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo)
{
    uint32_t allocated_time = time / movestogo + increment; //basic time manager
    //always leave 10ms extra time, beyond the rather large overhead
    total_remaining_time = (time - OVERHEAD - 10) * CLOCKS_PER_SEC / 1000; //total_remaining_time is in clock, not in ms

    //hard cap on time usage (under no circumstance use more than 7/8th of available time)
    // allocated_time = std::fmin(allocated_time, 7 * time / 8);

    //always leave at least 10ms to spare (base time, may change during search)
    allocated_time = std::fmin(allocated_time, time - 10);

    //lower hard cap on time to avoid crashing (not sure if necessary)
    return std::fmax(allocated_time, MIN_SEARCH_TIME + OVERHEAD) - OVERHEAD;
}
