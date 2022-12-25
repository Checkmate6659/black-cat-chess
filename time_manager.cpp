#include "time_manager.h"

#ifdef TUNING_TM
int default_mtg = 32;
double tm_mul = 0.4231, tm_mul2 = 0.5114;
int tm_const = -9;
double tm_nodefrac_mul = 0.0751, tm_nodefrac_const = 0.0814;
double tm_afterbook_bonus = 1.5217;
int tm_afterbook_length = 24;
#endif

clock_t total_remaining_time;
uint16_t moves_after_book = 0;

uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo)
{
    uint32_t allocated_time = time / movestogo + increment; //basic time manager
    //always leave 10ms extra time, beyond the rather large overhead
    total_remaining_time = (time - OVERHEAD - 10) * CLOCKS_PER_SEC / 1000; //total_remaining_time is in clock, not in ms

    //formula from Cray Blitz, with tuning
    uint8_t nmoves = std::min(moves_after_book, (uint16_t)TM_AFTERBOOK_LENGTH);
    allocated_time *= 1 + TM_AFTERBOOK_BONUS * (1 - nmoves / TM_AFTERBOOK_LENGTH); //multiplier is 2 at start, but converges towards 1; no risk of flagging

    //hard cap on time usage (under no circumstance use more than 7/8th of available time)
    // allocated_time = std::fmin(allocated_time, 7 * time / 8);

    //always leave at least 10ms to spare (base time, may change during search)
    allocated_time = std::fmin(allocated_time, time - 10);

    //lower hard cap on time to avoid crashing (not sure if necessary)
    return std::fmax(allocated_time, MIN_SEARCH_TIME + OVERHEAD) - OVERHEAD;
}
