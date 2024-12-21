/**
 * @file BSP.c
 *
 * @brief Board support package header file for a CY8CPROTO-063-BLE dev board.
 * The board contains CYBLE-416045-02 chip based on 
 * PSoC 6 MCU CY8C6347BZI-BLD53
 * 
 * @version 0.1.0
 */

#include "cy_pdl.h"
#include "BSP.h"
#include "cycfg.h"
#include "qpc.h"

////////////////////////
// Private data
////////////////////////
static cyhal_uart_t uartObj;
static bspUartRxCallback rxCallback;

//////////////////////////
// Functions prototype
//////////////////////////
static void uart_event_callback_(void *callback_arg, cyhal_uart_event_t event);
static cy_rslt_t uart_readFifo_(uint8_t *data, size_t *len);

#if defined(Q_UTEST)
void BSP_test_init(void);
#endif  // Q_UTEST

/////////////////////////
// Code
//////////////////////////

#if BSP_ENABLE_PLL_CONFIG == true
/**
 * @brief      Init board: power, clocks
 * 
 * @param[in]  pSettings  board settings
 * 
 * @retval     bsp_status_init_t
 */
bsp_status_init_t BSP_init_board(bsp_board_init_t* pSettings) 
{
    if (BSP_power_init() != CY_SYSPM_SUCCESS) {
        return bsp_status_init_fail;
    }

    BSP_clock_setWaitStates();

    if (BSP_clock_wcoInit() != CY_SYSCLK_SUCCESS) {
        return bsp_status_init_fail;
    }

    if (pSettings->mainOscilSrc == bsp_main_oscil_src_ECO) {
        if (BSP_clock_ecoInit() != CY_SYSCLK_SUCCESS) {
            return bsp_status_init_fail;
        }
    }

    if (BSP_clock_hifclkInit(pSettings) != CY_SYSCLK_SUCCESS) {
        return bsp_status_init_fail;
    }

    return bsp_status_init_success;
}
#endif  //BSP_ENABLE_PLL_CONFIG

/**
 * @brief   Initialize board power domain
 * 
 * @details Use Linear core regulator:
 *           - 1.1 V mode;
 *           - set high current/normal mode;
 * 
 * @note    Consider using of Buck converter instaed of LDO in Power save modes (Deep Sleep etc.)
 *          Typically firmware set LDO as default regulator at start up
 * 
 * @param   None
 * 
 * @retval  cy_en_syspm_status_t
 */
cy_en_syspm_status_t BSP_power_init(void) 
{
    cy_en_syspm_status_t status = CY_SYSPM_SUCCESS;

    status = Cy_SysPm_LdoSetMode(CY_SYSPM_LDO_MODE_NORMAL);

    QS_TEST_PROBE_DEF(&BSP_power_init)
    QS_TEST_PROBE(
        status = qs_tp_;
    )

    if (CY_SYSPM_SUCCESS == status) {
        status = Cy_SysPm_LdoSetVoltage(CY_SYSPM_LDO_VOLTAGE_1_1V);
    }

    return status;
}

/**
 * @brief  Set wait states for ROM, SRAM, FLASH before setting HIFCLK
 * 
 * @param  None
 * 
 * @retval None
 */
void BSP_clock_setWaitStates(void) 
{
    Cy_SysLib_SetWaitStates(false, CLOCK_CLK_FAST_HZ / CLOCK_HZ_IN_MHZ);
}


