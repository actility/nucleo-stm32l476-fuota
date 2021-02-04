/**
  ******************************************************************************
  * @file    FwUpdateAgent.c
  * @author  MCD Application Team
  * @brief   Firmware Update module.
  *          This file provides set of firmware functions to manage Firmware
  *          Update functionalities.
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
#include "stm32l4xx_hal.h"
#include "stm32l4xx_nucleo.h"
#include "FwUpdateAgent.h"
#include "FlashMemHandler.h"

#if defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"         /* to access to the definition of REGION_SLOT_1_START*/
#elif defined(__CC_ARM)
#include "mapping_fwimg.h"
#endif

#include "se_crypto_config.h"        /*@@@ these files have been included to get back the rawheader definition@ */
#include "se_def_metadata.h"        /* the struct defintion is dependent of the crypto scheme used*/


#define SFU_IMG_SWAP_REGION_SIZE ((uint32_t)(REGION_SWAP_END - REGION_SWAP_START + 1U))
#define SFU_IMG_SWAP_REGION_BEGIN_VALUE  ((uint32_t)REGION_SWAP_START)

/*starting offset to add to the  first address */
#define SFU_IMG_IMAGE_OFFSET ((uint32_t)512U)

/*size of header to write in Swap sector to trigger installation*/
#define INSTALLED_LENGTH  ((uint32_t)512U)

#define SFU_IMG_SLOT_DWL_REGION_BEGIN_VALUE ((uint32_t)REGION_SLOT_1_START)
#define SFU_IMG_SLOT_DWL_REGION_SIZE        ((uint32_t)(REGION_SLOT_1_END - REGION_SLOT_1_START + 1U))

#include "string.h"

#include "util_console.h"

/* Private function prototypes -----------------------------------------------*/


/* Private functions ---------------------------------------------------------*/
static uint32_t WriteInstallHeader(uint8_t *pfw_header);
static uint32_t FwUpdateAgentInstallAtNextReset(uint8_t *fw_header);
static uint32_t FwUpdateAgentGetDownloadAreaInfo(FwImageFlashTypeDef *pArea);

/**
  * @brief  Run FW Update process.
  * @param  None
  * @retval HAL Status.
  */
void FwUpdateAgentRun(void)
{

  HAL_StatusTypeDef ret = HAL_ERROR;
  uint8_t  fw_header_input[SE_FW_HEADER_TOT_LEN];
  FwImageFlashTypeDef fw_image_dwl_area;


  /* Get Info about the download area */
  if (FwUpdateAgentGetDownloadAreaInfo(&fw_image_dwl_area) != HAL_ERROR)
  {
    /* Read header in slot 1 */
    memcpy((void *) fw_header_input, (void *) fw_image_dwl_area.DownloadAddr, sizeof(fw_header_input));

    /* Ask for installation at next reset */
    (void)FwUpdateAgentInstallAtNextReset((uint8_t *) fw_header_input);

    /* System Reboot*/
    PRINTF("  -- Image correctly downloaded - reboot\r\n\n");
    HAL_Delay(1000U);
    NVIC_SystemReset();
  }
  if (ret != HAL_OK)
  {
    PRINTF("  --  Operation Failed  \r\n");
  }
}

/**
  * @brief  Data file Transfer from Ram to Flash
  * @param  pData Pointer to the buffer.
  * @param  uSize file dimension (Bytes).
  * @retval None
  */
