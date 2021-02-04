/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   this is the main!
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "../inc/version.h"
#include "hw.h"
#include "low_power_manager.h"
#include "bsp.h"
#include "timeServer.h"
#include "vcom.h"

#include "Commissioning.h"
#include "LmHandler.h"
#include "LmhpCompliance.h"
#include "LmhpClockSync.h"
#include "LmhpRemoteMcastSetup.h"
#include "LmhpFragmentation.h"
#include "LmhpFirmwareManagement.h"

#include "FlashMemHandler.h"

#include "FwUpdateAgent.h"

#include "FragDecoder.h"

#include "sfu_app_new_image.h"
#include "storage.h"
#if	( ACTILITY_SMART_DELTA == 1 )
#include "patch.h"
#include "verify_signature.h"
#endif	/* ACTILITY_SMART_DELTA == 1  */

#if defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"    /* to access to the definition of REGION_SLOT_1_START*/
#elif defined(__CC_ARM)
#include "mapping_fwimg.h"
#endif



#ifndef ACTIVE_REGION

#warning "No active region defined, LORAMAC_REGION_EU868 will be used as default."

#define ACTIVE_REGION LORAMAC_REGION_EU868

#endif


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/*!
 * To overcome the SBSFU contraint. The FW file is copied from RAM to FLASH
 *      - SFU_HEADER & FW code will be copied at the rigth place in FLASH
 */
#define OVERCOME_SBSFU_CONSTRAINT       1

/*!
 * To select ACTILITY Smart Delta library instead of STM Partial Update
 * library set ACTILITY_LIBRARY to 1 and STM_LIBRARY to 0
 * NOTE: Should be defined in project config
 *
#define ACTILITY_LIBRARY                1
#define STM_LIBRARY                     0
 */

/*!
 * In interop test, the downloaded file is stored in RAM (Caching mode)
 *      - 0 -> the file is stored in FLASH
 *      - 1 -> The file is stored in RAM
 * Note: Due to SBSFU contraint, in the current version we always have to keep in RAM the
 * storing of the file whatever the running mode (Interop test or reel File download).
 * Defined via compiler -D
 *
 * #define INTEROP_TEST_MODE               1
 */


#define LORAWAN_MAX_BAT   254

#define LPP_APP_PORT 99

/*!
 * Defines the application data transmission duty cycle. 10s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            10000
/*!
 * Defines a random delay for application data transmission duty cycle. 5s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        5000
/*!
 * LoRaWAN Adaptive Data Rate
 * @note Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_STATE                           LORAMAC_HANDLER_ADR_ON
/*!
 * LoRaWAN Default data Rate Data Rate
 * @note Please note that LORAWAN_DEFAULT_DATA_RATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATA_RATE                   DR_0
/*!
 * LoRaWAN application port
 * @note do not use 224. It is reserved for certification
 */
#define LORAWAN_APP_PORT                            2
/*!
 * LoRaWAN default endNode class port
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A
/*!
 * LoRaWAN default confirm state
 */
#define LORAWAN_DEFAULT_CONFIRMED_MSG_STATE         LORAMAC_HANDLER_UNCONFIRMED_MSG
/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE            242
/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        true

/*
 * Firmware file magic to distinguish from binary
 */
#define FIRMWARE_MAGIC								0x4D554653
/*!
 * User application buffer
 */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];
/*!
 * User application data structure
 */
LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* call back when LoRa endNode has received a frame*/
static void LORA_RxData(LmHandlerAppData_t *AppData, LmHandlerRxParams_t *params);

/* callback to get the battery level in % of full charge (254 full charge, 0 no charge)*/
static uint8_t LORA_GetBatteryLevel(void);

/* start the tx process*/
static void LoraStartTx(void);

/* tx timer callback function*/
static void OnTxTimerEvent(void *context);

/*!
 * Timer to handle the application data transmission duty cycle
 */
TimerEvent_t TxTimer;            /* to open the timer to the RemoteMulticast*/



static void OnMacProcessNotify(void);
static void OnNvmContextChange(LmHandlerNvmContextStates_t state);
static void OnNetworkParametersChange(CommissioningParams_t *params);
static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq);
static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq);
static void OnJoinRequest(LmHandlerJoinParams_t *params);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnClassChange(DeviceClass_t deviceClass);
static void OnBeaconStatusChange(LoRaMAcHandlerBeaconParams_t *params);
static void OnSysTimeUpdate(void);
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
#if( STM_LIBRARY == 1 )
static uint8_t FragDecoderWrite(uint32_t addr, uint8_t *data, uint32_t size);
static uint8_t FragDecoderRead(uint32_t addr, uint8_t *data, uint32_t size);
#endif
#endif