/**
 * @brief  Init WCO 32 kHz clock
 * 
 * @param  None
 * 
 * @note   External Crystal 32768 Hz (ECS-.327-12.5-34B-TR) 
 *         is installed on the Dev Board. 
 * 
 * @retval cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_wcoInit(void) 
{
    cy_en_sysclk_status_t status;

    Cy_SysLib_ResetBackupDomain();
    Cy_SysClk_IloDisable();
    Cy_SysClk_IloEnable();

    Cy_SysPm_UnlockPmic();
	Cy_SysPm_DisablePmicOutput();

    Cy_GPIO_Pin_FastInit(GPIO_WCO_IN_PORT, GPIO_WCO_IN_PIN, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO); 
    Cy_GPIO_Pin_FastInit(GPIO_WCO_OUT_PORT, GPIO_WCO_OUT_PIN, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
    Cy_SysClk_WcoBypass(CY_SYSCLK_WCO_NOT_BYPASSED);
    status = Cy_SysClk_WcoEnable(CLOCK_WCO_ENABLE_TIMEOUT);

    QS_TEST_PROBE_DEF(&BSP_clock_wcoInit)
    QS_TEST_PROBE(
        status = qs_tp_;
    )

    if (CY_SYSCLK_SUCCESS == status) {
        Cy_SysClk_ClkBakSetSource(CY_SYSCLK_BAK_IN_WCO);
    }
#if defined(Q_UTEST)
    Cy_SysClk_ClkBakSetSource(CY_SYSCLK_BAK_IN_WCO); 
#endif //Q_UTEST

    return status;
}

#if BSP_ENABLE_ECO_CONFIG == true
/**
 * @brief  Init ECO (EXTERNAL crystal oscillator) 24 MHz clock
 * 
 * @param  None
 * 
 * @note External oscillator isn't instaled on PCBA by default
 * 
 * @retval cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_ecoInit(void) 
{
    cy_en_sysclk_status_t status;

    Cy_SysClk_EcoDisable();
    Cy_GPIO_Pin_FastInit(GPIO_ECO_IN_PORT, GPIO_ECO_IN_PIN, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO); 
    Cy_GPIO_Pin_FastInit(GPIO_ECO_OUT_PORT, GPIO_ECO_OUT_PIN, CY_GPIO_DM_ANALOG, 0UL, HSIOM_SEL_GPIO);
    status = Cy_SysClk_EcoEnable(CLOCK_ECO_ENABLE_TIMEOUT);

    return status;
}
#endif //BSP_ENABLE_ECO_CONFIG

#if BSP_ENABLE_PLL_CONFIG == true 
/**
 * @brief     Init high frequency clock CLK_HF[0]
 * 
 * @param[in] pSettings board settings
 * 
 * @retval    cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_hifclkInit(bsp_board_init_t* pSettings) 
{
    cy_en_sysclk_status_t status;
    cy_stc_pll_config_t pllConfig = {
        .outputFreq = CLOCK_CLK_FAST_HZ,             /**< PLL output desired frequency in Hz */
        .lfMode     = false,                         /**< Disable low frequency mode (VCO = 200~400 MHz) */
        .outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO   /**< Output 100 MHz when locked. Otherwise 8 MHz */
    };

    if (pSettings->mainOscilSrc == bsp_main_oscil_src_ECO) {
        pllConfig.inputFreq = CLOCK_CLK_ECO_HZ;
    }
    else {
        pllConfig.inputFreq = CLOCK_CLK_IMO_HZ;
    }

    /** Set dividers */
    status = Cy_SysClk_ClkHfSetDivider(CLOCK_HIFCLK0, CY_SYSCLK_CLKHF_NO_DIVIDE);

    if (CY_SYSCLK_SUCCESS == status) { 
        Cy_SysClk_ClkFastSetDivider(0UL);
        Cy_SysClk_ClkSlowSetDivider(1UL);
        Cy_SysClk_ClkPeriSetDivider(0UL);	
    }

    /** PLL config */
    status = Cy_SysClk_PllDisable(CLOCK_CLKPATH1);
    if (CY_SYSCLK_SUCCESS == status) {
        if (pSettings->mainOscilSrc == bsp_main_oscil_src_ECO) {
            status = Cy_SysClk_ClkPathSetSource(CLOCK_CLKPATH1, CY_SYSCLK_CLKPATH_IN_ECO);
        }
        else {
            status = Cy_SysClk_ClkPathSetSource(CLOCK_CLKPATH1, CY_SYSCLK_CLKPATH_IN_IMO);
        }
    }
    /** Configure Path 1 PLL with the settings in pllConfig struct */
    if (CY_SYSCLK_SUCCESS == status) {
        status = Cy_SysClk_PllConfigure(CLOCK_CLKPATH1, &pllConfig);
    }
    /** Enable the Path 1 PLL */
    if (CY_SYSCLK_SUCCESS == status) { 
        status = Cy_SysClk_PllEnable(CLOCK_CLKPATH1, CLOCK_PLL_ENABLE_TIMEOUT);
    }

    if (CY_SYSCLK_SUCCESS == status) { 
        status = Cy_SysClk_ClkHfSetSource(CLOCK_HIFCLK0, CY_SYSCLK_CLKHF_IN_CLKPATH1);
    }

    SystemCoreClockUpdate();

    return status;
}
#endif  //BSP_ENABLE_PLL_CONFIG 

