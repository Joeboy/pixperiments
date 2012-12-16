#include "pi_hardware.h"

extern void PUT32 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );

void reboot()
{
    unsigned int pm_rstc,pm_wdog;
    pm_rstc = GET32(PM_RSTC);
    pm_wdog = PM_PASSWORD | (1 & PM_WDOG_TIME_SET); // watchdog timer = timer clock / 16; need password (31:16) + value (11:0)
    pm_rstc = PM_PASSWORD | (pm_rstc & PM_RSTC_WRCFG_CLR) | PM_RSTC_WRCFG_FULL_RESET;
    PUT32(PM_WDOG,pm_wdog);
    PUT32(PM_RSTC,pm_rstc);
}
