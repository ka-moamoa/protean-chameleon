#include "chameleon.h"

// indicates if this is the first boot
__nv uint8_t _not_first_boot;

// entry function for the application initialization
// applications should implement this function
extern void __app_init();

// application specific things that are to be configured at each reboot
// applications should implement this function
extern void __app_reboot();

int __hwReboot(void) {
    uint8_t isNotFirstBoot = FALSE;
    __NVM_GET(isNotFirstBoot, _not_first_boot);
    
    // if first boot
    if(!isNotFirstBoot) {
        KERNEL_LOG_INFO("First Boot");
        
	    // initialize the application
	    __app_init();

        // initilize all global NVM variables here
        __init_dispatcher();
        __NVM_SET(_not_first_boot, TRUE);

#ifdef Supersensor
        // initialize energy driver's NVM global variable here
        __init_energy_driver();
        if(setThresholdVoltage(DEFAULT_VOLTAGE_THRESHOLD) != E_NO_ERROR) {
            KERNEL_LOG_ERROR("VOLTAGE THRESHOLD CONFIGURATION FAILED");
            while (1);
        }
        else {
            KERNEL_LOG_INFO("voltage threshold changed successfully!");
        }
#endif

    }
    else {
        KERNEL_LOG_INFO("Not First Boot");
    }

    return E_SUCCESS;
}

int main(void)
{
	// will be called at each reboot of the application
	__app_reboot();

	// activate task dispatcher
	__dispatcher_run();

	return 0;
}
