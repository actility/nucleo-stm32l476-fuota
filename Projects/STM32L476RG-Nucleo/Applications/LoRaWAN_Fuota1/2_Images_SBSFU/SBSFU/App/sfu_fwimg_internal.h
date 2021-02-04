/**
  ******************************************************************************
  * @file    sfu_fwimg_internal.h
  * @author  MCD Application Team
  * @brief   This file contains internal definitions (private) for SFU_FWIMG functionalities.
  *          This file should be included only by sfu_fwimg_core.c and sfu_fwimg_services.c.
  *          Nevertheless, the SFU_KMS module is allowed to include it to re-use the variable
  *          fw_image_header_to_test to install an KMS blob.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SFU_FWIMG_INTERNAL_H
#define SFU_FWIMG_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "se_def.h"

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_IMG
  * @{
  */

/** @defgroup SFU_IMG_COMMON Common definitions for FWIMG module.
  * @brief This file contains common FWIMG definitions.
  *        This file should be included only by sfu_fwimg_core.c and sfu_fwimg_services.c.
  * @{
  */


/** @defgroup SFU_IMG_Exported_Defines_COMMON Common FWIMG defines
  * @brief Common defines for sfu_fwimg_core.c and sfu_fwimg_services.c.
  * @{
  */

/**
  * @brief FW Header Info offset
  * These defines are in the common part to keep them altogether.
  * The only one really needed in the common part is: FW_INFO_TOT_LEN.
  */
#define FW_HEADER_SIZE_OF(A)             (sizeof(((SE_FwRawHeaderTypeDef *)0)->A))
#define FW_HEADER_TOT_LEN                (sizeof(SE_FwRawHeaderTypeDef))     /*!< FW INFO Total Len*/
#define FW_HEADER_MAC_LEN                FW_HEADER_SIZE_OF(HeaderMAC)        /*!< FW Header MAC Len*/


/**
  * @brief FW Header Authentication field
  */
#define FW_INFO_TOT_LEN                (FW_HEADER_TOT_LEN)    /*!< FW INFO Total Len*/
#define FW_INFO_MAC_LEN                (FW_HEADER_MAC_LEN)    /*!< FW INFO MAC Len*/


/**
  *  2 images + swap slot in FLASH
  */
#define SFU_SLOTS 3

/** Header position in slot #0 */
#define SLOT_0_HDR (( uint8_t *)(SFU_IMG_SLOT_0_REGION_BEGIN))
/** Header position in slot #1 */
#define SLOT_1_HDR (( uint8_t *)(SFU_IMG_SLOT_1_REGION_BEGIN))
/** Header position in swap area */
#define SWAP_HDR (( uint8_t *)(SFU_IMG_SWAP_REGION_BEGIN))


/**
  * @}
  */

/** @defgroup SFU_IMG_Private_Macros_COMMON Common FWIMG Macros
  * @brief  Common macros for sfu_fwimg_core.c and sfu_fwimg_services.c.
  * @{
  */

/**
  * @brief  Status Macro
  * This macros aims at capturing abnormal errors in the FWIMG sub-module (typically FLASH errors).
  * When SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE is activated this macro blocks the execution.
  * When SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE is deactivated, a log is printed in the console (if SFU_DEBUG_MODE is
  * activated) and the execution continues.
  */
#if defined(SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE)
#define StatusFWIMG(B,A) if (B) { \
                                  SFU_IMG_Status=A; \
                                  SFU_IMG_Line = __LINE__; \
                                  TRACE("\r\n          Abnormal error %d at line %d in %s - BLOCK", SFU_IMG_Status, \
                                         SFU_IMG_Line, __FILE__); \
                                  while(1); \
                                } while(0);
#else
#define StatusFWIMG(B,A) if (B) { \
                                  SFU_IMG_Status=A; \
                                  SFU_IMG_Line = __LINE__; \
                                  TRACE("\r\n          Abnormal error %d at line %d in %s - CONTINUE", SFU_IMG_Status, \
                                         SFU_IMG_Line, __FILE__); \
                                } while(0);
#endif /* SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE */

/**
  * @}
  */


/** @defgroup SFU_IMG_Types_COMMON Common FWIMG Types
  * @brief Common types for sfu_fwimg_core.c and sfu_fwimg_services.c.
  * @{
  */

/**
  * @brief Payload Buffer descriptor.
  * This structure describes how a Firmware is split in Flash.
  * In the nominal case a Firmware is stored in 1 contiguous area.
  * But, a Firmware can be split in up to 2 areas (typically when a FW installation procedure is interrupted or after a
  * decrypt operation).
  */
typedef struct
{
  uint8_t *pPayload[2];               /*!<  table containing payload pointer*/
  int32_t  PayloadSize[2];            /*!<  table containing Payload Size*/
} SE_Ex_PayloadDescTypeDef;


/**
  * @brief SFU_IMG Flash Status Type Definition
  * Status of a FLASH operation.
  */
