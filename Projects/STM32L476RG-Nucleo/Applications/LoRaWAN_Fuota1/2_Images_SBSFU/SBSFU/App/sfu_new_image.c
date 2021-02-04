/**
  ******************************************************************************
  * @file    sfu_new_image.c
  * @author  MCD Application Team
  * @brief   This file provides set of firmware functions to manage the New Firmware Image storage and installation.
  *          This file contains the services the bootloader can use to
  *          know where to store a new FW image and request its installation.
  *          The same services are offered to the user application thanks to a similar file integrated in the user
  *          application.
  * @note    This file is compiled in the scope of SB_SFU.
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
#include "main.h"
#include "sfu_def.h"
#include "sfu_low_level_flash.h"
#include "sfu_new_image.h"
#include "sfu_fwimg_services.h"
#include "sfu_fwimg_regions.h"
#include "se_def_metadata.h"
#include <string.h> /* needed for memset (see WriteInstallHeader)*/

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


/** @defgroup  SFU_IMG_NEWIMG New Firmware Image storage and installation
  * @brief Code used to handle the storage and installation of a new Firmware Image.
  * @{
  */

/** @defgroup SFU_IMG_NEWIMG_Private_Functions Private Functions
  * @{
  */

/**
  * @brief  Write the header of the firmware to install
  * @param  pfw_header pointer to header to write.
  * @retval SFU_SUCCESS on success otherwise SFU_ERROR
  */
static SFU_ErrorStatus WriteInstallHeader(uint8_t *pfw_header)
{
  SFU_ErrorStatus ret = SFU_SUCCESS;
  SFU_FLASH_StatusTypeDef flash_if_info;

  ret = SFU_LL_FLASH_Erase_Size(&flash_if_info, (void *) SFU_IMG_SWAP_REGION_BEGIN_VALUE, SFU_IMG_IMAGE_OFFSET);
  if (ret == SFU_SUCCESS)
  {
    ret = SFU_LL_FLASH_Write(&flash_if_info, (void *)SFU_IMG_SWAP_REGION_BEGIN_VALUE, pfw_header, SE_FW_HEADER_TOT_LEN);
  }
  return ret;
}


/**
  * @}
  */

/** @defgroup SFU_IMG_NEWIMG_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  Write in Flash the next header image to install.
  *         This function is used by the local loader to request a Firmware installation (at next reboot).
  * @param  fw_header FW header of the FW to be installed
  * @retval SFU_SUCCESS if successful, otherwise SFU_ERROR
  */
SFU_ErrorStatus SFU_IMG_InstallAtNextReset(uint8_t *fw_header)
{
  if (fw_header == NULL)
  {
    return SFU_ERROR;
  }
  if (WriteInstallHeader(fw_header) != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
  return SFU_SUCCESS;
}


/**
  * @brief  Provide the area descriptor to write a FW image in Flash.
  *         This function is used by the local loader to know where to store a new Firmware Image before asking for its
  *         installation.
  * @param  pArea pointer to area descriptor
  * @retval SFU_SUCCESS if successful, otherwise SFU_ERROR
  */
SFU_ErrorStatus SFU_IMG_GetDownloadAreaInfo(SFU_FwImageFlashTypeDef *pArea)
{
  SFU_ErrorStatus ret;
  if (pArea != NULL)
  {
    pArea->DownloadAddr = SFU_IMG_SLOT_DWL_REGION_BEGIN_VALUE;
    pArea->MaxSizeInBytes = (uint32_t)SFU_IMG_SLOT_DWL_REGION_SIZE - SFU_IMG_GetTrailerSize();
    pArea->ImageOffsetInBytes = SFU_IMG_IMAGE_OFFSET;
    ret =  SFU_SUCCESS;
  }
  else
  {
    ret = SFU_ERROR;
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
