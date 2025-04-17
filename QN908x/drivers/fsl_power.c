/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_power.h"
#include "fsl_iocon.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Handle critical issue bedore/after power down, by executing code in ram.
 *
 * This is an assembly function defined in fsl_power_in_ram.s.
 */
extern void POWER_EnterPowerDownInRam(void);

/*!
 * @brief After waking up from power down, bootloader will jump to this assembly lable.
 *
 * This is a tricky entry point, refer to Software Development Guide for more information.
 */
#if defined(__cplusplus)
extern "C" {
#endif
extern void POWER_WakeupEntry(void);
#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern void *__Vectors;
static uint32_t regPrimaskLowPower = 0U;

/* used to backup/restore current stack information(one for CONTROL and one for R13) */
uint32_t g_StackInfo[2U] = {0U, 0U};

/*******************************************************************************
 * Code
 ******************************************************************************/

void POWER_WritePmuCtrl1(SYSCON_Type *base, uint32_t mask, uint32_t value)
{
    uint32_t reg;

    reg = base->PMU_CTRL1;
    reg &= ~mask;
    reg |= (mask & value);

    base->PMU_CTRL1 = reg;
}

void POWER_EnablePD(pd_bit_t en)
{
    uint32_t regPrimask = 0U;
    uint32_t reg = (en >> 8UL);

    regPrimask = DisableGlobalIRQ();

    if (PMUCTRL0 == reg)
    {
        SYSCON->PMU_CTRL0 |= (1UL << (en & 0xff));
    }
    else /* PMUCTRL1 */
    {
        POWER_WritePmuCtrl1(SYSCON, 1UL << (en & 0xff), 1UL << (en & 0xff));
    }

    EnableGlobalIRQ(regPrimask);
}

void POWER_DisablePD(pd_bit_t en)
{
    uint32_t regPrimask = 0U;
    uint32_t reg = (en >> 8UL);

    regPrimask = DisableGlobalIRQ();

    if (PMUCTRL0 == reg)
    {
        SYSCON->PMU_CTRL0 &= ~(1UL << (en & 0xff));
    }
    else /* PMUCTRL1 */
    {
        POWER_WritePmuCtrl1(SYSCON, 1UL << (en & 0xff), ~(1UL << (en & 0xff)));
    }

    EnableGlobalIRQ(regPrimask);
}

void POWER_EnableDCDC(bool flag)
{
    if (flag)
    {
        POWER_WritePmuCtrl1(SYSCON, SYSCON_PMU_CTRL1_BUCK_CTRL_MASK, SYSCON_PMU_CTRL1_BUCK_CTRL(0U));
    }
    else
    {
        POWER_WritePmuCtrl1(SYSCON, SYSCON_PMU_CTRL1_BUCK_CTRL_MASK, SYSCON_PMU_CTRL1_BUCK_CTRL(0xFU));
    }
}

void POWER_EnableADC(bool flag)
{
    volatile uint32_t delayX;
    uint32_t loopCount = (100 * SystemCoreClock) / (1000000 * 13); // (time to wait in 100us)/(time per loop)
    if (flag)
    {
        POWER_DisablePD(kPDRUNCFG_PD_ADC_BUF);
        POWER_DisablePD(kPDRUNCFG_PD_ADC_BG);
        POWER_DisablePD(kPDRUNCFG_PD_ADC);
        POWER_DisablePD(kPDRUNCFG_PD_ADC_VCM);
        POWER_DisablePD(kPDRUNCFG_PD_ADC_VREF);
        /* 100us for ADC stable */
        for (delayX = 0; delayX < loopCount; delayX++)
        {
        }
    }
    else
    {
        POWER_EnablePD(kPDRUNCFG_PD_ADC_BUF);
        POWER_EnablePD(kPDRUNCFG_PD_ADC_BG);
        POWER_EnablePD(kPDRUNCFG_PD_ADC);
        POWER_EnablePD(kPDRUNCFG_PD_ADC_VCM);
        POWER_EnablePD(kPDRUNCFG_PD_ADC_VREF);
    }
}

void POWER_LatchIO(void)
{
    /* Capture GPIO's DATAOUT and OUTENSET to PIO_CAP_OE & PIO_CAP_OUT */
    SYSCON->IO_CAP = SYSCON_IO_CAP_PIN_RETENTION_MASK;

    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk))
    {
        /* Hand over control */
        SYSCON->PIO_WAKEUP_EN1 |= SYSCON_PIO_WAKEUP_EN1_PDM_IO_SEL_MASK;
    }
}

