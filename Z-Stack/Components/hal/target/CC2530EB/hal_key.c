/**************************************************************************************************
  Filename:       hal_key.c
  Description:    This file contains the interface to the HAL KEY Service.
                  Simplified for a single button on P0.1 for the E18-MS1-PCB.
**************************************************************************************************/

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_board_cfg.h" // Contains our custom pin configuration
#include "hal_drivers.h"
#include "hal_key.h"
#include "osal.h"

#if (defined HAL_KEY) && (HAL_KEY == TRUE)

/**************************************************************************************************
 * CONSTANTS
 */
#define HAL_KEY_DEBOUNCE_VALUE  25 // Debounce time in milliseconds

/**************************************************************************************************
 * TYPEDEFS
 */
static halKeyCBack_t pHalKeyProcessFunction;
static uint8 HalKeyConfigured;
static uint8 halKeySavedKeys = 0; // Stores the previous key state for polling

/**************************************************************************************************
 * GLOBAL VARIABLES
 */
// FIX: Added this global variable back to resolve a linker error.
// The hal_drivers.c file requires this variable to be externally available.
bool Hal_KeyIntEnable;

/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initialize Key Service.
 */
void HalKeyInit( void )
{
  // Configure P0.1 as a GPIO input pin
  P0SEL &= ~PUSH1_BV;
  P0DIR &= ~PUSH1_BV;
  
  // Enable the internal pull-down resistor on P0.1
  // This is the correct method for the CC2530
  P2INP &= ~BV(6);    // Set Port 0 input mode to pull-up/pull-down
  P0INP &= ~PUSH1_BV; // Set P0.1 to have its pull resistor enabled (default is pull-down)
  
  halKeySavedKeys = 0;
  pHalKeyProcessFunction = NULL;
  HalKeyConfigured = FALSE;
  
  // Since this driver is polling-only, ensure the interrupt flag is always false.
  Hal_KeyIntEnable = FALSE;
}

/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key service. Polling is used for this simple driver.
 */
void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback)
{
  (void)interruptEnable; // This simplified driver uses polling, so interrupts are ignored.
  
  // Register the application's callback function
  pHalKeyProcessFunction = cback;

  // Start the polling timer if it's not already running
  if (HalKeyConfigured == FALSE)
  {
    osal_set_event(Hal_TaskID, HAL_KEY_EVENT);
  }
  
  HalKeyConfigured = TRUE;
}

/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of the key(s).
 *
 * @return  keys - bit mask of all currently pressed keys
 */
uint8 HalKeyRead( void )
{
  uint8 keys = 0;

  // HAL_PUSH_BUTTON1() is defined in hal_board_cfg.h and points to P0.1
  if (HAL_PUSH_BUTTON1())
  {
    // If the button is pressed, report it as HAL_KEY_SW_1.
    // This now matches the key code expected by the application.
    keys |= HAL_KEY_SW_1;
  }

  return keys;
}

/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by the HAL task to poll the keys. This function is called
 * periodically by the OSAL timer.
 */
void HalKeyPoll(void)
{
  uint8 currentKeys = HalKeyRead();

  // If the key state has changed since the last poll...
  if (currentKeys != halKeySavedKeys)
  {
    halKeySavedKeys = currentKeys; // ...save the new state...
    
    // ...and notify the application by calling its registered callback function.
    if (pHalKeyProcessFunction)
    {
      (pHalKeyProcessFunction)(currentKeys, HAL_KEY_STATE_NORMAL);
    }
  }
}

// All other functions from the original TI file (interrupts, sleep, joystick) are
// removed as they are not needed for this hardware configuration.

#else // This is for builds where HAL_KEY is FALSE

void HalKeyInit(void) {}
void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback) {}
uint8 HalKeyRead(void) { return 0; }
void HalKeyPoll(void) {}

#endif // HAL_KEY