/**
 * @brief  Init red LED
 * 
 * @param  None
 * 
 * @retval None
 */
void BSP_init_led_red(void) 
{
    Cy_GPIO_Pin_FastInit(GPIO_LED_RED_PORT, GPIO_LED_RED_PIN, CY_GPIO_DM_STRONG_IN_OFF, 1UL, HSIOM_SEL_GPIO);
}

/**
 * @brief  Init green LED
 * 
 * @param  None
 * 
 * @retval None
 */
void BSP_init_led_green(void) 
{
    Cy_GPIO_Pin_FastInit(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN, CY_GPIO_DM_STRONG_IN_OFF, 1UL, HSIOM_SEL_GPIO);
}

/**
 * @brief  Toggle red LED
 * 
 * @param  None
 * 
 * @retval None 
 */
void BSP_led_red_toggle(void) 
{
    Cy_GPIO_Inv(GPIO_LED_RED_PORT, GPIO_LED_RED_PIN);
}

/**
 * @brief  On red LED
 * 
 * @param  None
 * 
 * @retval None 
 */
void BSP_led_red_On(void) 
{
    cyhal_gpio_write(P6_3, false);
}

/**
 * @brief  Off red LED
 * 
 * @param  None
 * 
 * @retval None 
 */
void BSP_led_red_Off(void) 
{
    cyhal_gpio_write(P6_3, true);
}

/**
 * @brief  Toggle green LED
 * 
 * @param  None
 * 
 * @retval None 
 */
void BSP_led_green_toggle(void) 
{
    Cy_GPIO_Inv(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN);
}

/**
 * @brief  Green LED on
 *
 * @param  None
 *
 * @retval None
 */
void BSP_led_green_on(void)
{
    cyhal_gpio_write(P7_1, false);
}

/**
 * @brief  Green LED off
 *
 * @param  None
 *
 * @retval None
 */
void BSP_led_green_off(void)
{
    cyhal_gpio_write(P7_1, true);
}

/**
 * @brief  Initialize UART
 * 
 * @details Configrure & enable UART; Setup RX interrupt
 * 
 * @param[in]  callback UART RX callback
 * 
 * @retval 0 - success, otherwise - fail status
 */
bsp_status_init_t BSP_initUart(bspUartRxCallback callback) 
{
    cy_rslt_t result;
    bsp_status_init_t status = bsp_status_init_success;
    rxCallback = callback;

    result = cyhal_uart_init_cfg(&uartObj, &scb_5_hal_config);

    QS_TEST_PROBE_DEF(&BSP_initUart)
    QS_TEST_PROBE(
        result = qs_tp_;
    )
    if(result == CY_RSLT_SUCCESS) {
        cyhal_uart_register_callback(&uartObj, 
                                    uart_event_callback_, 
                                    /*void *callback_arg*/ NULL);
        cyhal_uart_enable_event(&uartObj, 
                                CYHAL_UART_IRQ_RX_NOT_EMPTY, 
                                /* irq prio*/1U, 
                                true);
    } else {
        status = bsp_status_init_fail;
#if defined (Q_UTEST)
        cyhal_uart_register_callback(&uartObj, 
                                    uart_event_callback_, 
                                    /*void *callback_arg*/ NULL);
        cyhal_uart_enable_event(&uartObj, 
                                CYHAL_UART_IRQ_RX_NOT_EMPTY, 
                                /* irq prio*/1U, 
                                true);
#endif //Q_UTEST
    }

    return status;
}

#if defined(Q_UTEST)
void uart_event_callback_test(void)
{
    uart_event_callback_(NULL, CYHAL_UART_IRQ_TX_ERROR);    // not nominal path of execution
}
#endif //Q_UTEST

