/**************************************************************************************************
  Filename:       zcl_samplesw.c
  Description:    Fixed Sample Switch Application for Coordinator (E18-MS1-PCB).
                  This version is compatible with the default SampleSwitch project includes.
**************************************************************************************************/

#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_samplesw.h"
#include "zcl_ezmode.h"

#define EZMODE_ACTION_CLIENT_INITIATOR_START  0x01

#include "OnBoard.h"
#include "hal_led.h"
#include "hal_key.h"
#include "DebugTrace.h"
#include <stdio.h>

/*********************************************************************
 * CONSTANTS
 */
#define APP_KEY_PRESS HAL_KEY_SW_1

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclSampleSw_TaskID;
uint8 zclSampleSw_TransID = 0;
afAddrType_t zclSampleSw_DstAddr;
devStates_t zclSampleSw_NwkState = DEV_INIT;

extern uint8 aExtendedAddress[];

/*********************************************************************
 * LOCAL VARIABLES
 */
static endPointDesc_t zclSampleSw_epDesc;
static uint8 switchWasPressed = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclSampleSw_HandleKeys(uint8 shift, uint8 keys);
static void zclSampleSw_MessageMSGCB(afIncomingMSGPacket_t *pckt);
static void zclSampleSw_BasicResetCB(void);

/*********************************************************************
 * ZCL General Profile Callbacks
 */
static zclGeneral_AppCallbacks_t zclSampleSw_CmdCallbacks =
{
  zclSampleSw_BasicResetCB,
  NULL, NULL,
  NULL, NULL, NULL,
  NULL
};

/*********************************************************************
 * @fn      zclSampleSw_Init
 */
void zclSampleSw_Init(byte task_id)
{
  zclSampleSw_TaskID = task_id;

  zclSampleSw_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zclSampleSw_DstAddr.endPoint = 0;
  zclSampleSw_DstAddr.addr.shortAddr = 0;

  zclSampleSw_epDesc.endPoint = SAMPLESW_ENDPOINT;
  zclSampleSw_epDesc.task_id = &zclSampleSw_TaskID;
  zclSampleSw_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&zclSampleSw_SimpleDesc;
  zclSampleSw_epDesc.latencyReq = noLatencyReqs;
  afRegister(&zclSampleSw_epDesc);

  RegisterForKeys(zclSampleSw_TaskID);
  zclGeneral_RegisterCmdCallbacks(SAMPLESW_ENDPOINT, &zclSampleSw_CmdCallbacks);

  // Register for the ZDO messages needed to confirm a successful bind
  ZDO_RegisterForZDOMsg(zclSampleSw_TaskID, End_Device_Bind_rsp);
  ZDO_RegisterForZDOMsg(zclSampleSw_TaskID, Match_Desc_rsp);

  debug_str((uint8 *)"Coordinator (E18-MS1-PCB) Power ON\r\n");
}

/*********************************************************************
 * @fn      zclSampleSw_event_loop
 */
