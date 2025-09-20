/**
 * @file BSP.h
 *
 * @brief Board support package header file for a CY8CPROTO-063-BLE dev board.
 *        The board contains CYBLE-416045-02 chip based on 
 *        PSoC 6 MCU CY8C6347BZI-BLD53
 * 
 * @version 0.6.0
 */

#ifndef __BSP_H__
#define __BSP_H__

///////////////////
// DEFINITIONS
///////////////////

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

#define CLOCK_CLKPATH1           1U
#define CLOCK_HIFCLK0            0U
#define CLOCK_CLK_FAST_HZ        100000000U
#define CLOCK_HZ_IN_MHZ          1000000U
#define CLOCK_CLK_ECO_HZ         24000000U
#define CLOCK_CLK_IMO_HZ         8000000U
#define CLOCK_WCO_ENABLE_TIMEOUT 5000000U   /**< microseconds */ 
#define CLOCK_ECO_ENABLE_TIMEOUT 20000U     /**<  microseconds */
#define CLOCK_PLL_ENABLE_TIMEOUT 20000U     /**<  microseconds */

#define BSP_TICKS_PER_SEC 1000U

#define BSP_ENABLE_ECO_CONFIG           false
#define BSP_ENABLE_PLL_CONFIG           false
#define BSP_ENABLE_UART_EXTENDED_FUNCS  false

///////////////////
// BSP USER TYPES
///////////////////

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
    bsp_main_oscil_src_t mainOscilSrc;  /**< Main oscillator source */
} bsp_board_init_t;

/** UART RX callback */
typedef void (*bspUartRxCallback)(uint8_t *data, uint16_t len);

//////////////////////////
// API
//////////////////////////
bsp_status_init_t BSP_init_board(bsp_board_init_t* pSettings);
void BSP_init_led_red(void);
void BSP_init_led_green(void);
cy_en_syspm_status_t BSP_power_init(void);
void BSP_clock_setWaitStates(void);
cy_en_sysclk_status_t BSP_clock_wcoInit(void);
cy_en_sysclk_status_t BSP_clock_ecoInit(void);
cy_en_sysclk_status_t BSP_clock_hifclkInit(bsp_board_init_t* pSettings);
void BSP_led_red_toggle(void);
void BSP_led_red_On(void);
void BSP_led_red_Off(void);
void BSP_led_green_toggle(void);
void BSP_led_green_on(void);
void BSP_led_green_off(void);
bsp_status_init_t BSP_initUart(bspUartRxCallback callback);
bool BSP_isUartTxReady(void);
bool BSP_isUartTxEmpty(void);
void BSP_uartTxData(uint8_t *data, uint16_t len);
bool BSP_isUartTxActive(void);
#if defined(Q_UTEST)
void BSP_initUTdic(void);
#endif //Q_UTEST

#endif  // __BSP_H__


/************************** END OF FILE *****************************************/