/**
 * @brief Handles UART interrupts
 * 
 * @param[in] callback_arg  additional callback's args
 * 
 * @param[in] event  UART evts (RX not empty, TX done etc.)
 * 
 * @details HAL & PDL are used for implementing UART communication.
 *          1. _cyhal_uart_irq_handler calls Cy_SCB_UART_Interrupt.
 *             Cy_SCB_UART_Interrupt processes most IRQs .
 *          2. _cyhal_uart_cb_wrapper - warpper callback for UART evts/IRQs
 *             that calls Application callback ( @ref uart_event_callback_  )
 * 
 * @retval None
 */
static void uart_event_callback_(void *callback_arg, cyhal_uart_event_t event) 
{
    uint8_t rxData[16];
    size_t len = 1;

    switch(event) {
        case CYHAL_UART_IRQ_RX_NOT_EMPTY:
        {
            uart_readFifo_(rxData, &len);
            rxCallback(rxData, (uint16_t) len);
            break;
        }
        default:
            break;
    }
}

/**
 * @brief Reads data form UART FIFO
 * 
 * @param[in] data  ptr to buf
 * 
 * @param[in] len   ptr to data len
 * 
 * 
 * @retval 0 - if success, otherwise - failed status
 */
cy_rslt_t uart_readFifo_(uint8_t *data, size_t *len) 
{
    cy_rslt_t result;

    result = cyhal_uart_read(&uartObj, data, len);

    return result;
}

/**
 * @brief  Check if UART TX FIFO is ready to receive data
 * 
 * @param  None
 * 
 * @retval true - UART is ready, false - not ready
 */
bool BSP_isUartTxReady(void) 
{
    uint32_t num = cyhal_uart_writable(&uartObj);

    return (num > 0);
}

#if BSP_ENABLE_UART_EXTENDED_FUNCS == true
/**
 * @brief  Check if UART TX FIFO is empty
 * 
 * @param  None
 * 
 * @retval true - empty, false - not empty
 */
bool BSP_isUartTxEmpty(void) 
{
    uint32_t num = cyhal_uart_writable(&uartObj);

    return (num == 0);
}
#endif //BSP_ENABLE_UART_EXTENDED_FUNCS

/**
 * @brief  Transmit data via UART
 * 
 * @param[in]   data  data buffer
 * 
 * @param[in]   len  length of data
 * 
 * @retval None 
 */
void BSP_uartTxData(uint8_t *data, uint16_t len) 
{
    size_t txLen = (size_t) len;
    cyhal_uart_write(&uartObj, data, (size_t *) &txLen);
}

#if defined(Q_UTEST)
/**
 * @brief  Initi dictionaries for BSP UTs
 * 
 * @param  None
 * 
 * @retval None 
 */
void BSP_initUTdic(void)
{
    QS_FUN_DICTIONARY(BSP_init_led_red);
    QS_FUN_DICTIONARY(BSP_init_led_green);
    QS_FUN_DICTIONARY(BSP_power_init);
    QS_FUN_DICTIONARY(BSP_clock_setWaitStates);
    QS_FUN_DICTIONARY(BSP_clock_wcoInit);
#if BSP_ENABLE_ECO_CONFIG == true
    QS_FUN_DICTIONARY(BSP_clock_ecoInit);
#endif //BSP_ENABLE_ECO_CONFIG
    QS_FUN_DICTIONARY(BSP_led_red_toggle);
    QS_FUN_DICTIONARY(BSP_led_red_On);
    QS_FUN_DICTIONARY(BSP_led_red_Off);
    QS_FUN_DICTIONARY(BSP_led_green_toggle);
    QS_FUN_DICTIONARY(BSP_isUartTxReady);
#if BSP_ENABLE_UART_EXTENDED_FUNCS == true
    QS_FUN_DICTIONARY(BSP_isUartTxEmpty);
#endif  //BSP_ENABLE_UART_EXTENDED_FUNCS
    QS_FUN_DICTIONARY(BSP_initUart);
    QS_FUN_DICTIONARY(uart_event_callback_test);

    BSP_test_init();
}
#endif //Q_UTEST

/************************** END OF FILE *****************************************/