static void OnFragProgress(uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost);

#if ( STM_LIBRARY == 1 )
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static void OnFragDone(int32_t status, uint32_t size);
#else
static void OnFragDone(int32_t status, uint8_t *file, uint32_t size);
#endif
#endif // STM_LIBRARY == 1

#if ( ACTILITY_LIBRARY == 1 )
static void OnUpdateAgentFragDone(int32_t status, uint32_t size);
static void NewImageValidate ( void *params ); 
#endif // ACTILITY_LIBRARY
static void UplinkProcess(void);

#if ( ACTILITY_LIBRARY == 0 )
/*!
 * Computes a CCITT 32 bits CRC
 *
 * \param [IN] buffer   Data buffer used to compute the CRC
 * \param [IN] length   Data buffer length
 *
 * \retval crc          The computed buffer of length CRC
 */
static uint32_t Crc32(uint8_t *buffer, uint16_t length);
#endif /* ACTILITY_LIBRARY == 0 */

/* Private variables ---------------------------------------------------------*/
/* load Main call backs structure*/
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel = LORA_GetBatteryLevel, /*BoardGetBatteryLevel,*/
  .GetTemperature = HW_GetTemperatureLevel, /*NULL,*/
  .GetUniqueId = HW_GetUniqueId, /*BoardGetUniqueId,*/
  .GetRandomSeed = HW_GetRandomSeed, /*BoardGetRandomSeed,*/
  .OnMacProcess = OnMacProcessNotify,
  .OnNvmContextChange = OnNvmContextChange,
  .OnNetworkParametersChange = OnNetworkParametersChange,
  .OnMacMcpsRequest = OnMacMcpsRequest,
  .OnMacMlmeRequest = OnMacMlmeRequest,
  .OnJoinRequest = OnJoinRequest,
  .OnTxData = OnTxData,
  .OnRxData = LORA_RxData, /*OnRxData,*/
  .OnClassChange = OnClassChange,
  .OnBeaconStatusChange = OnBeaconStatusChange,
  .OnSysTimeUpdate = OnSysTimeUpdate
};

static LmHandlerParams_t LmHandlerParams =
{
  .Region = ACTIVE_REGION,
  .AdrEnable = LORAWAN_ADR_STATE,
  .TxDatarate = LORAWAN_DEFAULT_DATA_RATE,
  .PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK,
  .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
  .DataBufferMaxSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE,
  .DataBuffer = AppDataBuffer
};

static LmhpComplianceParams_t LmhpComplianceParams =
{
  .AdrEnabled = LORAWAN_ADR_STATE,
  .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
  .StopPeripherals = NULL,
  .StartPeripherals = NULL,
};

/*!
 * Defines the maximum size for the buffer receiving the fragmentation result.
 *
 * \remark By default FragDecoder.h defines:
 *         \ref FRAG_MAX_NB   313
 *         \ref FRAG_MAX_SIZE 216
 *
 *         In interop test mode will be
 *         \ref FRAG_MAX_NB   21
 *         \ref FRAG_MAX_SIZE 50
 *
 *         FileSize = FRAG_MAX_NB * FRAG_MAX_SIZE
 *
 *         If bigger file size is to be received or is fragmented differently
 *         one must update those parameters.
 *
 * \remark  Memory allocation is done at compile time. Several options have to be foreseen
 *          in order to optimize the memory. Will depend of the Memory management used
 *          Could be Dynamic allocation --> malloc method
 *          Variable Length Array --> VLA method
 *          pseudo dynamic allocation --> memory pool method
 *          Other option :
 *          In place of using the caching memory method we can foreseen to have a direct
 *          flash memory copy. This solution will depend of the SBSFU constraint
 *
 */

#define UNFRAGMENTED_DATA_SIZE                     ( FRAG_MAX_NB * FRAG_MAX_SIZE )


#if ( INTEROP_TEST_MODE == 1 )
/*
 * Un-fragmented data storage. Only if datafile fragments are
 * collected in RAM
 */
static uint8_t UnfragmentedData[UNFRAGMENTED_DATA_SIZE];