HAL_StatusTypeDef FwUpdateAgentDataTransferFromRamToFlash(uint8_t *pData, uint32_t uFlashDestination, uint32_t uSize)
{
  /* End address of downloaded Image : initialized with first packet (header) and checked along download process */
  /* static uint32_t m_uDwlImgEnd = 0U; */

  /* Current destination address for data packet : initialized with first packet (header), incremented at each flash write */
  static uint32_t m_uDwlImgCurrent = 0U;

  HAL_StatusTypeDef e_ret_status = HAL_OK;

  /* Initialize Current destination address for data packet */
  m_uDwlImgCurrent = uFlashDestination;

  /* End of Image to be downloaded */
  /* m_uDwlImgEnd = uFlashDestination + ((SE_FwRawHeaderTypeDef *)pData)->PartialFwSize
                                   + (((SE_FwRawHeaderTypeDef *)pData)->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE)
                                   + SFU_IMG_IMAGE_OFFSET; */

  /* Clear Download area*/
  if (FlashMemHandlerFct.Erase_Size((void *)REGION_SLOT_1_START, (REGION_SLOT_1_END - REGION_SLOT_1_START)) == HAL_OK)
  {
    /* Write the FW header (SFU_IMG_IMAGE_OFFSET bytes length) at start of DWL area */
    if (FlashMemHandlerFct.Write((void *)uFlashDestination, pData, SFU_IMG_IMAGE_OFFSET) == HAL_OK)
    {
      e_ret_status = HAL_OK;

      /* Shift the DWL area pointer, to align image with (PartialFwOffset % sector size) in DWL area */
      m_uDwlImgCurrent += SFU_IMG_IMAGE_OFFSET +
                          ((SE_FwRawHeaderTypeDef *)pData)->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE;

      /* Update remaining packet size to write */
      uSize -= SFU_IMG_IMAGE_OFFSET;

      /* Update pData pointer to received packet data */
      pData += SFU_IMG_IMAGE_OFFSET;
    }
    else
    {
      e_ret_status = HAL_ERROR;
    }

    /*Adjust dimension to 64-bit length */
    if (uSize % FLASH_IF_MIN_WRITE_LEN != 0U)
    {
      uSize += (FLASH_IF_MIN_WRITE_LEN - (uSize % FLASH_IF_MIN_WRITE_LEN));
    }

    /* Write Data in Flash - size has to be 64-bit aligned */

    /* Write in flash only if not beyond allowed area */
    /* if (((m_uDwlImgCurrent + uSize) <= m_uDwlImgEnd) && (e_ret_status == HAL_OK)) */

    if (FlashMemHandlerFct.Write((void *)m_uDwlImgCurrent, pData, uSize) == HAL_OK)
    {
      e_ret_status = HAL_OK;
    }
    else
    {
      e_ret_status = HAL_ERROR;
    }

  }
  else
  {
    e_ret_status = HAL_ERROR;
  }

  return e_ret_status;
}

/************************************************************************************
  * @brief   Set of firmware functions to manage the New Firmware Image storage
  *          and installation.
  *          Services the user application can use to know where to store a
  *          new FW image and request its installation.
  *          Same services are offered to the local loader thanks to a similar
  *          file integrated in SB_SFU.
************************************************************************************/




/**
  * @brief  Provide the area descriptor to write a FW image in Flash.
  *         This function is used by the User Application to know where to store
  *          a new Firmware Image before asking for its installation.
  * @param  pArea pointer to area descriptor
  * @retval HAL_OK if successful, otherwise HAL_ERROR
  */
static uint32_t FwUpdateAgentGetDownloadAreaInfo(FwImageFlashTypeDef *pArea)
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


/**
  * @brief  Write in Flash the next header image to install.
  *         This function is used by the User Application to request a Firmware installation (at next reboot).
  * @param  fw_header FW header of the FW to be installed
  * @retval HAL_OK if successful, otherwise HAL_ERROR
  */
static uint32_t FwUpdateAgentInstallAtNextReset(uint8_t *fw_header)
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
  * @brief  Write the header of the firmware to install
  * @param  pfw_header pointer to header to write.
  * @retval HAL_OK on success otherwise HAL_ERROR
  */
static uint32_t WriteInstallHeader(uint8_t *pfw_header)
{
  uint32_t ret = HAL_OK;
  uint8_t zero_buffer[INSTALLED_LENGTH - SE_FW_HEADER_TOT_LEN];

  memset(zero_buffer, 0x00, sizeof(zero_buffer));
  ret = FlashMemHandlerFct.Erase_Size((void *) SFU_IMG_SWAP_REGION_BEGIN_VALUE, SFU_IMG_IMAGE_OFFSET);
  if (ret == HAL_OK)
  {
    ret = FlashMemHandlerFct.Write((void *)SFU_IMG_SWAP_REGION_BEGIN_VALUE, pfw_header, SE_FW_HEADER_TOT_LEN);
  }
  if (ret == HAL_OK)
  {
    ret = FlashMemHandlerFct.Write((void *)(SFU_IMG_SWAP_REGION_BEGIN_VALUE + SE_FW_HEADER_TOT_LEN), (void *)zero_buffer, sizeof(zero_buffer));
  }
  return ret;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