void POWER_RestoreIO(void)
{
    /* gpio registers get lost during power down, restore from captured values */
    GPIOA->DATAOUT = SYSCON->PIO_CAP_OUT0;
    GPIOA->OUTENSET = SYSCON->PIO_CAP_OE0;
    GPIOB->DATAOUT = SYSCON->PIO_CAP_OUT1;
    GPIOB->OUTENSET = SYSCON->PIO_CAP_OE1;

    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk))
    {
        SYSCON->PIO_WAKEUP_EN1 &= ~SYSCON_PIO_WAKEUP_EN1_PDM_IO_SEL_MASK;
    }
}

void POWER_EnableSwdWakeup(void)
{
    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk))
    {
        /* Switch PA23's function (SWDIO -> GPIO), the debugger can wakeup chip through PA23 */
        IOCON_PinMuxSet(IOCON, 0U, SWDIO_GPIO_PIN, IOCON_FUNC1 | IOCON_MODE_PULLUP);

        /* Read PA23 and set the opposite level as wakeup level */
        SYSCON->PIO_WAKEUP_LVL0 &= ~SWDIO_GPIO_PIN_MASK;
        SYSCON->PIO_WAKEUP_LVL0 |= (GPIOA->DATA & SWDIO_GPIO_PIN_MASK);

        /* Enable PA23 as wakeup source */
        SYSCON->PIO_WAKEUP_EN0 |= SWDIO_GPIO_PIN_MASK;

        /* disable swd */
        SYSCON->MISC &= ~SYSCON_MISC_EN_SWD_MASK;
    }
}

void POWER_RestoreSwd(void)
{
    if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk))
    {
        /* restore PA23 as SWDIO */
        IOCON_PinMuxSet(IOCON, 0U, SWDIO_GPIO_PIN, IOCON_FUNC0 | IOCON_MODE_PULLUP);

        /* Disable PA23 wakeup */
        SYSCON->PIO_WAKEUP_EN0 &= ~SWDIO_GPIO_PIN_MASK;

        /* re-enable swd */
        SYSCON->MISC |= SYSCON_MISC_EN_SWD_MASK;
    }
}

bool POWER_GpioActiveRequest(void)
{
    uint8_t result = false;
    uint32_t iowake = SYSCON->PIO_WAKEUP_EN0;
    uint32_t n_wake_level = SYSCON->PIO_WAKEUP_LVL0;
    uint32_t real_level = GPIOA->DATA;

    /* for GPIOA */
    if ((n_wake_level ^ real_level) & iowake)
    {
        result = true;
    }
    else /* for GPIOB */
    {
        iowake = SYSCON->PIO_WAKEUP_EN1 &
                 (SYSCON_PIO_WAKEUP_EN1_PB00_WAKEUP_EN_MASK | SYSCON_PIO_WAKEUP_EN1_PB01_WAKEUP_EN_MASK |
                  SYSCON_PIO_WAKEUP_EN1_PB02_WAKEUP_EN_MASK);
        n_wake_level = SYSCON->PIO_WAKEUP_LVL1;
        real_level = GPIOB->DATA;

        if ((n_wake_level ^ real_level) & iowake)
        {
            result = true;
        }
    }

    return result;
}

void POWER_PreEnterLowPower(void)
{
    regPrimaskLowPower = DisableGlobalIRQ();
}

void POWER_PostExitLowPower(void)
{
    EnableGlobalIRQ(regPrimaskLowPower);
}

void POWER_EnterSleep(void)
{
    /* Errata FC.1 */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    /* Work around for dvreg11 */
    /* The regulator output voltage change to 1.1v to fix the hardfault issue which is caused by unstable regulator output voltage */
    SYSCON->ANA_CTRL1 =
        (SYSCON->ANA_CTRL1 & ~SYSCON_ANA_CTRL1_DVREG11_SET_DIG_MASK) | SYSCON_ANA_CTRL1_DVREG11_SET_DIG(0x2U);

    __WFI();

    /* Work around for dvreg11 */
    /* SYSCON->ANA_CTRL1 =
        (SYSCON->ANA_CTRL1 & ~SYSCON_ANA_CTRL1_DVREG11_SET_DIG_MASK) | SYSCON_ANA_CTRL1_DVREG11_SET_DIG(0x2U); */
}