/*
 * storage_GetSourceAreaInfo() is defined weak in storage.c
 * If we collect datafile fragments in RAM this definition
 * will overwrite one in storage.c
 */
uint32_t storage_GetSourceAreaInfo(SFU_FwImageFlashTypeDef *pArea) {
  uint32_t ret;
  if (pArea != NULL)
  {
    pArea->DownloadAddr = (uint32_t)UnfragmentedData;
    pArea->MaxSizeInBytes = (uint32_t)UNFRAGMENTED_DATA_SIZE;
    pArea->ImageOffsetInBytes = 0;
    ret =  HAL_OK;
  }
  else
  {
    ret = HAL_ERROR;
  }
  return ret;
}
#endif /* DATAFILE_USE_RAM_SOURCE == 1 */

#if ( STM_LIBRARY == 1 )

static LmhpFragmentationParams_t FragmentationParams =
{
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
  .DecoderCallbacks =
  {
    .FragDecoderWrite = FragDecoderWrite,
    .FragDecoderRead = FragDecoderRead,
  },
#else
  .Buffer = UnfragmentedData,
  .BufferSize = UNFRAGMENTED_DATA_SIZE,
#endif
  .OnProgress = OnFragProgress,
  .OnDone = OnFragDone
};
#endif // STM_LIBRARY == 1
#if ( ACTILITY_LIBRARY == 1 )

static LmhpFragmentationParams_t FragmentationParams =
{
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
  .DecoderCallbacks =
  {
    .FragDecoderWrite = FragDecoderActilityWrite,
    .FragDecoderRead = FragDecoderActilityRead,
  },
#else
  .Buffer = UnfragmentedData,
  .BufferSize = UNFRAGMENTED_DATA_SIZE,
#endif
  .OnProgress = OnFragProgress,           // should be OnUpdateAgentFragProgress to use Actility lib handling progress
  .OnDone = OnUpdateAgentFragDone         // the only API hook with Actility lib right now
};

static LmhpFWManagementParams_t FWManagementParams =
{
    .ImageValidate = NewImageValidate,    // Validate image when reception is done
    .NewImageValidateStatus = UPIMG_STATUS_ABSENT,
    .NewImageFWVersion = 0
};
#endif

/*!
 * Indicates if LoRaMacProcess call is pending.
 *
 * \warning If variable is equal to 0 then the MCU can be set in low power mode
 */
static volatile uint8_t IsMacProcessPending = 0;
/*!
 * Indicates if a Tx frame is pending.
 *
 * \warning Set to 1 when OnTxTimerEvent raised
 */
static volatile uint8_t IsTxFramePending = 0;

/*
 * Indicates if the system time has been synchronized
 */
static volatile bool IsClockSynched = false;

/*
 * MC Session Started
 */
static volatile bool IsMcSessionStarted = false;

/*
 * Indicates if the file transfer is done
 */
static volatile bool IsFileTransferDone = false;

/*
 *  Received file computed CRC32
 */
static volatile uint32_t FileRxCrc = 0;



static void OnMacProcessNotify(void)
{
  IsMacProcessPending = 1;
}

/*************************************************************************************************/

static bool volatile AppProcessRequest = false;
/*!
 * Specifies the state of the application LED
 */
static uint8_t AppLedStateOn = RESET;


int test_fragments_storage(void);


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32 HAL library initialization*/
  HAL_Init();

  /* Configure the system clock*/
  SystemClock_Config();

  /* Configure the debug mode*/
  DBG_Init();

  /* Configure the hardware*/
  HW_Init();

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /*Disbale Stand-by mode*/
  LPM_SetOffMode(LPM_APPLI_Id, LPM_Disable);

  PRINTF("\r\nAPP_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__APP_VERSION >> 24), (uint8_t)(__APP_VERSION >> 16), (uint8_t)(__APP_VERSION >> 8), (uint8_t)__APP_VERSION);
  PRINTF("MAC_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__LORA_MAC_VERSION >> 24), (uint8_t)(__LORA_MAC_VERSION >> 16), (uint8_t)(__LORA_MAC_VERSION >> 8), (uint8_t)__LORA_MAC_VERSION);
  PRINTF("HW_VERSION= %02X.%02X.%02X.%02X\r\n", (uint8_t)(__HW_VERSION >> 24), (uint8_t)(__HW_VERSION >> 16), (uint8_t)(__HW_VERSION >> 8), (uint8_t)__HW_VERSION);
  
  /* Configure the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks, &LmHandlerParams);


  /*The LoRa-Alliance Compliance protocol package should always be initialized and activated.*/
  LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams);

  LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC, NULL);

  LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP, NULL);

  LmHandlerPackageRegister(PACKAGE_ID_FRAGMENTATION, &FragmentationParams);
  
  LmHandlerPackageRegister(PACKAGE_ID_FWMANAGEMENT, &FWManagementParams);

  PRINTF("\n\rTAG to VALIDATE new FW upgrade %d\n\r", __APP_VERSION_RC);


  storage_init();

