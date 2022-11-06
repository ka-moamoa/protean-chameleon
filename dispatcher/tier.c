#include "tier.h"
#include "nvm.h"

#ifdef Supersensor
#include "energy.h"
#endif

// indicates if device rebooted
uint8_t _booted = TRUE;

// returns 1 if buffer is read from FRAM
uint8_t __copy_buff_fram_to_sram(tier_t *tier) {
    // get data from FRAM only if the device reboots, otherwise use the SRAM copy
#if ADAPTIVE_FRAM_READS    
    if (_booted) {
        // if sram_buffer[1] is active, copy from fram_buffer[1] in FRAM
        KERNEL_LOG_DEBUG("copy started buff[%u] -> SRAM", tier->tcb_idx);
        __NVM_MEMSET(tier->tcb_buffer[tier->tcb_idx].sram_buffer, tier->tcb_buffer[tier->tcb_idx].fram_buffer, tier->tcb_buffer[tier->tcb_idx].size);
        KERNEL_LOG_DEBUG("copied buff[%u] -> SRAM", tier->tcb_idx);
        _booted = FALSE;
        return 1;
    }
    return 0;
#else
    // if sram_buffer[1] is active, copy from fram_buffer[1] in FRAM
    KERNEL_LOG_DEBUG("copy started buff[%u] -> SRAM", tier->tcb_idx);
    __NVM_MEMSET(tier->tcb_buffer[tier->tcb_idx].sram_buffer, tier->tcb_buffer[tier->tcb_idx].fram_buffer, tier->tcb_buffer[tier->tcb_idx].size);
    KERNEL_LOG_DEBUG("copied buff[%u] -> SRAM", tier->tcb_idx);
    _booted = FALSE;
    return 1;
#endif
}

// return 1 if buffer is written to FRAM
uint8_t __copy_buff_sram_to_fram(tier_t *tier) {
#if ADAPTIVE_FRAM_WRITES
    // TODO
    // check if energy in the capacitor is sufficient for the next task and writing back to FRAM after executing that task

    if (!isSufficientEnergy()){
        // if sram_buffer[1] is active, copy SRAM buffer to fram_buffer[0]
        KERNEL_LOG_DEBUG("copy started SRAM -> buff[%u]", !tier->tcb_idx);
        __NVM_MEMSET(tier->tcb_buffer[!tier->tcb_idx].fram_buffer, tier->tcb_buffer[tier->tcb_idx].sram_buffer, tier->tcb_buffer[!tier->tcb_idx].size);
        KERNEL_LOG_DEBUG("copied SRAM -> buff[%u]", !tier->tcb_idx);
        return 1;
    }
    return 0;
#else
    // if sram_buffer[1] is active, copy SRAM buffer to fram_buffer[0]
    KERNEL_LOG_DEBUG("copy started SRAM -> buff[%u]", !tier->tcb_idx);
    __NVM_MEMSET(tier->tcb_buffer[!tier->tcb_idx].fram_buffer, tier->tcb_buffer[tier->tcb_idx].sram_buffer, tier->tcb_buffer[!tier->tcb_idx].size);
    KERNEL_LOG_DEBUG("copied SRAM -> buff[%u]", !tier->tcb_idx);
    return 1;
#endif
}


// runs one task, return 0 if buffer is written to FRAM
uint8_t __tick(tier_t *tier)
{
    // prepare SRAM for task execution 
    // copy active buffer from FRAM to SRAM
    __copy_buff_fram_to_sram(tier);

    // execute task
    // function call void* (*task_t) (buffer_t *)
    tier->tcb_buffer[!tier->tcb_idx].next = (void *)(((task_t)tier->tcb_buffer[tier->tcb_idx].next)(tier->tcb_buffer[tier->tcb_idx].sram_buffer));
    tier->tcb_buffer[tier->tcb_idx].next = tier->tcb_buffer[!tier->tcb_idx].next;
    
    
    // no need to change entry task and size becasue they are fixed no matter which buffer is active
    // tier->tcb_buffer[!tier->tcb_idx].entry = tier->tcb_buffer[tier->tcb_idx].entry;
    // tier->tcb_buffer[!tier->tcb_idx].size = tier->tcb_buffer[tier->tcb_idx].size;

    // no need to change fram_buffer pointer because it is already pointing to the right buffer
    // __copy_buff_sram_to_fram() flips the index and copies to the right location in FRAM
    // tier->tcb_buffer[!tier->tcb_idx].fram_buffer = tier->tcb_buffer[tier->tcb_idx].fram_buffer;
    
    // copy SRAM buffer to scratch buffer
    return __copy_buff_sram_to_fram(tier);
}


