/**
  ******************************************************************************
  * @file    sfu_loader.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for Secure Firmware Update local
  *          loader.
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
#ifndef SFU_LOADER_H
#define SFU_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "se_def.h"
#include "sfu_def.h"
#include "sfu_new_image.h"


/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_LOADER SFU Loader
  * @{
  */

/** @defgroup SFU_LOADER_Exported_Types Exported Types
  * @{
  */

/**
  * @brief  SFU_LOADER Status Type Definition
  */
typedef enum
{
  SFU_LOADER_OK                            = 0x00U,
  SFU_LOADER_ERR                           = 0x01U,
  SFU_LOADER_ERR_COM                       = 0x02U,
  SFU_LOADER_ERR_CMD_AUTH_FAILED           = 0x03U,
  SFU_LOADER_ERR_FW_LENGTH                 = 0x04U,
  SFU_LOADER_ERR_OLD_FW_VERSION            = 0x05U,
  SFU_LOADER_ERR_DOWNLOAD                  = 0x06U,
  SFU_LOADER_ERR_FLASH_ACCESS              = 0x07U,
  SFU_LOADER_ERR_CRYPTO                    = 0x08U
} SFU_LOADER_StatusTypeDef;
/**
  * @}
  */


/** @addtogroup SFU_LOADER_Exported_Functions
  * @{
  */

/** @addtogroup SFU_LOADER_Initialization_Functions
  * @{
  */

SFU_ErrorStatus SFU_LOADER_Init(void);
SFU_ErrorStatus SFU_LOADER_DeInit(void);

/**
  * @}
  */

/** @addtogroup SFU_LOADER_Control_Functions
  * @{
  */

SFU_ErrorStatus SFU_LOADER_DownloadNewUserFw(SFU_LOADER_StatusTypeDef *peSFU_LOADER_Status,
                                             SFU_FwImageFlashTypeDef *p_FwImageFlashData, uint32_t *puSize);

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

#endif /* SFU_LOADER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