#if	( ACTILITY_SMART_DELTA == 1 )
  patch_init();
#endif 	/*  ACTILITY_SMART_DELTA == 1  */

  IsClockSynched = false;

  IsFileTransferDone = false;

  /* first join request*/
  LmHandlerJoin();

  /* first Tx frame transmission*/
  LoraStartTx() ;

  while (1)
  {
    /*Process application uplink*/
    UplinkProcess();

    /*Processes the LoRaMac events*/
    LmHandlerProcess();

    /*If a flag is set at this point, mcu must not enter low power and must loop*/
    DISABLE_IRQ();

    /* if an interrupt has occurred after DISABLE_IRQ, it is kept pending
     * and cortex will not enter low power anyway  */
    if ((IsMacProcessPending != 1) && (IsTxFramePending != 1))
    {
#ifndef LOW_POWER_DISABLE
      LPM_EnterLowPower();
#endif
    }
    else
    {
      /*reset notification flag*/
      IsMacProcessPending = 0;
    }

    ENABLE_IRQ();

    /* USER CODE BEGIN 2 */
    /* USER CODE END 2 */
  }
}


static void LORA_RxData(LmHandlerAppData_t *AppData, LmHandlerRxParams_t *params)
{
  if (AppData == NULL)
  {
	  PRINTF("PACKET RECEIVED WITH AppData == NULL\n\r");
	  return;
  }

  /* USER CODE BEGIN 4 */
  PRINTF("PACKET RECEIVED ON PORT %d\n\r", AppData->Port);

  switch (AppData->Port)
  {
    case 3:
      /*this port switches the class*/
      if (AppData->BufferSize == 1)
      {
        switch (AppData->Buffer[0])
        {
          case 0:
          {
            LmHandlerRequestClass(CLASS_A);
            break;
          }
          case 1:
          {
            LmHandlerRequestClass(CLASS_B);
            break;
          }
          case 2:
          {
            LmHandlerRequestClass(CLASS_C);
            break;
          }
          default:
            break;
        }
      }
      break;
    case LORAWAN_APP_PORT:
      if (AppData->BufferSize == 1)
      {
        AppLedStateOn = AppData->Buffer[0] & 0x01;
        if (AppLedStateOn == RESET)
        {
          PRINTF("LED OFF\n\r");
          LED_Off(LED_BLUE) ;
        }
        else
        {
          PRINTF("LED ON\n\r");
          LED_On(LED_BLUE) ;
        }
      }
      break;
    case LPP_APP_PORT:
    {
      AppLedStateOn = (AppData->Buffer[2] == 100) ?  0x01 : 0x00;
      if (AppLedStateOn == RESET)
      {
        PRINTF("LED OFF\n\r");
        LED_Off(LED_BLUE) ;

      }
      else
      {
        PRINTF("LED ON\n\r");
        LED_On(LED_BLUE) ;
      }
      break;
    }
    default:
      break;
  }
  /* USER CODE END 4 */
}

/**
  * @brief This function return the battery level
  * @param none
  * @retval the battery level  1 (very low) to 254 (fully charged)
  */
uint8_t LORA_GetBatteryLevel(void)
{
  uint16_t batteryLevelmV;
  uint8_t batteryLevel = 0;

  batteryLevelmV = HW_GetBatteryLevel();


  /* Convert batterey level from mV to linea scale: 1 (very low) to 254 (fully charged) */
  if (batteryLevelmV > VDD_BAT)
  {
    batteryLevel = LORAWAN_MAX_BAT;
  }
  else if (batteryLevelmV < VDD_MIN)
  {
    batteryLevel = 0;
  }
  else
  {
    batteryLevel = (((uint32_t)(batteryLevelmV - VDD_MIN) * LORAWAN_MAX_BAT) / (VDD_BAT - VDD_MIN));
  }

  return batteryLevel;
}


