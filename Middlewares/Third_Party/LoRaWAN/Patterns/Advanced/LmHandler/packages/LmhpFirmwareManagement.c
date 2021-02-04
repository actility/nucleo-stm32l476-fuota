/*!
 * \file      LmhpFirmwareManagement.c
 *
 * \brief     Implements the LoRa-Alliance firmware management rc2 package
 *            Specification: 
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *            
 *              (C)2019-2020 Actility
 *
 * \endcode
 *
 * \author    Oleg Tabarovskiy ( RTLS )
 */
#include "LmHandler.h"
#include "LmhpFirmwareManagement.h"
#include "FwUpdateAgent.h"
#include "version.h"
#include <string.h>
#include "storage.h"

#include "util_console.h"        // Required for debug PRINTF()

/*!
 * LoRaWAN LoRaWAN Firmware Management Protocol Specification
 */
#define FWMANAGEMENT_PORT                          203

#define FWMANAGEMENT_ID                            4
#define FWMANAGEMENT_VERSION                       1

/*!
 * LoRaWAN LoRaWAN Firmware Management Package defines
 */
/*
 * Originally used BlkAckDelay, but we can't transfer it through
 * reboot so define it here and use this parameters
 */
#define	DEVVERSIONANSMIN							16	/* min delay seconds */
#define DEVVERSIONANSMAX							128 /* max delay seconds */


/*!
 * Package current context
 */
typedef struct LmhpFWManagementState_s
{
    bool Initialized;
    bool IsRunning;
    bool DevVersionAnsSentOnBoot;       
    bool IsUpgradeScheduled;                    // Delayed reboot and image upgrade scheduled
    bool IsUpgradeDelayed;
    uint8_t DataBufferMaxSize;
    uint8_t *DataBuffer;
    uint8_t *file;
}LmhpFWManagementState_t;

typedef enum LmhpFWManagementMoteCmd_e
{
    FWMANAGEMENT_PKG_VERSION_ANS         = 0x00,
    FWMANAGEMENT_DEV_VERSION_ANS         = 0x01,
    FWMANAGEMENT_DEV_REBOOT_TIME_ANS     = 0x02,
    FWMANAGEMENT_DEV_UPGRADE_IMAGE_ANS   = 0x04,
	FWMANAGEMENT_DEV_DELETE_IMAGE_ANS    = 0x05
}LmhpFWManagementMoteCmd_t;

typedef enum LmhpFWManagementSrvCmd_e
{
    FWMANAGEMENT_PKG_VERSION_REQ         = 0x00,
    FWMANAGEMENT_DEV_VERSION_REQ         = 0x01,
    FWMANAGEMENT_DEV_REBOOT_TIME_REQ     = 0x02,
    FWMANAGEMENT_DEV_UPGRADE_IMAGE_REQ   = 0x04,
	FWMANAGEMENT_DEV_DELETE_IMAGE_REQ    = 0x05
}LmhpFWManagementSrvCmd_t;

/*!
 * LoRaWAN Firmware management protocol handler parameters
 */
static LmhpFWManagementParams_t* LmhpFWManagementParams;

/*!
 * Initializes the package with provided parameters
 *
 * \param [IN] params            Pointer to the package parameters
 * \param [IN] dataBuffer        Pointer to main application buffer
 * \param [IN] dataBufferMaxSize Main application buffer maximum size
 */
static void LmhpFWManagementInit( void *params, uint8_t *dataBuffer, uint8_t dataBufferMaxSize );

/*!
 * Returns the current package initialization status.
 *
 * \retval status Package initialization status
 *                [true: Initialized, false: Not initialized]
 */
static bool LmhpFWManagementIsInitialized( void );

/*!
 * Returns the package operation status.
 *
 * \retval status Package operation status
 *                [true: Running, false: Not running]
 */
static bool LmhpFWManagementIsRunning( void );

/*!
 * Processes the internal package events.
 */
static void LmhpFWManagementProcess( void );

/*!
 * Processes the MCPS Indication
 *
 * \param [IN] mcpsIndication     MCPS indication primitive data
 */
static void LmhpFWManagementOnMcpsIndication( McpsIndication_t *mcpsIndication );

/*
 * Supplementary functions
 */
static void OnDevVersionAnsTimerEvent( void *context );
static void OnDevRebootTimerEvent( void *context );

static LmhpFWManagementState_t LmhpFWManagementState =
{
    .Initialized = false,
    .IsRunning = false,
    .DevVersionAnsSentOnBoot = false,
    .IsUpgradeScheduled = false,
    .IsUpgradeDelayed = false
};

