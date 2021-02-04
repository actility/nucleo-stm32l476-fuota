/**
  ******************************************************************************
  * @file    sfu_app_new_image.c
  * @author  MCD Application Team
  * @brief   This file provides set of firmware functions to manage the New Firmware
  *          Image storage and installation.
  *          This file contains the services the user application can use to
  *          know where to store a new FW image and request its installation.
  *          The same services are offered to the local loader thanks to a similar
  *          file integrated in SB_SFU.
  * @note    This file is compiled in the scope of the User Application.
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

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "flash_if.h"
#include "sfu_app_new_image.h"
#include "sfu_fwimg_regions.h"
#include "se_def_metadata.h"

/** @addtogroup USER_APP User App Example
  * @{
  */

/** @addtogroup  FW_UPDATE Firmware Update Example
  * @{
  */


/** @defgroup  SFU_APP_NEWIMG New Firmware Image storage and installation
  * @brief Code used to handle the storage and installation of a new Firmware Image.
  * @{
  */

/** @defgroup SFU_APP_NEWIMG_Private_Functions Private Functions
  * @{
  */

/**
  * @brief  Write the header of the firmware to install
  * @param  pfw_header pointer to header to write.
  * @retval HAL_OK on success otherwise HAL_ERROR
  */
static uint32_t WriteInstallHeader(uint8_t *pfw_header)
{
  uint32_t ret = HAL_OK;

  ret = FLASH_If_Erase_Size((void *) SFU_IMG_SWAP_REGION_BEGIN_VALUE, SFU_IMG_IMAGE_OFFSET);
  if (ret == HAL_OK)
  {
    ret = FLASH_If_Write((void *)SFU_IMG_SWAP_REGION_BEGIN_VALUE, pfw_header, SE_FW_HEADER_TOT_LEN);
  }
  return ret;
}


/**
  * @}
  */

/** @defgroup SFU_APP_NEWIMG_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  Write in Flash the next header image to install.
  *         This function is used by the User Application to request a Firmware installation (at next reboot).
  * @param  fw_header FW header of the FW to be installed
  * @retval HAL_OK if successful, otherwise HAL_ERROR
  */
uint32_t SFU_APP_InstallAtNextReset(uint8_t *fw_header)
{
  if (fw_header == NULL)
  {
    return HAL_ERROR;
  }
  if (WriteInstallHeader(fw_header) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}


/**
  * @brief  Provide the area descriptor to write a FW image in Flash.
  *         This function is used by the User Application to know where to store a new Firmware Image before asking for
  *         its installation.
  * @param  pArea pointer to area descriptor
  * @retval HAL_OK if successful, otherwise HAL_ERROR
  */
uint32_t SFU_APP_GetDownloadAreaInfo(SFU_FwImageFlashTypeDef *pArea)
{
  uint32_t ret;
  if (pArea != NULL)
  {
    pArea->DownloadAddr = SFU_IMG_SLOT_DWL_REGION_BEGIN_VALUE;
    pArea->MaxSizeInBytes = (uint32_t)SFU_IMG_SLOT_DWL_REGION_SIZE;
    pArea->ImageOffsetInBytes = SFU_IMG_IMAGE_OFFSET;
    ret =  HAL_OK;
  }
  else
  {
    ret = HAL_ERROR;
  }
  return ret;
}

uint32_t SFU_APP_GetActiveAreaInfo(SFU_FwImageFlashTypeDef *pArea)
{
  uint32_t ret;
  if (pArea != NULL)
  {
    pArea->DownloadAddr = SFU_IMG_SLOT_0_REGION_BEGIN_VALUE;
    pArea->MaxSizeInBytes = (uint32_t)SFU_IMG_SLOT_0_REGION_SIZE;
    pArea->ImageOffsetInBytes = SFU_IMG_IMAGE_OFFSET;
    ret =  HAL_OK;
  }
  else
  {
    ret = HAL_ERROR;
  }
  return ret;
}

uint32_t SFU_APP_GetSwapAreaInfo(SFU_FwImageFlashTypeDef *pArea)
{
  uint32_t ret;
  if (pArea != NULL)
  {
    pArea->DownloadAddr = SFU_IMG_SWAP_REGION_BEGIN_VALUE;
    pArea->MaxSizeInBytes = (uint32_t)SFU_IMG_SWAP_REGION_SIZE;
    pArea->ImageOffsetInBytes = 0;
    ret =  HAL_OK;
  }
  else
  {
    ret = HAL_ERROR;
  }
  return ret;
}

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


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