/********************************************************************************/

static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
  if (params->Status == LORAMAC_HANDLER_ERROR)
  {
    LmHandlerJoin();
  }
  else
  {
    PRINTF("\r\n.......  JOINED  .......\r\n");
    LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
  }
}


static void OnClassChange(DeviceClass_t deviceClass)
{
  PRINTF("\r\n...... Switch to Class %c done. .......\r\n", "ABC"[deviceClass]);

  switch (deviceClass)
  {
    default:
    case CLASS_A:
    {
      IsMcSessionStarted = false;
      break;
    }
    case CLASS_B:
    {
      /* Inform the server as soon as possible that the end-device has switched to ClassB */
      LmHandlerAppData_t appData =
      {
        .Buffer = NULL,
        .BufferSize = 0,
        .Port = 0
      };
      LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
      IsMcSessionStarted = true;
      break;
    }
    case CLASS_C:
    {
      IsMcSessionStarted = true;
      /* Switch LED 3 ON*/
#if (INTEROP_TEST_MODE == 1)
      LED_On(LED_BLUE) ;
#endif
      break;
    }
  }
}

static void OnFragProgress(uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost)
{
  /* Switch LED 3 OFF for each received downlink */
#if (INTEROP_TEST_MODE == 1)
  LED_Off(LED_BLUE) ;
#endif

  PRINTF("\r\n....... FRAG_DECODER in Progress .......\r\n");
  PRINTF("RECEIVED    : %5d / %5d Fragments\r\n", fragCounter, fragNb);
  PRINTF("              %5d / %5d Bytes\r\n", fragCounter * fragSize, fragNb * fragSize);
  PRINTF("LOST        :       %7d Fragments\r\n\r\n", fragNbLost);
}

#if( STM_LIBRARY == 1 )
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static void OnFragDone(int32_t status, uint32_t size)
{
  FileRxCrc = Crc32(UnfragmentedData, size);
  IsFileTransferDone = true;

#if (OVERCOME_SBSFU_CONSTRAINT == 1)
    /* copy the file from RAM to FLASH & ASK to reboot the device*/
    if (FwUpdateAgentDataTransferFromRamToFlash(UnfragmentedData, REGION_SLOT_1_START, size) == HAL_OK) {
      PRINTF("\r\n...... Transfer file RAM to Flash success --> Run  ......\r\n");
      FwUpdateAgentRun();
    } else {
      PRINTF("\r\n...... Transfer file RAM to Flash Failed  ......\r\n");
    }
#else
    /*Do a request to Run the Secure boot - The file is already in flash*/
    FwUpdateAgentRun();
#endif
  } else {
    PRINTF("...... Patch error:%d ......\r\n", patch_res);
  }

  /* Switch LED 3 OFF */
#if (INTEROP_TEST_MODE == 1)
  LED_Off(LED_BLUE) ;
#endif

  PRINTF("\r\n....... FRAG_DECODER Finished .......\r\n");
  PRINTF("STATUS      : %ld\r\n", status);
  PRINTF("CRC         : %08lX\r\n\r\n", FileRxCrc);
}
#else
static void OnFragDone(int32_t status, uint8_t *file, uint32_t size)
{
  FileRxCrc = Crc32(file, size);
  IsFileTransferDone = true;

  /* Switch LED 3 OFF */
#if (INTEROP_TEST_MODE == 1)
  LED_Off(LED_BLUE) ;
#endif

  PRINTF("\r\n....... FRAG_DECODER Finished .......\r\n");
  PRINTF("STATUS      : %ld\r\n", status);
  PRINTF("CRC         : %08lX\r\n\r\n", FileRxCrc);
}
#endif
#endif // STM_LIBRARY == 1

