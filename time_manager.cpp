#include "time_manager.h"

uint32_t alloc_time(uint32_t time, uint32_t increment, uint8_t movestogo)
{
    uint32_t allocated_time = time / movestogo + increment; //extremely basic time manager
    return std::fmax(allocated_time, MIN_SEARCH_TIME + OVERHEAD) - OVERHEAD;
}