#if (defined(__GNUC__) || defined (__MCUXPRESSO))
void __attribute__((optimize("O0"))) POWER_EnterPowerDown(uint32_t exclude_from_pd)
#else
void POWER_EnterPowerDown(uint32_t exclude_from_pd)
#endif
{
    uint32_t backISER[2U];

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    SYSCON->PMU_CTRL0 |= (SYSCON_PMU_CTRL0_PMU_EN_MASK | SYSCON_PMU_CTRL0_WAKEUP_EN_MASK);

    /* Work around for dvreg11 and pmu voltage */
    SYSCON->ANA_CTRL1 =
        (SYSCON->ANA_CTRL1 & ~(SYSCON_ANA_CTRL1_DVREG11_SET_DIG_MASK | SYSCON_ANA_CTRL1_VDD_PMU_SET_MASK)) |
            SYSCON_ANA_CTRL1_DVREG11_SET_DIG(0x0) | SYSCON_ANA_CTRL1_VDD_PMU_SET(0x1);

    /* Backup useful NVIC registers, because they are lost after waking from power down */
    backISER[0U] = NVIC->ISER[0U];
    backISER[1U] = NVIC->ISER[1U];

    /* Flash is shut down during power down, so after waking up, execute code from RAM
     * until flash becomes ready (that takes 7us) */
    POWER_EnterPowerDownInRam();

    /* Disable interrupt response (since primask will lost in PD mode) */
    DisableGlobalIRQ();

#if defined(CFG_BLE_PRJ)
    /* XTAL is on now, enable ble core's clock ASAP */
    CLOCK_EnableClock(kCLOCK_Ble);
#endif

#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1)) || (defined(__VFP_FP__) && !defined(__SOFTFP__))
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10, CP11 Full Access */
#endif

    SCB->VTOR = (uint32_t)&__Vectors;

    /* All registers in NVIC are lost, restore from backup values */
    NVIC->ISER[0U] = backISER[0U];
    NVIC->ISER[1U] = backISER[1U];

    /* Work around for dvreg11 and pmu voltage, wait signal of Vreg_D ready, and then change analog setting */
    while (!(SYSCON->SYS_MODE_CTRL & (1U << 24U)))
    {
    }
    SYSCON->ANA_CTRL1 =
        (SYSCON->ANA_CTRL1 & ~(SYSCON_ANA_CTRL1_DVREG11_SET_DIG_MASK | SYSCON_ANA_CTRL1_VDD_PMU_SET_MASK)) |
            SYSCON_ANA_CTRL1_DVREG11_SET_DIG(0x2) | SYSCON_ANA_CTRL1_VDD_PMU_SET(0x3);

    SYSCON->PMU_CTRL0 &= ~(SYSCON_PMU_CTRL0_PMU_EN_MASK | SYSCON_PMU_CTRL0_WAKEUP_EN_MASK);
}

#if defined(__CC_ARM)
__asm void __attribute__((section(".textrw"))) POWER_EnterPowerDownInRam(void)
{
    EXPORT  POWER_WakeupEntry
    EXTERN  g_StackInfo
    MRS    R2, PSP
    MRS    R3, MSP

    PUSH   {R2-R11, LR}

    LDR    R0, =g_StackInfo
    MRS    R1, CONTROL
    MOV    R2, R13
    STRD   R1, R2, [R0]

    WFI

POWER_WakeupEntry
    LDR    R1, =0x40000014
    LDR    R0, [R1]
    LDR    R2, =0x1
    ORR    R0, R0, R2
    STR    R0, [R1]

    LDR    R0, =g_StackInfo
    LDRD   R1, R2, [R0]
    MSR    CONTROL, R1
    MOV    R13, R2

    POP    {R2-R11, LR}

    MSR    MSP, R3
    MSR    PSP, R2

    MOV    R1,  #0
    MOV    R0,  #0xFF
loop    
    ISB
    ADD    R1,  R1,  #1
    CMP    R1,  R0
    BCC    loop

    BX     LR
    ALIGN
}
#elif defined(__GNUC__)
void __attribute__((section(".ramfunc"))) POWER_EnterPowerDownInRam(void)
{
    __asm volatile (
    "    PUSH   {R0-R3}              \n"
    "    MRS    R2, PSP              \n"
    "    MRS    R3, MSP              \n"
    "    PUSH   {R2-R11, LR}         \n"
    "    LDR    R0, =g_StackInfo     \n"
    "    MRS    R1, CONTROL          \n"
    "    MOV    R2, R13              \n"
    "    STRD   R1, R2, [R0]         \n"
    "    WFI                         \n"
    "    B      POWER_WakeupEntry    \n"
    );
}

