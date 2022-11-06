#include "dispatcher.h"

// all tiers of the application
static __nv tier_t _tiers[MAX_TIERS];
static tier_t _sram_tiers[MAX_TIERS];

// current index of the tier being executed
static __nv uint8_t _curr_tier_level;

// number of tiers initialized
static __nv uint8_t _tiers_used;

#if (ADAPTIVE_CAP_STORAGE || MULTI_TIER_EXECUTION)
extern voltage_threshold_t _new_voltage_threshold;
extern voltage_threshold_t _prev_voltage_threshold;

extern __nv float _energy_window[WINDOW_SIZE];
extern float _sram_energy_window[WINDOW_SIZE];

extern __nv uint8_t _window_index;
extern uint8_t _sram_window_index;
#endif

// Assigns a slot to a tier. Should be called ONLY at the first boot
void __create_tier(uint8_t tier_level, void *entry, void *fram_buff, void *sram_buff, uint32_t size) {
    if ((tier_level >= 0) && (tier_level < MAX_TIERS)) {
        tier_t temp_tier;
        uint8_t tiers_used;

        temp_tier.tcb_idx = 0;
        temp_tier.tcb_buffer[0].entry = entry;
        temp_tier.tcb_buffer[0].next = entry;
        temp_tier.tcb_buffer[0].sram_buffer = sram_buff;
        temp_tier.tcb_buffer[0].fram_buffer = fram_buff; // fram_buff[0]
        temp_tier.tcb_buffer[0].size = size;
        temp_tier.tcb_buffer[1].entry = entry;
        temp_tier.tcb_buffer[1].next = entry;
        temp_tier.tcb_buffer[1].sram_buffer = sram_buff;
        temp_tier.tcb_buffer[1].fram_buffer = fram_buff + size; // fram_buff[1]
        temp_tier.tcb_buffer[1].size = size;
        __NVM_SET(_tiers[tier_level], temp_tier);
        KERNEL_LOG_DEBUG("added tier %u", tier_level + 1);

        __NVM_GET(tiers_used, _tiers_used);
        tiers_used++;
        __NVM_SET(_tiers_used, tiers_used);
    }
    else {
        KERNEL_LOG_ERROR("INVALID TIER LEVEL: %u", tier_level + 1);
    }
}

inline uint8_t __next_tier(uint8_t tier_level) {
    uint8_t tiers_used;
    __NVM_GET(tiers_used, _tiers_used);

    KERNEL_LOG_DEBUG("current tier: %u", tier_level + 1);

    // TODO
    // use appropriate API here to get predicted energy
    float predicted_energy = getPredictedEnergy();

    // change the tier based on predictied energy

    /*  predictied energy and tier relationship
        high energy --> go for the highest tier
        medium energy --> go for any tier between MAX_TIERS/3 and 2*MAX_TIERS/3
        low energy --> go for the lowest tier
    */

    // assumption 1: if a lower tier is defined, the higher tiers must be defined, converse might not be true
    // assumption 2: tiers should be continuous, there shouldn't be any jumps (1, 2, 3 is okay. 1, 3, 5 is not okay)
    
    if (IS_HIGH_ENERGY(predicted_energy)) {
        KERNEL_LOG_INFO("high energy: %f", predicted_energy);

        if ((tier_level >= 0) && (tier_level < MAX_TIERS)) {
            if(tier_level > 0) {
                // high energy + any tier --> adapt up
                // no need to check if the higher tier exists becuase if a lower tier is defined, the higher tiers must be defined
                ADAPT_UP(tier_level);
                KERNEL_LOG_DEBUG("adapt up");
                KERNEL_LOG_DEBUG("change to tier: %u", tier_level + 1);
            }
            else {
                KERNEL_LOG_DEBUG("do not change tier because it doesn't exist");
            }
        }
        else {
            KERNEL_LOG_ERROR("INVALID TIER LEVEL: %u", tier_level + 1);
        }
    }
    else if (IS_MED_ENERGY(predicted_energy)) {
        KERNEL_LOG_INFO("medium energy: %f", predicted_energy);

        if ((tier_level >= 0) && (tier_level < MAX_TIERS/3)) {
            // medium energy + high tier --> adapt down
            // check if the lower tier exists becuase if a higher tier is defined, the lower tiers might not be defined
            if (tiers_used > tier_level + 1) {
                ADAPT_DOWN(tier_level);
                KERNEL_LOG_DEBUG("adapt down");
                KERNEL_LOG_DEBUG("change to tier: %u", tier_level + 1);
            }
            else {
                KERNEL_LOG_DEBUG("do not change tier because it doesn't exist");
            }
        }
        else if ((tier_level >= MAX_TIERS/3) && (tier_level < 2*(MAX_TIERS/3))) {
            KERNEL_LOG_DEBUG("do not change tier");
            // medium energy + medium tier --> no need to change the tier
        }
        else if ((tier_level >= 2*(MAX_TIERS/3)) && (tier_level < MAX_TIERS)) {
            // medium energy + low tier --> adapt up
            // no need to check if the higher tier exists becuase if a lower tier is defined, the higher tiers must be defined
            ADAPT_UP(tier_level);
            KERNEL_LOG_DEBUG("adapt up");
            KERNEL_LOG_DEBUG("change to tier: %u", tier_level + 1);
        }
        else {
            KERNEL_LOG_ERROR("INVALID TIER LEVEL: %u", tier_level + 1);
        }
    }
    else { // LOW ENERGY
        KERNEL_LOG_INFO("low energy: %f", predicted_energy);
        if ((tier_level >= 0) && (tier_level < MAX_TIERS)) {
            // low energy + any tier --> adapt down
            // check if the lower tier exists becuase if a higher tier is defined, the lower tiers might not be defined
            if (tiers_used > tier_level + 1) {
                ADAPT_DOWN(tier_level);
                KERNEL_LOG_DEBUG("adapt down");
                KERNEL_LOG_DEBUG("change to tier: %u", tier_level + 1);
            }
            else {
                KERNEL_LOG_DEBUG("do not change tier because it doesn't exist");
            }
        }
        else {
            KERNEL_LOG_ERROR("INVALID TIER LEVEL: %u", tier_level + 1);
        }
    }
    
    return tier_level;
}