UINT16 zclSampleSw_event_loop(byte task_id, UINT16 events)
{
  afIncomingMSGPacket_t *MSGpkt;

  if (events & SYS_EVENT_MSG)
  {
    while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclSampleSw_TaskID)))
    {
      switch (MSGpkt->hdr.event)
      {
        case KEY_CHANGE:
          zclSampleSw_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
          break;

        case AF_INCOMING_MSG_CMD:
          zclSampleSw_MessageMSGCB(MSGpkt);
          break;

        case ZDO_STATE_CHANGE:
          zclSampleSw_NwkState = (devStates_t)(MSGpkt->hdr.status);

          if (zclSampleSw_NwkState == DEV_ZB_COORD)
          {
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK); // Blink until bound
            debug_str((uint8 *)"CONNECTED: Zigbee network started as Coordinator.\r\n");
            
            char buf[100];
            uint16 panId = _NIB.nwkPanId;
            uint16 nwkAddr = NLME_GetShortAddr();
            sprintf(buf, "PAN ID: 0x%04X, Coordinator Address: 0x%04X\r\n", panId, nwkAddr);
            debug_str((uint8 *)buf);
            sprintf(buf, "IEEE Address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    aExtendedAddress[7], aExtendedAddress[6], aExtendedAddress[5], aExtendedAddress[4],
                    aExtendedAddress[3], aExtendedAddress[2], aExtendedAddress[1], aExtendedAddress[0]);
            debug_str((uint8 *)buf);

            debug_str((uint8 *)"Scanning for devices to bind...\r\n");
            zcl_EZModeAction(EZMODE_ACTION_CLIENT_INITIATOR_START, NULL);
          }
          else if (zclSampleSw_NwkState == DEV_INIT)
          {
             debug_str((uint8 *)"CONNECTING: Initializing and starting Zigbee network...\r\n");
             HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
          }
          else
          {
             debug_str((uint8 *)"NOT CONNECTED: Network status changed.\r\n");
             HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
          }
          break;

        case ZDO_CB_MSG:
        {
          zdoIncomingMsg_t *zdoMsg = (zdoIncomingMsg_t *)MSGpkt;
          if (zdoMsg->clusterID == End_Device_Bind_rsp)
          {
            // --- FIX: Read the status directly from the message payload ---
            // The first byte of the End_Device_Bind_rsp payload is the status.
            // This avoids issues with the ZDEndDeviceBindRsp_t type not being found.
            if (zdoMsg->asdu[0] == ZDP_SUCCESS)
            {
              // A successful bind has occurred!
              // The destination address is the source of the response.
              zclSampleSw_DstAddr.addrMode = Addr16Bit;
              zclSampleSw_DstAddr.addr.shortAddr = zdoMsg->srcAddr.addr.shortAddr;
              zclSampleSw_DstAddr.endPoint = SAMPLESW_ENDPOINT; // Typically the same endpoint
              
              debug_str("CONNECTED: Successfully bound to a device.\r\n");
              HalLedSet(HAL_LED_1, HAL_LED_MODE_ON); // Solid LED for bound state
            }
          }
        }
        break;

        default:
          break;
      }
      osal_msg_deallocate((uint8 *)MSGpkt);
    }
    return (events ^ SYS_EVENT_MSG);
  }

  return 0;
}

/*********************************************************************
 * @fn      zclSampleSw_HandleKeys
 */
static void zclSampleSw_HandleKeys(uint8 shift, uint8 keys)
{
  (void)shift;

  if (zclSampleSw_DstAddr.addrMode == (afAddrMode_t)AddrNotPresent)
  {
    if (keys & APP_KEY_PRESS) {
        debug_str("Switch pressed, but no device bound. Starting scan...\r\n");
        zcl_EZModeAction(EZMODE_ACTION_CLIENT_INITIATOR_START, NULL);
    }
    return;
  }

  if (keys & APP_KEY_PRESS)
  {
    if (switchWasPressed == FALSE)
    {
      switchWasPressed = TRUE;
      debug_str("SENT: ON\r\n");
      zclGeneral_SendOnOff_CmdOn(SAMPLESW_ENDPOINT, &zclSampleSw_DstAddr, FALSE, zclSampleSw_TransID++);
    }
  }
  else
  {
    if (switchWasPressed == TRUE)
    {
      switchWasPressed = FALSE;
      debug_str("SENT: OFF\r\n");
      zclGeneral_SendOnOff_CmdOff(SAMPLESW_ENDPOINT, &zclSampleSw_DstAddr, FALSE, zclSampleSw_TransID++);
    }
  }
}

/*********************************************************************
 * @fn      zclSampleSw_MessageMSGCB
 */
static void zclSampleSw_MessageMSGCB(afIncomingMSGPacket_t *pckt)
{
  zclIncomingMsg_t *msg = (zclIncomingMsg_t *)pckt;

  if (msg->clusterId == ZCL_CLUSTER_ID_GEN_ON_OFF)
  {
    switch (msg->zclHdr.commandID)
    {
      case COMMAND_ON:
        debug_str((uint8 *)"MOTION: Motion Detected (ON received)\r\n");
        break;
      case COMMAND_OFF:
        debug_str((uint8 *)"MOTION: No Motion (OFF received)\r\n");
        break;
    }
  }

  zcl_ProcessMessageMSG(pckt);  // Keep ZCL stack processing
}


/*********************************************************************
 * @fn      zclSampleSw_BasicResetCB
 */
static void zclSampleSw_BasicResetCB(void)
{
  debug_str("Device received a Basic Cluster Reset command.\r\n");
}