#if ( ACTILITY_LIBRARY == 1 )
// Code to process Smart Delta patch which is currently located in RAM
// Current Smart Delta processing function expects patch in RAM
static void OnUpdateAgentFragDone(int32_t status, uint32_t size) 
{
	uint8_t *datafile;

#if ( INTEROP_TEST_MODE == 1 )
  datafile = UnfragmentedData;
#else
  uint32_t ptr, len, crc;
  storage_get_slot_info(STORAGE_SLOT_SOURCE, &ptr, &len);
  datafile = (uint8_t *)ptr;
#endif	/* INTEROP_TEST_MODE == 1 */
  FileRxCrc = 0;
  if( storage_crc32(STORAGE_SLOT_SOURCE, 0, size, &crc) != STR_OK ) {
	  PRINTF("Failed to calc CRC32 in source slot\r\n");
  }
  FileRxCrc = crc;
  PRINTF("File size: %u CRC32: %x\r\n", size, FileRxCrc);
  if( *(uint32_t *)datafile != FIRMWARE_MAGIC)
  {
	  PRINTF("Binary file received, no firmware magic found\r\n");
	  goto cleanup;
  }
  datafile += HEADER_OFFSET;
  if( size <= HEADER_OFFSET )
  {
	  PRINTF("File size: %u less then: %u error\r\n", size, HEADER_OFFSET);
	  goto cleanup;
  }
#if	( ACTILITY_SMART_DELTA == 1 )
  size_t fwsize;
  if( SmartDeltaVerifyHeader (datafile) == SMARTDELTA_OK )
  {
  	fwsize = size - HEADER_OFFSET;
	if( SmartDeltaVerifySignature (datafile, fwsize) == SMARTDELTA_OK ) {

		  PRINTF("Patch size: %u\r\n", size);
		  patch_res_t patch_res = patch(size);
		  if (patch_res == patchDecoded) {
			FWManagementParams.ImageValidate( &FWManagementParams );
			PRINTF("\r\n...... Smart Delta Unpack from RAM to Flash Succeeded  ......\r\n");
		  } else if (patch_res == patchUnrecognized) {
			PRINTF("...... Patch unrecognized ......\r\n");
		  } else {
			PRINTF("Patch error:%d\r\n", patch_res);
		  }
	} else {
		  PRINTF("Invalid Smart Delta signature\r\n");
		  FWManagementParams.NewImageValidateStatus = UPIMG_STATUS_WRONG;
		  FWManagementParams.NewImageFWVersion = 0;
	}
  } else { /* Process full image upgrade */
#endif 	/* 	 ACTILITY_SMART_DELTA == 1  */
#if ( INTEROP_TEST_MODE == 1 )
    /* copy the file from RAM to FLASH */
    if (FwUpdateAgentDataTransferFromRamToFlash(UnfragmentedData, REGION_SLOT_1_START, size) == HAL_OK) {
    	PRINTF("\r\n...... Transfer full image from RAM to Flash success ......\r\n");
	} else {
	    PRINTF("\r\n...... Transfer full image from RAM to Flash Failed  ......\r\n");
	}
#else
    if( move_image(STORAGE_SLOT_SOURCE, STORAGE_SLOT_SCRATCH, size, 1) == STR_OK ) {
    	PRINTF("\r\n...... Transfer full image from Swap to Slot1 success ......\r\n");
    	FWManagementParams.ImageValidate( &FWManagementParams );
    } else {
    	PRINTF("\r\n...... Transfer full image from Swap to Slot1 Failed  ......\r\n");
    }
#endif	/* INTEROP_TEST_MODE == 1 */
#if	( ACTILITY_SMART_DELTA == 1 )
  }
#endif  /*  ACTILITY_SMART_DELTA == 1  */
  /*
   * All fragments received and processed. Switch to Class A
   * and signal multicast session is finished
   */
cleanup:

  LmHandlerRequestClass( CLASS_A );
  IsMcSessionStarted = false;
  IsFileTransferDone = true;
}

static void NewImageValidate ( void *params ) {
  // TODO: Implement new image validation function
  //       verify hardware version is ok 
  //       verify image MAC is ok
  ((LmhpFWManagementParams_t *)params)->NewImageValidateStatus = UPIMG_STATUS_VALID;
  // Right now we do not have 4-byte FW & HW version in the header
  // and we can't validate them. Until they are available in the header
  // we assume that we always load version with SUB2 + 1
  ((LmhpFWManagementParams_t *)params)->NewImageFWVersion = __APP_VERSION + ( 1U << 8 );
  
}
#endif // ACTILITY_LIBRARY == 1

static void LoraStartTx(void)
{
  /* send every time timer elapses */
  TimerInit(&TxTimer, OnTxTimerEvent);
  TimerSetValue(&TxTimer, APP_TX_DUTYCYCLE  + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND));
  OnTxTimerEvent(NULL);

}

/*!
 * Function executed on TxTimer event to process an uplink frame
 */