void __attribute__((section(".ramfunc"))) POWER_WakeupEntry(void)
{
    __asm volatile (
    "    LDR    R1, =0x40000014      \n"
    "    LDR    R0, [R1]             \n"
    "    LDR    R2, =0x1             \n"
    "    ORR    R0, R0, R2           \n"
    "    STR    R0, [R1]             \n"

    "    LDR    R0, =g_StackInfo     \n"
    "    LDRD   R1, R2, [R0]         \n"
    "    MSR    CONTROL, R1          \n"
    "    MOV    R13, R2              \n"
    "    POP    {R2-R11, LR}         \n"

    "    MSR    MSP, R3              \n"
    "    MSR    PSP, R2              \n"

    "    MOV    R1,  #0              \n"
    "    MOV    R0,  #0xFF           \n"
    "loop:                           \n"
    "    ISB                         \n"
    "    ADD    R1,  R1,  #1         \n"
    "    CMP    R1,  R0              \n"
    "    BCC    loop                 \n"
    "    POP    {R0-R3}              \n"
    );
}
#elif defined(__ICCARM__)
__ramfunc void POWER_EnterPowerDownInRam(void)
{
    __ASM volatile (
    "    MRS    R2, PSP           \n"
    "    MRS    R3, MSP           \n"
    "    PUSH   {R2-R11, LR}      \n"
    "    MRS    R1, CONTROL       \n"
    "    MOV    R2, R13           \n"
    );
    __ASM volatile (
    "    STRD   R1, R2, [%0]" : : "r"(g_StackInfo)
    );
    __ASM volatile (
    "    WFI                      \n"
    "    B      POWER_WakeupEntry \n"
    );
}

__ramfunc void POWER_WakeupEntry(void)
{
    __ASM volatile (
    "    MOV    R2, %0" : : "i"(0x01)
    );
    __ASM volatile (
    "    MOV    R1, %0" : : "r"(0x40000014)
    );
    __ASM volatile (
    "    LDR    R0, [R1]          \n"
    "    ORR    R0, R0, R2        \n"
    "    STR    R0, [R1]          \n"
    );

    __ASM volatile (
    "    LDRD   R1, R2, [%0]" : : "r"(g_StackInfo)
    );
    __ASM volatile (
    "    MSR    CONTROL, R1       \n"
    "    MOV    R13, R2           \n"

    "    POP    {R2-R11, LR}      \n"
    "    MSR    MSP, R3           \n"
    "    MSR    PSP, R2           \n"
    "    MOV    R1,  #0           \n"
    "    MOV    R0,  #0xFF        \n"
    "loop:                        \n"
    "    ISB                      \n"
    "    ADD    R1,  R1,  #1      \n"
    "    CMP    R1,  R0           \n"
    "    BCC    loop              \n"
    "    BX     LR                \n"
    );
}
#endif

void POWER_Init(void)
{
    POWER_RegisterWakeupEntry((uint32_t)POWER_WakeupEntry);

    /* Save power in active mode */
    SYSCON->CLK_CTRL = (SYSCON->CLK_CTRL & ~SYSCON_CLK_CTRL_CGBYPASS_MASK) | SYSCON_CLK_CTRL_CGBYPASS(0x0U);

    /* power down flash during power down modes */
    SYSCON->PMU_CTRL2 = SYSCON->PMU_CTRL2 | SYSCON_PMU_CTRL2_FLSH_PDM_DIS_MASK;

    SYSCON->MISC = (SYSCON->MISC & ~SYSCON_MISC_RCO_PWR_MODE_MASK) | SYSCON_MISC_RCO_PWR_MODE(0x2U);
}
