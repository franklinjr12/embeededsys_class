/******************************************************************************
 * @file     system_TM4C1294_tw.c (based on system_TM4C129.c) - TivaWare version
 * @brief    CMSIS Device System Source File for
 *           TI TIVA TM4C129 Snowflake Device Series
 * @version  V1.00
 * @date     11. August 2019 (modified by Hugo Vieira Neto)
 ******************************************************************************/

#include <stdint.h>
#include "TM4C1294NCPDT.h"
#include <stdbool.h>
#include "driverlib/sysctl.h"

/*----------------------------------------------------------------------------
  Defines for clock configuration -- user defined
 *----------------------------------------------------------------------------*/
#define __CONFIG_CLOCK                                                        \
    (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480)
#define __SYSTEM_CLOCK    (120000000UL)


/*----------------------------------------------------------------------------
  Externals
 *----------------------------------------------------------------------------*/
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  extern uint32_t __Vectors;
#endif


/*----------------------------------------------------------------------------
  System Core Clock variable
 *----------------------------------------------------------------------------*/
__no_init uint32_t SystemCoreClock; // System Core Clock Frequency


/*----------------------------------------------------------------------------
  System Core Clock update function
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate(void){
  /* TODO: Updates required to fully work with TM4C1294 series devices */
} // SystemCoreClockUpdate


/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit(void){
  #if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
    SCB->VTOR = (uint32_t) &__Vectors;
  #endif

  #if defined (__FPU_USED) && (__FPU_USED == 1U)
    SCB->CPACR |= ((3UL << 10*2) |  // enable CP10 Full Access
                   (3UL << 11*2));  // enable CP11 Full Access
  #endif

  #ifdef UNALIGNED_SUPPORT_DISABLE
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
  #endif

  // Clock configuration
  SystemCoreClock = SysCtlClockFreqSet(__CONFIG_CLOCK, __SYSTEM_CLOCK);
} // SystemInit