static void UplinkProcess(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  uint8_t isPending = 0;
  CRITICAL_SECTION_BEGIN();
  isPending = IsTxFramePending;
  IsTxFramePending = 0;
  CRITICAL_SECTION_END();
  if (isPending == 1)
  {
    if (LmHandlerIsBusy() == true)
    {
      return;
    }

    if (IsMcSessionStarted == false)    /* we are in Class A*/
    {
      if (IsClockSynched == false)    /* we request AppTimeReq to allow FUOTA */
      {
        status = LmhpClockSyncAppTimeReq();
      }
      else
      {
        AppDataBuffer[0] = randr(0, 255);
        /* Send random packet */
        LmHandlerAppData_t appData =
        {
          .Buffer = AppDataBuffer,
          .BufferSize = 1,
          .Port = 1
        };
        status = LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
        PRINTF(" Uplink sent status: %d\n\r", status); // *olg*TMP
      }
    }
    else  /* Now we are in Class C or in Class B -- FUOTA feature could be activated */
    {
      if (IsFileTransferDone == false)
      {
        /* do nothing up to the transfer done or sent a data user */
      }
      else
      {
        AppDataBuffer[0] = 0x05; // FragDataBlockAuthReq
        AppDataBuffer[1] = FileRxCrc & 0x000000FF;
        AppDataBuffer[2] = (FileRxCrc >> 8) & 0x000000FF;
        AppDataBuffer[3] = (FileRxCrc >> 16) & 0x000000FF;
        AppDataBuffer[4] = (FileRxCrc >> 24) & 0x000000FF;

        /* Send FragAuthReq */
        LmHandlerAppData_t appData =
        {
          .Buffer = AppDataBuffer,
          .BufferSize = 5,
          .Port = 201
        };
        status = LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
      }
      IsFileTransferDone = false;
      if (status == LORAMAC_HANDLER_SUCCESS)
      {
        /* The fragmented transport layer V1.0 doesn't specify any behavior*/
        /* we keep the interop test behavior - CRC32 is returned to the server*/
        PRINTF(" CRC send \n\r");
      }
    }
    /* send application frame - could be put in conditional compilation*/
    /*  Send(NULL);  comment the sending to avoid interference during multicast*/
  }
}

#if (ACTILITY_LIBRARY == 0)
static uint32_t Crc32(uint8_t *buffer, uint16_t length)
{
  // The CRC calculation follows CCITT - 0x04C11DB7
  const uint32_t reversedPolynom = 0xEDB88320;

  // CRC initial value
  uint32_t crc = 0xFFFFFFFF;

  if (buffer == NULL)
  {
    return 0;
  }

  for (uint16_t i = 0; i < length; ++i)
  {
    crc ^= (uint32_t)buffer[i];
    for (uint16_t i = 0; i < 8; i++)
    {
      crc = (crc >> 1) ^ (reversedPolynom & ~((crc & 0x01) - 1));
    }
  }

  return ~crc;
}
#endif /* ACTILITY_LIBRARY == 0 */

/******************** additional code for trace usage ********************/

#if 1
static void OnBeaconStatusChange(LoRaMAcHandlerBeaconParams_t *params)
{
  switch (params->State)
  {
    case LORAMAC_HANDLER_BEACON_RX:
    {
      PRINTF("Beacon_Rx\r\n");
      break;
    }
    case LORAMAC_HANDLER_BEACON_LOST:
    case LORAMAC_HANDLER_BEACON_NRX:
    {
      PRINTF("Beacon_NRx_Lost\r\n");
      break;
    }
    default:
    {
      break;
    }
  }

  PRINTF("BeaconUpdate %d\r\n", params->State);
}
#endif

static void OnSysTimeUpdate(void)
{
  IsClockSynched = true;
}

#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
#if( STM_LIBRARY == 1 )
static uint8_t FragDecoderWrite(uint32_t addr, uint8_t *data, uint32_t size)
{
  if (size >= UNFRAGMENTED_DATA_SIZE)
  {
    return (uint8_t) - 1; /* Fail */
  }

  for (uint32_t i = 0; i < size; i++)
  {
    UnfragmentedData[addr + i] = data[i];
  }
  return 0; // Success
}

