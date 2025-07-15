/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Description:    Board configuration for E18-MS1-PCB.
**************************************************************************************************/
#ifndef HAL_BOARD_CFG_H
#define HAL_BOARD_CFG_H

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"

#define HAL_CPU_CLOCK_MHZ     32
#define HAL_CLOCK_CRYSTAL

#if !defined (OSC32K_CRYSTAL_INSTALLED) || (defined (OSC32K_CRYSTAL_INSTALLED) && (OSC32K_CRYSTAL_INSTALLED == TRUE))
  #define OSC_32KHZ  0x00
#else
  #define OSC_32KHZ  0x80
#endif

#define HAL_CLOCK_STABLE()    st( while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ)); )

#define HAL_NUM_LEDS            1
#define HAL_LED_BLINK_DELAY()   st( { volatile uint32 i; for (i=0; i<0x5800; i++) { }; } )

#define LED1_BV           BV(4)
#define LED1_SBIT         P1_4
#define LED1_DDR          P1DIR
#define LED1_POLARITY     ACTIVE_HIGH

#define LED2_BV           LED1_BV
#define LED2_SBIT         LED1_SBIT
#define LED2_DDR          LED1_DDR
#define LED2_POLARITY     LED1_POLARITY

#define LED3_BV           LED1_BV
#define LED3_SBIT         LED1_SBIT
#define LED3_DDR          LED1_DDR
#define LED3_POLARITY     LED1_POLARITY

#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!

#define PUSH1_BV          BV(1)
#define PUSH1_SBIT        P0_1
#define PUSH1_POLARITY    ACTIVE_HIGH

#define PUSH2_BV          BV(0)
#define PUSH2_SBIT        P2_0
#define PUSH2_POLARITY    ACTIVE_HIGH

#define HAL_FLASH_PAGE_PER_BANK    16
#define HAL_FLASH_PAGE_SIZE        2048
#define HAL_FLASH_WORD_SIZE        4
#define HAL_FLASH_PAGE_MAP         0x8000

#if defined NON_BANKED
#define HAL_FLASH_LOCK_BITS        16
#define HAL_NV_PAGE_END            30
#define HAL_NV_PAGE_CNT            2
#else
#define HAL_FLASH_LOCK_BITS        16
#define HAL_NV_PAGE_END            126
#define HAL_NV_PAGE_CNT            6
#endif

#define HAL_FLASH_IEEE_SIZE        8
#define HAL_FLASH_IEEE_PAGE       (HAL_NV_PAGE_END+1)
#define HAL_FLASH_IEEE_OSET       (HAL_FLASH_PAGE_SIZE - HAL_FLASH_LOCK_BITS - HAL_FLASH_IEEE_SIZE)
#define HAL_INFOP_IEEE_OSET        0xC

#define HAL_NV_PAGE_BEG           (HAL_NV_PAGE_END-HAL_NV_PAGE_CNT+1)

#define HAL_NV_DMA_CH              0
#define HAL_DMA_CH_RX              3
#define HAL_DMA_CH_TX              4
#define HAL_NV_DMA_GET_DESC()      HAL_DMA_GET_DESC0()
#define HAL_NV_DMA_SET_ADDR(a)     HAL_DMA_SET_ADDR_DESC0((a))

#define HAL_LCD      FALSE
#define HAL_TIMER    FALSE
#define HAL_LED      TRUE
#define HAL_KEY      TRUE
#define HAL_ADC      TRUE
#define HAL_DMA      TRUE
#define HAL_FLASH    TRUE
#define HAL_AES      TRUE
#define HAL_AES_DMA  TRUE

#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif

#ifndef HAL_UART
#if (defined ZAPP_P1) || (defined ZAPP_P2) || (defined ZTOOL_P1) || (defined ZTOOL_P2)
#define HAL_UART TRUE
#else
#define HAL_UART FALSE
#endif
#endif