static LmhPackage_t LmhpFWManagementPackage =
{
    .Port = FWMANAGEMENT_PORT,
    .Init = LmhpFWManagementInit,
    .IsInitialized = LmhpFWManagementIsInitialized,
    .IsRunning = LmhpFWManagementIsRunning,
    .Process = LmhpFWManagementProcess,
    .OnMcpsConfirmProcess = NULL,                              // Not used in this package
    .OnMcpsIndicationProcess = LmhpFWManagementOnMcpsIndication,
    .OnMlmeConfirmProcess = NULL,                              // Not used in this package
    .OnMlmeIndicationProcess = NULL,                           // Not used in this package
    .OnMacMcpsRequest = NULL,                                  // To be initialized by LmHandler
    .OnMacMlmeRequest = NULL,                                  // To be initialized by LmHandler
    .OnJoinRequest = NULL,                                     // To be initialized by LmHandler
    .OnSendRequest = NULL,                                     // To be initialized by LmHandler
    .OnDeviceTimeRequest = NULL,                               // To be initialized by LmHandler
    .OnSysTimeUpdate = NULL,                                   // To be initialized by LmHandler
};

static TimerEvent_t DevVersionAnsTimer;

static TimerEvent_t RebootTimer;

LmhPackage_t *LmhpFWManagementPackageFactory( void )
{
    return &LmhpFWManagementPackage;
}

static void LmhpFWManagementInit( void *params, uint8_t *dataBuffer, uint8_t dataBufferMaxSize )
{
    if( params != NULL && dataBuffer != NULL )
    {
        LmhpFWManagementParams = ( LmhpFWManagementParams_t* )params;
        LmhpFWManagementParams->NewImageValidateStatus = UPIMG_STATUS_ABSENT;
        LmhpFWManagementParams->NewImageFWVersion = 0;
        LmhpFWManagementState.DataBuffer = dataBuffer;
        LmhpFWManagementState.DataBufferMaxSize = dataBufferMaxSize;
        LmhpFWManagementState.Initialized = true;
        LmhpFWManagementState.IsRunning = true;
        PRINTF( "FW Management Package initialized\r\n" );
    }
    else
    {
        LmhpFWManagementParams = NULL;
        LmhpFWManagementState.IsRunning = false;
        LmhpFWManagementState.Initialized = false;
    }
    LmhpFWManagementState.DevVersionAnsSentOnBoot = false;
    LmhpFWManagementState.IsUpgradeScheduled = false;
    LmhpFWManagementState.IsUpgradeDelayed = false;
  
}

static bool LmhpFWManagementIsInitialized( void )
{
    return LmhpFWManagementState.Initialized;
}

static bool LmhpFWManagementIsRunning( void )
{
    if( LmhpFWManagementState.Initialized == false )
    {
        return false;
    }

    return LmhpFWManagementState.IsRunning;
}

static void LmhpFWManagementProcess( void )
{
    int32_t delay = randr(DEVVERSIONANSMIN, DEVVERSIONANSMAX) * 1000;
    
    // We have to send DevVersionAns on boot after random delay just once
    if( LmhpFWManagementState.DevVersionAnsSentOnBoot == false )
    {
      TimerInit( &DevVersionAnsTimer, OnDevVersionAnsTimerEvent );
      TimerSetValue( &DevVersionAnsTimer, delay );
      TimerStart( &DevVersionAnsTimer );
      LmhpFWManagementState.DevVersionAnsSentOnBoot = true;  
      PRINTF( "DevVersionAns scheduled in %ums\r\n", delay );
    }
    // Cneck if upgrade was not delayed and signal immediate upgrade if image valid
    if( !LmhpFWManagementState.IsUpgradeDelayed && LmhpFWManagementParams->NewImageValidateStatus == UPIMG_STATUS_VALID )
    {
      LmhpFWManagementState.IsUpgradeScheduled = true;
    }
    if( LmhpFWManagementState.IsUpgradeScheduled && LmhpFWManagementParams->NewImageValidateStatus == UPIMG_STATUS_VALID ) 
    {
      PRINTF( "\r\n...... Upgrading firmware  ......\r\n" );
      FwUpdateAgentRun( );
    }
}