static uint8_t FragDecoderRead(uint32_t addr, uint8_t *data, uint32_t size)
{
  if (size >= UNFRAGMENTED_DATA_SIZE)
  {
    return (uint8_t) - 1; /* Fail */
  }

  for (uint32_t i = 0; i < size; i++)
  {
    data[i] = UnfragmentedData[addr + i];
  }
  return 0; // Success
}
#endif
#endif

#if 1

static void OnNvmContextChange(LmHandlerNvmContextStates_t state)
{
  PRINTF("OnNvmContextChange\r\n");
}
static void OnNetworkParametersChange(CommissioningParams_t *params)
{

  PRINTF("OnNwkParamsUpdate\r\n");

  PRINTF("DevEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", params->DevEui[0], params->DevEui[1],
         params->DevEui[2], params->DevEui[3],
         params->DevEui[4], params->DevEui[5],
         params->DevEui[6], params->DevEui[7]);

  PRINTF("AppEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", params->JoinEui[0], params->JoinEui[1],
         params->JoinEui[2], params->JoinEui[3],
         params->JoinEui[4], params->JoinEui[5],
         params->JoinEui[6], params->JoinEui[7]);

  PRINTF("AppKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r",
         params->NwkKey[0], params->NwkKey[1],
         params->NwkKey[2], params->NwkKey[3],
         params->NwkKey[4], params->NwkKey[5],
         params->NwkKey[6], params->NwkKey[7],
         params->NwkKey[8], params->NwkKey[9],
         params->NwkKey[10], params->NwkKey[11],
         params->NwkKey[12], params->NwkKey[13],
         params->NwkKey[14], params->NwkKey[15]);

  PRINTF("GenAppKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r",
         params->GenAppKey[0], params->GenAppKey[1],
         params->GenAppKey[2], params->GenAppKey[3],
         params->GenAppKey[4], params->GenAppKey[5],
         params->GenAppKey[6], params->GenAppKey[7],
         params->GenAppKey[8], params->GenAppKey[9],
         params->GenAppKey[10], params->GenAppKey[11],
         params->GenAppKey[12], params->GenAppKey[13],
         params->GenAppKey[14], params->GenAppKey[15]);
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq)
{

  switch (mcpsReq->Type)
  {
    case MCPS_CONFIRMED:
    {
      PRINTF("\r\n.......  MCPS_CONFIRMED_Req  .......\r\n");
      break;
    }
    case MCPS_UNCONFIRMED:
    {
      PRINTF("\r\n.......  MCPS_UNCONFIRMED_Req  .......\r\n");
      break;
    }
    case MCPS_PROPRIETARY:
    {
      PRINTF("\r\n.......  MCPS_PROPRIETARY_Req  .......\r\n");
      break;
    }
    default:
    {
      PRINTF("\r\n.......  MCPS_ERROR_Req  .......\r\n");
      break;
    }
  }

}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq)
{

  switch (mlmeReq->Type)
  {
    case MLME_JOIN:
    {
      PRINTF("\r\n.......  MLME_JOIN_Req  .......\r\n");
      break;
    }
    case MLME_LINK_CHECK:
    {
      PRINTF("\r\n.......  MLME_LINK_CHECK_Req  .......\r\n");
      break;
    }
    case MLME_DEVICE_TIME:
    {
      PRINTF("\r\n.......  MLME-DEVICE_TIME_Req  .......\r\n");
      break;
    }
    case MLME_TXCW:
    {
      PRINTF("\r\n.......  MLME_TXCW_Req  .......\r\n");
      break;
    }
    case MLME_TXCW_1:
    {
      PRINTF("\r\n.......  MLME_TXCW_1_Req  .......\r\n");
      break;
    }
    default:
    {
      PRINTF("\r\n.......  MLME_UNKNOWN_Req  .......\r\n");
      break;
    }
  }

}
#endif


/******************** callback to trace the Rx and Tx  params******************/
#if 1
static void OnTxData(LmHandlerTxParams_t *params)
{

  if (params->IsMcpsConfirm == 0)
  {
    PRINTF("\r\n....... OnTxData (Mlme) .......\r\n");
    return;
  }

  PRINTF("\r\n....... OnTxData (Mcps) .......\r\n");
}
#endif


/*!
 * Function executed on TxTimer event
 */
static void OnTxTimerEvent(void *context)
{
  TimerStop(&TxTimer);

  IsTxFramePending = 1;

  // Schedule next transmission
  TimerSetValue(&TxTimer, APP_TX_DUTYCYCLE + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND));
  TimerStart(&TxTimer);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
