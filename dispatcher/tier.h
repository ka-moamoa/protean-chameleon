#ifndef TIER_H_
#define TIER_H_

#include <stdint.h>
#include "task.h"

#define MAX_TIERS       3
#define ADAPT_UP(x)     x--
#define ADAPT_DOWN(x)   x++

// creates a tier
#define __CREATE_TIER(tier_level, entry_task) \
        __create_tier(tier_level - 1, (void *)entry_task, (void *)&__persistent_vars, (void *)&__sram_vars, sizeof(FRAM_data_t));


// the tier control block
typedef struct {
    void *sram_buffer;      // pointer to the active buffer containing task shared variables, it is a pointer to SRAM copy
    void *fram_buffer;      // pointer to the active buffer containing task shared variables, it is a pointer to FRAM copy
    uint32_t size;          // size of the buffer
    void *entry;            // first task to be executed
    void *next;             // current task to be executed
} tcb_t;

typedef struct {
    tcb_t tcb_buffer[2]; // double buffer for tier control block
    uint8_t tcb_idx;             // index of the ative tier
} tier_t;

// allocates a double buffer for the persistent variables in FRAM
#define __shared(...)   \
        typedef struct {    \
            __VA_ARGS__     \
        } FRAM_data_t  __attribute__ ((aligned (2)));    \
        static __nv FRAM_data_t __persistent_vars[2]; \
        static FRAM_data_t __sram_vars;


// runs one task, return 0 if buffer is written to FRAM
uint8_t __tick(tier_t *tier);

#endif /* TIER_H_ */