static void LmhpFWManagementOnMcpsIndication( McpsIndication_t *mcpsIndication )
{
    uint8_t cmdIndex = 0;
    uint8_t dataBufferIndex = 0;
    
    while( cmdIndex < mcpsIndication->BufferSize )
    {
      switch (mcpsIndication->Buffer[cmdIndex++]) 
      {
            case FWMANAGEMENT_PKG_VERSION_REQ:
            {
                if( mcpsIndication->Multicast == 1 )
                {
                    // Multicast channel. Don't process command.
                    break;
                }
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_PKG_VERSION_ANS;
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_ID;
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_VERSION;
                break;
            }    
            
            case FWMANAGEMENT_DEV_VERSION_REQ:
            {
                if( mcpsIndication->Multicast == 1 )
                {
                    // Multicast channel. Don't process command.
                    break;
                }
                PRINTF("Receive DevVerionReq");
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_DEV_VERSION_ANS;
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION >> 8);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION >> 16);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION >> 24);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION >> 8);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION >> 16);
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION >> 24);
                break;
            }
     
            case FWMANAGEMENT_DEV_REBOOT_TIME_REQ:
            {
                if( mcpsIndication->Multicast == 1 )
                {
                    // Multicast channel. Don't process command.
                    break;
                }
                PRINTF("Receive DevRebootTimeReq\r\n");
                uint32_t upgrade_gps_s;
                memcpy( &upgrade_gps_s, &mcpsIndication->Buffer[cmdIndex], sizeof( upgrade_gps_s ) );
                cmdIndex += sizeof( upgrade_gps_s );
                if( upgrade_gps_s == FWM_APPLY_ASAP ) 
                {
                    PRINTF("FWM_APPLY_ASAP");
                    LmhpFWManagementState.IsUpgradeDelayed = false;
                    LmhpFWManagementState.IsUpgradeScheduled = true;
                } 
                else if( upgrade_gps_s == FWM_CANCEL_UPGRADE ) 
                {
                    PRINTF("FWM_CANCEL_UPGRADE");
                    // IsUpgradeDelayed still true and RebootTimer is stopped
                    // IsUpgradeScheduled will never be set to true and upgrade
                    // is cancelled until device reboots for any reason 
                    // (was manually reset for example) or new DevRebootTimeReq command is received
                    // Or DevDeleteImageReq was received and UPIMG_STATUS_VALID was cleared.
                    // In the later case if the new image is received reboot will be scheduled
                    // as no cancellation was performed.
                    TimerStop(&RebootTimer);
                } 
                else 
                {
          
                    SysTime_t sysTime = SysTimeGet();
                	uint32_t now_gps_s = sysTime.Seconds - UNIX_GPS_EPOCH_OFFSET;
                    uint32_t timeout_s = upgrade_gps_s - now_gps_s;
                    PRINTF( "upgrade scheduled at GPS time: %u after %u\r\n", upgrade_gps_s, timeout_s );                   
                    if( now_gps_s >= upgrade_gps_s ) 
                    {
                          PRINTF( "upgrade time in past!\r\n" );
                          upgrade_gps_s = 0; //report error
                    } 
                    else 
                    {
                          PRINTF( "upgrade scheduled at GPS time: %u after %u\r\n", upgrade_gps_s, timeout_s );
                          LmhpFWManagementState.IsUpgradeDelayed = true;
                          TimerInit( &RebootTimer, OnDevRebootTimerEvent );
                          TimerSetValue( &RebootTimer, timeout_s * 1000 );
                          TimerStart( &RebootTimer );
                          PRINTF( "Upgrade scheduled in %us", timeout_s );
                    }
                }
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_DEV_REBOOT_TIME_ANS;
                memcpy( &LmhpFWManagementState.DataBuffer[dataBufferIndex], &upgrade_gps_s, sizeof( upgrade_gps_s ) );
                dataBufferIndex += sizeof( upgrade_gps_s );
                break;
            }
            
            case FWMANAGEMENT_DEV_UPGRADE_IMAGE_REQ:
            {
                if( mcpsIndication->Multicast == 1 )
                {
                    // Multicast channel. Don't process command.
                    break;
                }
                PRINTF( "Receive DevUpgradeImageReq\r\n" );
                
                LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_DEV_UPGRADE_IMAGE_ANS;
                LmhpFWManagementState.DataBuffer[dataBufferIndex] = LmhpFWManagementParams->NewImageValidateStatus; 
                if( LmhpFWManagementState.DataBuffer[dataBufferIndex++] == UPIMG_STATUS_VALID ) 
                {
                    LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(LmhpFWManagementParams->NewImageFWVersion);
                    LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(LmhpFWManagementParams->NewImageFWVersion >> 8);
                    LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(LmhpFWManagementParams->NewImageFWVersion >> 16);
                    LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(LmhpFWManagementParams->NewImageFWVersion >> 24);
                }
                break;
            }

            case FWMANAGEMENT_DEV_DELETE_IMAGE_REQ:
            {
            	if( mcpsIndication->Multicast == 1 )
            	{
            	    // Multicast channel. Don't process command.
            	    break;
            	}
            	PRINTF( "Receive DevDeleteImageReq\r\n" );
            	uint32_t fw_version;
            	memcpy( &fw_version, &mcpsIndication->Buffer[cmdIndex], sizeof( fw_version ) );
            	cmdIndex += sizeof( fw_version );
            	LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_DEV_DELETE_IMAGE_ANS;
            	if( LmhpFWManagementParams->NewImageValidateStatus !=  UPIMG_STATUS_VALID )
            	{
            		LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWM_DEL_ERRORNOVALIDIMAGE;
            		PRINTF( "DevDeleteImageReq: No valid image\r\n" );

            	} else {
            		//
            		// For now always delete existing image. Until decision
            		// is made how to track downloaded image version
            		//
            		// if( LmhpFWManagementParams->NewImageFWVersion == fw_version )
            		// {
            		//
            			storage_erase_slot(STORAGE_SLOT_NEWIMG);
            			LmhpFWManagementState.DataBuffer[dataBufferIndex++] = 0;
            			LmhpFWManagementParams->NewImageValidateStatus = UPIMG_STATUS_ABSENT;
            			// Image is deleted no reason to cancel reboot any more
            			LmhpFWManagementState.IsUpgradeDelayed = false;
            			PRINTF( "DevDeleteImageReq: Image deleted\r\n" );
            		//
            		// } else {
            		//	LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWM_DEL_ERRORINVALIDVERSION;
            		//	PRINTF( "DevDeleteImageReq: Invalid image version %x vs %x\r\n", fw_version,
            		//			LmhpFWManagementParams->NewImageFWVersion );
            		// }
            	}
            	break;
            }
                            
            default:
            {
                PRINTF("Invalid FWM Package cmd: %x\r\n", mcpsIndication->Buffer[cmdIndex - 1]);
                // Skip remaining bytes in command buffer because we don't know what to do with it
                cmdIndex = mcpsIndication->BufferSize;
                break;
            }
      }  
      if( dataBufferIndex != 0 )
      {
          // Answer commands
          LmHandlerAppData_t appData =
          {
              .Buffer = LmhpFWManagementState.DataBuffer,
              .BufferSize = dataBufferIndex,
              .Port = FWMANAGEMENT_PORT
          };
          LmhpFWManagementPackage.OnSendRequest( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG );
      }
   }
}

