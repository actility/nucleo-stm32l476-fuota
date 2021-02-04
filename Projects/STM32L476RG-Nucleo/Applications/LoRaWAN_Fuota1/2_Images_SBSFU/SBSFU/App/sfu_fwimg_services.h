/**
  ******************************************************************************
  * @file    sfu_fwimg_services.h
  * @author  MCD Application Team
  * @brief   This file contains the 2 images handling service (SFU_FWIMG functionalities)
  *          API definitions.
  *          These services can be called by the bootloader to deal with images handling.
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
#ifndef SFU_FWIMG_SERVICES_H
#define SFU_FWIMG_SERVICES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "se_def.h"
#include "sfu_def.h"

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_IMG
  * @{
  */


/** @addtogroup SFU_IMG_SERVICES
  * @{
  */

/** @defgroup SFU_IMG_Exported_Types Exported Types for FWIMG services
  * @brief Exported Types for FWIMG services
  * @{
  */

/**
  * @brief  SFU_IMG Initialization Status Type Definition
  */
typedef enum
{
  SFU_IMG_INIT_OK = 0x0U,                /*!< SFU Firmware Image Handling (FWIMG) Init OK */
  SFU_IMG_INIT_SLOTS_SIZE_ERROR,         /*!< error related to slots size */
  SFU_IMG_INIT_SWAP_SETTINGS_ERROR,      /*!< error related to swap settings */
  SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR,  /*!< error related to flash constraints */
  SFU_IMG_INIT_CRYPTO_CONSTRAINTS_ERROR, /*!< error related to crypto constraints */
  SFU_IMG_INIT_ERROR                     /*!< Init is FAILED: unspecified error */
} SFU_IMG_InitStatusTypeDef;

/**
  * @brief  SFU_IMG Image Installation State Type Definition
  */
typedef enum
{
  SFU_IMG_FWIMAGE_TO_INSTALL = 0x0U,   /*!< There is a FW image to be installed */
  SFU_IMG_FWUPDATE_STOPPED,            /*!< A previous installation has been interrupted before it completed
                                           (recovery pending) */
  SFU_IMG_NO_FWUPDATE,                 /*!< No FW image installation pending */
} SFU_IMG_ImgInstallStateTypeDef;

/**
  * @}
  */

/** @addtogroup SFU_IMG_Exported_Functions FW Images Handling Services
  * @{
  */

/** @addtogroup SFU_IMG_Initialization_Functions FW Images Handling Init. Functions
  * @{
  */
SFU_IMG_InitStatusTypeDef SFU_IMG_InitImageHandling(void);
SFU_ErrorStatus SFU_IMG_ShutdownImageHandling(void);

/**
  * @}
  */

/** @addtogroup SFU_IMG_Service_Functions FW Images Handling Services Functions
  * @{
  */


/** @addtogroup SFU_IMG_Service_Functions_NewImg New image installation services
  * @{
  */
SFU_IMG_ImgInstallStateTypeDef SFU_IMG_CheckPendingInstallation(void);
SFU_ErrorStatus SFU_IMG_CheckCandidateMetadata(void);
SFU_ErrorStatus SFU_IMG_TriggerImageInstallation(void);
SFU_ErrorStatus SFU_IMG_TriggerResumeInstallation(void);
SFU_ErrorStatus SFU_IMG_EraseDownloadedImg(void);
SFU_ErrorStatus SFU_IMG_VerifyDownloadSlot(void);
uint32_t SFU_IMG_GetTrailerSize(void);
SFU_ErrorStatus SFU_IMG_CheckFwVersion(int32_t CurrentVersion, int32_t CandidateVersion);

/**
  * @}
  */

/** @addtogroup SFU_IMG_Service_Functions_CurrentImage Active Firmware services (current image)
  * @{
  */
SFU_ErrorStatus SFU_IMG_InvalidateCurrentFirmware(void);
SFU_ErrorStatus SFU_IMG_VerifyActiveImgMetadata(void);
SFU_ErrorStatus SFU_IMG_VerifyActiveImg(void);
SFU_ErrorStatus SFU_IMG_VerifyActiveSlot(void);
SFU_ErrorStatus SFU_IMG_VerifyEmptyActiveSlot(void);
SFU_ErrorStatus SFU_IMG_LaunchActiveImg(void);
int32_t SFU_IMG_GetActiveFwVersion(void);
SFU_ErrorStatus SFU_IMG_HasValidActiveFirmware(void);
SFU_ErrorStatus SFU_IMG_Validation(uint8_t *pHeader);
SFU_ErrorStatus SFU_IMG_ControlActiveImgTag(void);
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

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* SFU_FWIMG_SERVICES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

