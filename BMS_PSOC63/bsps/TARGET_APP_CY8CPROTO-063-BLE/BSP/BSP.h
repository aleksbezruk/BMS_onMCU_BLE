/*******************************************************************************
 * \file BSP.h
 * \version 0.1.0
 *
 * Board support package header file.
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
#define CLOCK_CLK_FAST_HZ        150000000U
#define CLOCK_HZ_IN_MHZ          1000000U
#define CLOCK_CLK_ECO_HZ         24000000U
#define CLOCK_CLK_IMO_HZ         8000000U
#define CLOCK_WCO_ENABLE_TIMEOUT 5000000U /// microseconds
#define CLOCK_ECO_ENABLE_TIMEOUT 20000U /// microseconds
#define CLOCK_PLL_ENABLE_TIMEOUT 20000U /// microseconds

/*******************************************************************************
 * Types
 ******************************************************************************/
typedef enum {
    bsp_status_init_success,
    bsp_status_init_fail
} bsp_status_init_t;

/**
 * ECO isn't installed on CYBLE-063 board, only footprint is available
 */
typedef enum {
   bsp_main_oscil_src_IMO,
   bsp_main_oscil_src_ECO
} bsp_main_oscil_src_t;

typedef struct {
    bsp_main_oscil_src_t mainOscilSrc;
} bsp_board_init_t;

/*******************************************************************************
 * API
 ******************************************************************************/
 
/*--------------------------*/
/** Board init APIs section */
/*--------------------------*/

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
 *              - 1.1 V mode;
 *              - set high current/normal mode.
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
 * @retval cy_en_sysclk_status_t 
 */
cy_en_sysclk_status_t BSP_clock_wcoInit(void);

/**
 * @fn     BSP_clock_ecoInit.
 * @brief  Init ECO 24 kHz clock.
 * @param  None
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
 * @fn     BSP_led_green_toggle.
 * @brief  Toggle green LED.
 * @param  None
 * @retval None 
 */
void BSP_led_green_toggle(void);

#endif  // __BSP_H__


/************************** END OF FILE *****************************************/