static void OnDevVersionAnsTimerEvent( void *context ) 
{
  uint8_t dataBufferIndex = 0;
  
  MibRequestConfirm_t mibReq;

  mibReq.Type = MIB_DEVICE_CLASS;
  LoRaMacMibGetRequestConfirm( &mibReq );
  if( mibReq.Param.Class != CLASS_A ) {
	// Do not interfere with CLASS_C or CLASS_B session,
	// just skip sending DevVersionAns if we are not in CLASS_A
	TimerStop(&DevVersionAnsTimer);
	PRINTF( "DevVersionAns canceled\r\n" );
	return;
  }

  if( LmHandlerIsBusy( ) == true )
  {
    // We will reschedule timer in Process() if stack is busy 
    LmhpFWManagementState.DevVersionAnsSentOnBoot = false;  
    return;
  }
  TimerStop(&DevVersionAnsTimer);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = FWMANAGEMENT_DEV_VERSION_ANS;
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION >> 8);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION >> 16);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__APP_VERSION >> 24);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION >> 8);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION >> 16);
  LmhpFWManagementState.DataBuffer[dataBufferIndex++] = (uint8_t)(__HW_VERSION >> 24);
  LmHandlerAppData_t appData = {
    .Buffer = LmhpFWManagementState.DataBuffer,
    .BufferSize = dataBufferIndex,
    .Port = FWMANAGEMENT_PORT
  };
  LmhpFWManagementPackage.OnSendRequest( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG );
  PRINTF( "DevVersionAns sent\r\n" );
}

static void OnDevRebootTimerEvent( void *context ) {
    LmhpFWManagementState.IsUpgradeScheduled = true;
}