inline uint8_t __get_curr_tier_level() {
    uint8_t tier_level;
    __NVM_GET(tier_level, _curr_tier_level);
    return tier_level;
}

inline tier_t *__fetch_tier(uint8_t tier_level) {
    KERNEL_LOG_DEBUG("fetching tier: %u", tier_level + 1);
    __NVM_GET(_sram_tiers[tier_level], _tiers[tier_level]);
    return &_sram_tiers[tier_level];
}

inline void __commit_tier(uint8_t tier_level) {
#if (ADAPTIVE_CAP_STORAGE || MULTI_TIER_EXECUTION)
    __NVM_MEMSET(_energy_window, _sram_energy_window, (WINDOW_SIZE * sizeof(float)));
    __NVM_SET(_window_index, _sram_window_index);
#endif

#if ADAPTIVE_CAP_STORAGE
    // TODO 
    // predict energy availability
    // set upper voltage threshold of capacitor

    if (_new_voltage_threshold != _prev_voltage_threshold) {
        _prev_voltage_threshold = _new_voltage_threshold;
        if(setThresholdVoltage(_new_voltage_threshold) != E_NO_ERROR) {
            KERNEL_LOG_ERROR("VOLTAGE THRESHOLD CONFIGURATION FAILED");
            while (1);
        }
        else {
            KERNEL_LOG_INFO("voltage threshold changed successfully!");
        }
    }
#endif

    __NVM_SET(_tiers[tier_level], _sram_tiers[tier_level]);

#if MULTI_TIER_EXECUTION
    // TODO 
    // predict energy availability
    // change tier index if needed
    // fetch the corresponding tier

    uint8_t new_tier_level = __next_tier(tier_level);
    if (new_tier_level != tier_level){
        // tier has changed, write back to FRAM
        __NVM_SET(_curr_tier_level, new_tier_level);
    }
#endif
}

void __dispatcher_run() {
    uint8_t curr_tier_level = __get_curr_tier_level();
    tier_t *curr_tier = __fetch_tier(curr_tier_level);

    while (1) {
        void *entry_task = (void *)((task_t) curr_tier->tcb_buffer[curr_tier->tcb_idx].entry);
        void *next_task = (void *)((task_t) curr_tier->tcb_buffer[curr_tier->tcb_idx].next);
        if(entry_task && next_task) {
            // TODO 
            // not changing voltage thresholds here because of a weird HW bug
            // capacitor starts decaying slowly the moment voltage threshold circuit is controlled
            // even if same voltage threshold is set again and again, the decay is slower
            // this gives unwanted/unexplainable adavandage to dynamic thresholding approach
            // therefore we change the thresholds only in __commit_tier() i.e., when power failure is approaching
            
            if(__tick(curr_tier)) {
                KERNEL_LOG_DEBUG("executed task");
                curr_tier->tcb_idx = !(curr_tier->tcb_idx);
                __commit_tier(curr_tier_level);
                KERNEL_LOG_DEBUG("committed to %u", curr_tier->tcb_idx);
            }
            else {
                // curr_tier->tcb_idx = !(curr_tier->tcb_idx);
                KERNEL_LOG_DEBUG("executed task");
                KERNEL_LOG_DEBUG("not committed");
            }
        }
        else {
            KERNEL_LOG_DEBUG("sleeping...");
            MXC_LP_EnterSleepMode();
        }
    }
}


// initialize all dispatcher global SRAM/FRAM variable here
// called at first boot only
void __init_dispatcher() {
    // dispatcher should always start with tier 1 
    uint8_t tier_level = 0; // tier 1
    __NVM_SET(_curr_tier_level, tier_level);
}