typedef enum
{
  SFU_IMG_OK = 0x0U,             /*!< No problem reported */
  SFU_IMG_FLASH_ERASE_FAILED,    /*!< FLASH erase failure */
  SFU_IMG_FLASH_WRITE_FAILED,    /*!< FLASH write failure */
  SFU_IMG_FLASH_READ_FAILED,     /*!< FLASH read failure */
} SFU_IMG_StatusTypeDef;

/**
  * @}
  */


/** @defgroup SFU_IMG_COMMON_Variables Common FWIMG Module Variables
  * @brief Common variables for sfu_fwimg_core.c and sfu_fwimg_services.c.
  * @{
  */

#if defined(SFU_FWIMG_CORE_C)
/**
  * Addresses of the FW header in the FW slots in FLASH
  */
const uint32_t  SlotHeaderAddress[SFU_SLOTS ] = { (uint32_t)SLOT_0_HDR, (uint32_t) SLOT_1_HDR, (uint32_t)SWAP_HDR };

/**
  * FW header (metadata) of the active FW in slot #0: structured format (access by fields)
  */
SE_FwRawHeaderTypeDef fw_image_header_validated;

/**
  * FW header (metadata) of the candidate FW in slot #1: structured format (access by fields)
  */
SE_FwRawHeaderTypeDef fw_image_header_to_test;

/**
  *  FWIMG status variables used to log errors and display debug messages.
  *  This is related to FLASH operations.
  *  This is handled with StatusFWIMG.
  */
SFU_IMG_StatusTypeDef SFU_IMG_Status;
uint32_t SFU_IMG_Line;

#elif defined(SFU_FWIMG_SERVICES_C)

extern uint32_t  SlotHeaderAddress[SFU_SLOTS ];
extern uint32_t SFU_IMG_Line;
extern SE_FwRawHeaderTypeDef fw_image_header_validated;
extern uint8_t fw_header_validated[FW_INFO_TOT_LEN];
extern SFU_IMG_StatusTypeDef SFU_IMG_Status;

extern SE_FwRawHeaderTypeDef fw_image_header_to_test;

#endif /* SFU_FWIMG_CORE_C */

/**
  * @}
  */


/** @defgroup SFU_IMG_Functions_COMMON Common FWIMG Functions
  * @brief Common functions for sfu_fwimg_core.c and sfu_fwimg_services.c.
  * @{
  */

/** @defgroup SFU_IMG_Functions_COMMON_LowLevel Images Handling Low Level Services
  * @brief Common functions dealing with the internals of the images handling.
  * @{
  */
/* Check the trailer validity */
SFU_ErrorStatus  SFU_IMG_CheckTrailerValid(void);

/**
  * @}
  */

/** @defgroup SFU_IMG_Functions_COMMON_HighLevel Images Handling High Level Services
  * @brief Common functions providing high level services for the Images and Headers handling.
  * @{
  */
/* FWIMG core init */
SFU_IMG_InitStatusTypeDef SFU_IMG_CoreInit(void);
/* FWIMG core shutdown */
SFU_ErrorStatus SFU_IMG_CoreDeInit(void);
/* check if the content of slot #0 has been tagged as VALID by the bootloader  */
SFU_ErrorStatus  SFU_IMG_CheckSlot0FwValid(void);
/*  slot #0 = active Firmware , slot #1 = downloaded image or backed-up image */
SFU_ErrorStatus SFU_IMG_GetFWInfoMAC(SE_FwRawHeaderTypeDef *pSE_FwImageHeader, uint32_t slot);
/*
 * if slot 0, check image
 * if slot 1 and SE_INFO_FW_STATUS_DECRYPTED check image in position after decryption
 * if slot 1 and SE_INFO_FW_STATUS_SWAP check image in position given by trailer information
 */
SFU_ErrorStatus SFU_IMG_VerifyFwSignature(SE_StatusTypeDef  *pSeStatus, SE_FwRawHeaderTypeDef *pFwImageHeader,
                                          uint32_t slot, int32_t SE_FwType);
/* tag the active FW in slot #0 as VALID */
SFU_ErrorStatus SFU_IMG_WriteHeaderValidated(uint8_t *pHeader);
/* Check if no additional code beyond FW image in a slot */
SFU_ErrorStatus SFU_IMG_VerifySlot(uint8_t *pSlotBegin, uint32_t uSlotSize, uint32_t uFwSize);
/* Control FW tag */
SFU_ErrorStatus SFU_IMG_ControlFwTag(uint8_t *pTag);

/* FW installation resume */
SFU_ErrorStatus SFU_IMG_Resume(void);
/* Check if there is a FW to install */
SFU_ErrorStatus SFU_IMG_FirmwareToInstall(void);
/* Prepare Candidate FW image for Installation */
SFU_ErrorStatus SFU_IMG_PrepareCandidateImageForInstall(void);
/* Install a FW image */
SFU_ErrorStatus SFU_IMG_InstallNewVersion(void);
/* check if there is a valid FW in slot #1 */
SFU_ErrorStatus SFU_IMG_ValidFwInSlot1(uint16_t MinFwVersion, uint16_t MaxFwVersion);
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */


#ifdef __cplusplus
}
#endif

#endif /* SFU_FWIMG_INTERNAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

