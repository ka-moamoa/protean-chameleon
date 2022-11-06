#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "tier.h"
#include "nvm.h"
#include "lp.h"

#ifdef Supersensor
#include "energy.h"
#endif

void __create_tier(uint8_t tier_level, void *entry, void *fram_buff, void *sram_buff, uint32_t size);
void __init_dispatcher();
void __dispatcher_run();

#endif /* DISPATCHER_H_ */
