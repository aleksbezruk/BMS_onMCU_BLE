/*******************************************************************************
 * \file BSP.h
 * \version 0.1.0
 *
 * Board support package header file for a CY8CPROTO-063-BLE dev board.
 * The board contains CYBLE-416045-02 chip based on 
 * PSoC 6 MCU CY8C6347BZI-BLD53
 ********************************************************************************/

#ifndef __BSP_H__
#define __BSP_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/** GPIO section*/
#define GPIO_LED_RED_PORT GPIO_PRT6
#define GPIO_LED_RED_PIN 3U

#define GPIO_LED_GREEN_PORT GPIO_PRT7
#define GPIO_LED_GREEN_PIN 1U

#define GPIO_WCO_IN_PORT GPIO_PRT0
#define GPIO_WCO_IN_PIN 0U

#define GPIO_WCO_OUT_PORT GPIO_PRT0
#define GPIO_WCO_OUT_PIN 1U


#define GPIO_ECO_IN_PORT GPIO_PRT12
#define GPIO_ECO_IN_PIN 6U

#define GPIO_ECO_OUT_PORT GPIO_PRT12
#define GPIO_ECO_OUT_PIN 7U

/** Clock section */
#define CLOCK_CLKPATH1           1U
#define CLOCK_HIFCLK0            0U
#define CLOCK_CLK_FAST_HZ        100000000U
#define CLOCK_HZ_IN_MHZ          1000000U
#define CLOCK_CLK_ECO_HZ         24000000U
#define CLOCK_CLK_IMO_HZ         8000000U
#define CLOCK_WCO_ENABLE_TIMEOUT 5000000U /// microseconds
#define CLOCK_ECO_ENABLE_TIMEOUT 20000U /// microseconds
#define CLOCK_PLL_ENABLE_TIMEOUT 20000U /// microseconds

#define BSP_TICKS_PER_SEC 1000U

/** BSP -> project config */
#define BSP_ENABLE_ECO_CONFIG           false
#define BSP_ENABLE_PLL_CONFIG           false
#define BSP_ENABLE_UART_EXTENDED_FUNCS  false

/*******************************************************************************
 * Types
 ******************************************************************************/
/** BSP general satus */
typedef enum {
    bsp_status_init_success,
    bsp_status_init_fail
} bsp_status_init_t;

/** 
 * Main iscillator source
 * Note: ECO isn't installed on CYBLE-063 board, only footprint is available
 */
typedef enum {
   bsp_main_oscil_src_IMO,
   bsp_main_oscil_src_ECO
} bsp_main_oscil_src_t;

/** Board init data structure */
typedef struct {
    bsp_main_oscil_src_t mainOscilSrc;
} bsp_board_init_t;

/** UART RX callback */
typedef void (*bspUartRxCallback)(uint8_t *data, uint16_t len);

/*******************************************************************************
 * API
 ******************************************************************************/
 
/*----------------------------------------*/
/** Board system periph init APIs section */
/*----------------------------------------*/
/**
 * @fn         BSP_init_board.
 * @brief      Init board: power, clocks.
 * @param[in]  pSettings - board settings
 * @retval     bsp_status_init_t
 */
bsp_status_init_t BSP_init_board(bsp_board_init_t* pSettings);

/**
 * @fn     BSP_init_led_red.
 * @brief  Init red LED.
 * @param  None
 * @retval None
 */
void BSP_init_led_red(void);

/**
 * @fn     BSP_init_led_green.
 * @brief  Init green LED.
 * @param  None
 * @retval None
 */
void BSP_init_led_green(void);
 
/*----------------------------------*/
/** Board power domain APIs section */
/*----------------------------------*/
/**
 * @fn      BSP_power_init.
 * @brief   Initialize board power domain.
 * @details Use Linear core regulator:
 *           - 1.1 V mode;
 *           - set high current/normal mode;
 * @note    Consider using of Buck converter instaed of LDO in Power save modes (Deep Sleep etc.)
 *          Typically firmware set LDO as default regulator at start up
 * @param   None
 * @retval  cy_en_syspm_status_t
 */
cy_en_syspm_status_t BSP_power_init(void);

/*----------------------------------*/
/** Board clock domain APIs section */
/*----------------------------------*/
/**
 * @fn     BSP_clock_setWaitStates.
 * @brief  Set wait states for ROM, SRAM, FLASH before setting HIFCLK.
 * @param  None
 * @retval None
 */
void BSP_clock_setWaitStates(void);

/**
 * @fn     BSP_clock_wcoInit.
 * @brief  Init WCO 32 kHz clock.
 * @param  None
 * 
 * @details External Crystal 32768 Hz (ECS-.327-12.5-34B-TR) 
 * is installed on the Dev Board. 
 * 
 * @retval cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_wcoInit(void);

/**
 * @fn     BSP_clock_ecoInit.
 * @brief  Init ECO (EXTERNAL crystal oscillator) 24 MHz clock.
 * @param  None
 * 
 * @details External oscillator isn't instaled on PCBA by default
 * 
 * @retval cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_ecoInit(void);

/**
 * @fn        BSP_clock_hifclkInit.
 * @brief     Init high frequency clock CLK_HF[0]
 * @param[in] pSettings - board settings
 * @retval    cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_hifclkInit(bsp_board_init_t* pSettings);

/*-------------------------*/
/** Board LED APIs section */
/*-------------------------*/
/**
 * @fn     BSP_led_red_toggle.
 * @brief  Toggle red LED.
 * @param  None
 * @retval None 
 */
void BSP_led_red_toggle(void);

/**
 * @fn     BSP_led_red_On.
 * @brief  On red LED.
 * @param  None
 * @retval None 
 */
void BSP_led_red_On(void);

/**
 * @fn     BSP_led_red_Off.
 * @brief  Off red LED.
 * @param  None
 * @retval None 
 */
void BSP_led_red_Off(void);

/**
 * @fn     BSP_led_green_toggle.
 * @brief  Toggle green LED.
 * @param  None
 * @retval None 
 */
void BSP_led_green_toggle(void);

/*--------------------------*/
/** Board UART APIs section */
/*--------------------------*/
/**
 * @fn     BSP_initUart.
 * @brief  Initialize UART.
 * @param  None
 * @retval 0 - success, otherwise - fail status
 */
bsp_status_init_t BSP_initUart(bspUartRxCallback callback);

/**
 * @fn     BSP_isUartTxReady.
 * @brief  Check if UART TX FIFO is ready to receive data.
 * @param  None
 * @retval true - UART is ready, false - not ready.
 */
bool BSP_isUartTxReady(void);

/**
 * @fn     BSP_isUartTxEmpty.
 * @brief  Check if UART TX FIFO is empty.
 * @param  None
 * @retval true - empty, false - not empty.
 */
bool BSP_isUartTxEmpty(void);

/**
 * @fn     BSP_uartTxData.
 * @brief  TRansmit data via UART.
 * @param[in] data - data buffer
 * @param[in] len - length of data
 * @retval None 
 */
void BSP_uartTxData(uint8_t *data, uint16_t len);

#if defined(Q_UTEST)
/**
 * @fn     BSP_initUTdic.
 * @brief  Initi dictionaries for BSP UTs.
 * @param  None
 * @retval None 
 */
void BSP_initUTdic(void);
#endif //Q_UTEST

#endif  // __BSP_H__


/************************** END OF FILE *****************************************/