#if HAL_UART
  #ifndef HAL_UART_DMA
    #if HAL_DMA
      #if (defined ZAPP_P2) || (defined ZTOOL_P2)
        #define HAL_UART_DMA  2
      #else
        #define HAL_UART_DMA  1
      #endif
    #else
      #define HAL_UART_DMA  0
    #endif
  #endif

  #ifndef HAL_UART_ISR
    #if HAL_UART_DMA
      #define HAL_UART_ISR  0
    #elif (defined ZAPP_P2) || (defined ZTOOL_P2)
      #define HAL_UART_ISR  2
    #else
      #define HAL_UART_ISR  1
    #endif
  #endif

  #if (HAL_UART_DMA && (HAL_UART_DMA == HAL_UART_ISR))
    #error HAL_UART_DMA & HAL_UART_ISR must be different.
  #endif

  #if ((HAL_UART_DMA == 1) || (HAL_UART_ISR == 1))
    #define HAL_UART_PRIPO             0x00
  #else
    #define HAL_UART_PRIPO             0x40
  #endif
#else
  #define HAL_UART_DMA 0
  #define HAL_UART_ISR 0
#endif

#define HAL_UART_USB 0

#define PREFETCH_ENABLE()     st( FCTL = 0x08; )
#define PREFETCH_DISABLE()    st( FCTL = 0x04; )

#define HAL_BOARD_INIT()                                         \
{                                                                \
  uint16 i;                                                      \
                                                                 \
  SLEEPCMD &= ~OSC_PD;                                           \
  while (!(SLEEPSTA & XOSC_STB));                                \
  asm("NOP");                                                    \
  for (i=0; i<504; i++) asm("NOP");                              \
  CLKCONCMD = (CLKCONCMD_32MHZ | OSC_32KHZ);                     \
  while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ));            \
  SLEEPCMD |= OSC_PD;                                            \
                                                                 \
  PREFETCH_ENABLE();                                             \
                                                                 \
  HAL_TURN_OFF_LED1();                                           \
  LED1_DDR |= LED1_BV;                                           \
}

#define HAL_DEBOUNCE(expr)    { int i; for (i=0; i<500; i++) { if (!(expr)) i = 0; } }

#define HAL_PUSH_BUTTON1()        (PUSH1_POLARITY (PUSH1_SBIT))
#define HAL_PUSH_BUTTON2()        (PUSH2_POLARITY (PUSH2_SBIT))
#define HAL_PUSH_BUTTON3()        (0)
#define HAL_PUSH_BUTTON4()        (0)
#define HAL_PUSH_BUTTON5()        (0)
#define HAL_PUSH_BUTTON6()        (0)

#define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY (0); )
#define HAL_TURN_OFF_LED2()       HAL_TURN_OFF_LED1()
#define HAL_TURN_OFF_LED3()       HAL_TURN_OFF_LED1()
#define HAL_TURN_OFF_LED4()       HAL_TURN_OFF_LED1()

#define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY (1); )
#define HAL_TURN_ON_LED2()        HAL_TURN_ON_LED1()
#define HAL_TURN_ON_LED3()        HAL_TURN_ON_LED1()
#define HAL_TURN_ON_LED4()        HAL_TURN_ON_LED1()

#define HAL_TOGGLE_LED1()         st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
#define HAL_TOGGLE_LED2()         HAL_TOGGLE_LED1()
#define HAL_TOGGLE_LED3()         HAL_TOGGLE_LED1()
#define HAL_TOGGLE_LED4()         HAL_TOGGLE_LED1()

#define HAL_STATE_LED1()          (LED1_POLARITY (LED1_SBIT))
#define HAL_STATE_LED2()          HAL_STATE_LED1()
#define HAL_STATE_LED3()          HAL_STATE_LED1()
#define HAL_STATE_LED4()          HAL_STATE_LED1()

#define VDD_2_0  74
#define VDD_2_7  100
#define VDD_MIN_RUN   VDD_2_0
#define VDD_MIN_NV   (VDD_2_0+4)
#define VDD_MIN_XNV  (VDD_2_7+5)

#endif /* HAL_BOARD_CFG_H */