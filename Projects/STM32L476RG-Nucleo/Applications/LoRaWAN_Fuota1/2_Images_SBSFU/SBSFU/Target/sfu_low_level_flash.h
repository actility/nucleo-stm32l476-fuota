/**
  ******************************************************************************
  * @file    sfu_low_level_flash.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for Secure Firmware Update flash
  *          low level interface.
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
#ifndef SFU_LOW_LEVEL_FLASH_H
#define SFU_LOW_LEVEL_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "sfu_def.h"

/** @addtogroup SFU Secure Secure Boot / Firmware Update
  * @{
  */

/** @addtogroup SFU_LOW_LEVEL
  * @{
  */

/** @defgroup SFU_LOW_LEVEL_FLASH Flash Low Level Interface
  * @{
  */

/** @defgroup SFU_FLASH_Exported_Constants Exported Constants
  * @{
  */

/**
  * @brief Flash Write Access Constraints (size and alignment)
  *
  * For instance, on L4, it is only possible to program double word (2 x 32-bit data).
  * See http://www.st.com/content/ccc/resource/technical/document/reference_manual/02/35/09/0c/4f/f7/40/03/DM00083560.pdf/files/DM00083560.pdf/jcr:content/translations/en.DM00083560.pdf
  *
  * @note This type is very important for the FWIMG module (see @ref SFU_IMG).
  * \li This is the type to be used for an atomic write in FLASH: see @ref AtomicWrite.
  * \li The size of this type changes the size of the TRAILER area at the end of slot #1,
  *     as it is used to tag if a Firmware Image chunk has been swapped or not (see @ref SFU_IMG_CheckTrailerValid).
  */

/* double-word is the default setting for most platforms */
typedef uint64_t SFU_LL_FLASH_write_t;

#if defined(SFU_FWIMG_CORE_C)
/*
 * Only sfu_fwimg_core.c should need this.
 * These elements are declared here because they are FLASH dependent so HW dependent.
 * Otherwise they would be internal items of sfu_fwimg_core.c
 */

/* Trailer pattern : sizeof of write access type */
const int8_t SWAPPED[sizeof(SFU_LL_FLASH_write_t)]  = {0, 0, 0, 0, 0, 0, 0, 0};
const int8_t NOT_SWAPPED[sizeof(SFU_LL_FLASH_write_t)] = {-1, -1, -1, -1, -1, -1, -1, -1};

/**
  * Length of a MAGIC tag (32 bytes).
  * This must be a multiple of @ref SFU_LL_FLASH_write_t with a minimum value of 32.
  */
#define MAGIC_LENGTH ((uint32_t)32U)

/**
  * Specific tag definition
  * This is used to erase the MAGIC patterns.
  */
const uint8_t MAGIC_NULL[MAGIC_LENGTH]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0
                                          };
#endif /* SFU_FWIMG_CORE_C */

/**
  * @brief  SFU_FLASH_IF Status definition
  */
typedef enum
{
  SFU_FLASH_ERROR = 0U,       /*!< Error Flash generic*/
  SFU_FLASH_ERR_HAL,          /*!< Error Flash HAL init */
  SFU_FLASH_ERR_ERASE,        /*!< Error Flash erase */
  SFU_FLASH_ERR_WRITING,      /*!< Error writing data in Flash */
  SFU_FLASH_ERR_WRITINGCTRL,  /*!< Error checking data written in Flash */
  SFU_FLASH_SUCCESS           /*!< Flash Success */
} SFU_FLASH_StatusTypeDef;

/**
  * @}
  */


/** @defgroup SFU_FLASH_Exported_Macros Exported Macros
  * @{
  */

/**
  * @brief Macro to make sure the FWIMG slots are properly aligned with regards to flash constraints
  * Each slot shoud start at the begining of a page/sector
  */
#define IS_ALIGNED(address) (0U == ((address) % FLASH_PAGE_SIZE))

/**
  * @}
  */

/** @defgroup SFU_FLASH_Exported_Functions Exported Functions
  * @{
  */
SFU_ErrorStatus SFU_LL_FLASH_Erase_Size(SFU_FLASH_StatusTypeDef *pxFlashStatus, void *pStart, uint32_t Length);
SFU_ErrorStatus SFU_LL_FLASH_Write(SFU_FLASH_StatusTypeDef *pxFlashStatus, void *pDestination, const void *pSource,
                                   uint32_t Length);
SFU_ErrorStatus SFU_LL_FLASH_Read(void *pDestination, const void *pSource, uint32_t Length);
SFU_ErrorStatus SFU_LL_FLASH_CleanUp(SFU_FLASH_StatusTypeDef *pFlashStatus, void *pStart, uint32_t Length);
void NMI_Handler(void);
uint32_t SFU_LL_FLASH_GetPage(uint32_t Addr);
uint32_t SFU_LL_FLASH_GetBank(uint32_t Addr);


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

#endif /* SFU_LOW_LEVEL_FLASH_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
