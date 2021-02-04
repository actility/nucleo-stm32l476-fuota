/**
  ******************************************************************************
  * @file    sfu_loader.c
  * @author  MCD Application Team
  * @brief   Secure Firmware Update LOADER module.
  *          This file provides set of firmware functions to manage SFU local
  *          loader functionalities.
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
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "sfu_loader.h"
#include "sfu_low_level_flash.h"
#include "sfu_low_level_security.h"
#include "sfu_com_loader.h"
#include "sfu_trace.h"
#include "se_interface_bootloader.h" /* for metadata authentication */
#include "sfu_fwimg_services.h"      /* for version checking & to check if a valid FW is installed (the local 
                                        bootloader is a kind of "application" running in SB_SFU) */
#include "app_sfu.h"

#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup SFU_LOADER SFU Loader
  * @brief Local loader: this file provides the functions to manage the checking and downloading of the SFU Application.
  * @{
  */



/** @defgroup SFU_LOADER_Private_Variables Private Variables
  * @{
  */
static uint32_t m_uDwlAreaAddress = 0U;                          /*!< Address of to write in download area */
static uint32_t m_uDwlAreaStart = 0U;                            /*!< Address of download area */
static uint32_t m_uDwlAreaSize = 0U;                             /*!< Size of download area */
static uint32_t m_uFileSizeYmodem = 0U;                          /*!< Ymodem file size being received */
static uint32_t m_uNbrBlocksYmodem = 0U;                         /*!< Number of blocks being received via Ymodem*/
static uint32_t m_uPacketsReceived = 0U;                         /*!< Number of packets received via Ymodem*/
/**
  * @}
  */

/** @defgroup SFU_LOADER_Private_Functions Private Functions
  * @{
  */

/**
  * @}
  */

/** @defgroup SFU_LOADER_Exported_Functions Exported Functions
  * @{
  */

/** @defgroup SFU_LOADER_Initialization_Functions Initialization Functions
  * @{
  */

/**
  * @brief  Initialize the SFU LOADER.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LOADER_Init(void)
{
  /*
  * ADD SRC CODE HERE
  */

  /*
   * Sanity check to make sure that the local loader cannot read out of the buffer bounds
   * when doing a length alignment before writing in FLASH.
   */
  /* m_aPacketData contains SFU_COM_YMODEM_PACKET_1K_SIZE bytes of payload */
  if (0 != (uint32_t)(SFU_COM_YMODEM_PACKET_1K_SIZE % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    /* The packet buffer (payload part) must be a multiple of the FLASH write length  */
    TRACE("\r\n= [FWIMG] Packet Payload size (%d) is not matching the FLASH constraints",
          SFU_COM_YMODEM_PACKET_1K_SIZE);
    return SFU_ERROR;
  } /* else the FW Header Length is fine with regards to FLASH constraints */

  return SFU_SUCCESS;
}

/**
  * @brief  DeInitialize the SFU LOADER.
  * @param  None
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LOADER_DeInit(void)
{
  /*
   * ADD SRC CODE HERE
   */
  return SFU_SUCCESS;
}

/**
  * @}
  */

/** @defgroup SFU_LOADER_Control_Functions Control Functions
  * @{
  */

/**
  * @brief  Download a new User Fw.
  *         Writes firmware received via Ymodem in FLASH.
  * @param  peSFU_LOADER_Status: SFU LOADER Status.
  *         This parameter can be a value of @ref SFU_LOADER_Status_Structure_definition.
  * @param  p_FwImageFlashData: Firmware image flash data.
  * @param  puSize: Size of the downloaded image.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_LOADER_DownloadNewUserFw(SFU_LOADER_StatusTypeDef *peSFU_LOADER_Status,
                                             SFU_FwImageFlashTypeDef *p_FwImageFlashData, uint32_t *puSize)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  SFU_COM_YMODEM_StatusTypeDef e_com_status = SFU_COM_YMODEM_ERROR;

  /* Check the pointers allocation */
  if ((peSFU_LOADER_Status == NULL) || (puSize == NULL) || p_FwImageFlashData == NULL)
  {
    return SFU_ERROR;
  }

  /* Set it to OK by default then log the error in case of issue */
  *peSFU_LOADER_Status = SFU_LOADER_OK;

  /* Refresh Watchdog */
  SFU_LL_SECU_IWDG_Refresh();

  /* Transfert FW Image via YMODEM protocol */
  TRACE("\r\n\t  File> Transfer> YMODEM> Send ");

  /* Assign the download flash address to be used during the YMODEM process */
  m_uDwlAreaStart =  p_FwImageFlashData->DownloadAddr;
  m_uDwlAreaSize =  p_FwImageFlashData->MaxSizeInBytes;

  /* Receive the FW in RAM and write it in the Flash*/
  if (SFU_COM_YMODEM_Receive(&e_com_status, puSize) != SFU_SUCCESS)
  {
    /*Download did not complete successfully*/
    *peSFU_LOADER_Status = SFU_LOADER_ERR_COM;
    e_ret_status = SFU_ERROR;
  }
  else
  {
    if (*puSize <= 0U)
    {
      /*Download file size is not correct*/
      *peSFU_LOADER_Status = SFU_LOADER_ERR_DOWNLOAD;
      e_ret_status = SFU_ERROR;
    }
  }


  return e_ret_status;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup SFU_LOADER_Private_Functions
  * @{
  */

/**
  * @brief  Verifies the Raw Fw Header received. It checks if the header is
  *         authentic and if the fields are ok with the device (e.g. size and version).
  * @note   Please note that when the new image is installed, this metadata is checked
  *         by @ref SFU_IMG_CheckCandidateMetadata.
  * @param  peSFU_LOADER_Status: SFU LOADER Status.
  *         This parameter can be a value of @ref SFU_LOADER_Status_Structure_definition.
  * @param  pBuffer: pointer to header Buffer.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus SFU_LOADER_VerifyFwHeader(SFU_LOADER_StatusTypeDef *peSFU_LOADER_Status, uint8_t *pBuffer)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef              e_se_status;
  uint8_t                       u_is_header_valid = 0U;
  SE_FwRawHeaderTypeDef         *p_x_fw_raw_header;

  *peSFU_LOADER_Status = SFU_LOADER_ERR_CMD_AUTH_FAILED;

  /* Check the pointers allocation */
  if ((peSFU_LOADER_Status == NULL) || (pBuffer == NULL))
  {
    return SFU_ERROR;
  }

  /*Parse the received buffer*/
  p_x_fw_raw_header = (SE_FwRawHeaderTypeDef *)pBuffer;

  /*Check if the received header packet is authentic*/
  if (SE_VerifyFwRawHeaderTag(&e_se_status, p_x_fw_raw_header) != SE_ERROR)
  {
    /*
     * ##1 - Check version :
     *      It is not allowed to install a Firmware with a lower version than the active firmware.
     *      But we authorize the re-installation of the current firmware version.
     *      We also check (in case there is no active FW) that the candidate version is at least the min. allowed
     *      version for this device.
     *
     *      SFU_IMG_GetActiveFwVersion() returns -1 if there is no active firmware.
     */
    int32_t curVer = SFU_IMG_GetActiveFwVersion();

    /* if no valid firmware is installed then checking that the new version is greater than or equal to the current
       version is meaningless */
    /* candidate >= current version && candidate >= min.version number  */
    if ((((int32_t)p_x_fw_raw_header->FwVersion) >= curVer) && (p_x_fw_raw_header->FwVersion >=
                                                                SFU_FW_VERSION_START_NUM))
    {
      /* The installation is authorized */
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n          Anti-rollback: candidate version(%d) accepted | current version(%d) , min.version(%d) !",
            p_x_fw_raw_header->FwVersion, curVer, SFU_FW_VERSION_START_NUM);
#endif /* SFU_VERBOSE_DEBUG_MODE */
      e_ret_status = SFU_SUCCESS;
    }
    else
    {
      /* The installation is forbidden */
      TRACE("\r\n          Anti-rollback: candidate version(%d) rejected | current version(%d) , min.version(%d) !",
            p_x_fw_raw_header->FwVersion, curVer, SFU_FW_VERSION_START_NUM);
      *peSFU_LOADER_Status = SFU_LOADER_ERR_OLD_FW_VERSION;
      e_ret_status = SFU_ERROR;
    }

    if (SFU_SUCCESS == e_ret_status)
    {
      /*
       * ##2 - Check length :
       *      We do not check the length versus the trailer constraint because this check is already implemented in
       *      SFU_IMG_FirmwareToInstall().
       *      If the firmware is too big to have some room left for the trailer info then the installation will not be
       *      triggered.
       *      The interest is to avoid duplicating the checks (checking just in time).
       *      This is also because in the case of a download handled by the UserApp we cannot rely on the checks done
       *      by the UserApp before installing a FW.
       *      The drawback is that the firmware can be downloaded in slot #1 though it is too big.
       *
       *      Nevertheless, we can still detect if the FW is too big to be downloaded (cannot be written in slot #1).
       *      This will avoid download issues (but the installation itself can still be rejected) or overflows.
       *      The slot #1 must contain the HEADER and also the binary FW (encrypted).
       *      But there is an offset of FW_OFFSET_IMAGE bytes to respect.
       */
      if ((p_x_fw_raw_header->PartialFwSize + (p_x_fw_raw_header->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE)) >
          (SFU_IMG_SLOT_DWL_REGION_SIZE - SFU_IMG_IMAGE_OFFSET))
      {
        /* The firmware cannot be written in slot #1 */
        *peSFU_LOADER_Status = SFU_LOADER_ERR_FW_LENGTH;
      }
      else
      {
        /* The firmware can be written in slot #1 (but this does not mean it can be installed, this will be checked at
           the SFU_IMG_FirmwareToInstall() stage.) */
        e_ret_status = SFU_SUCCESS;
        u_is_header_valid = 1U;
      }
    } /* check length end */

    /* Overall validation: authenticity and fields validity are considered*/
    if ((e_ret_status == SFU_SUCCESS) && (u_is_header_valid == 1U))
    {
      *peSFU_LOADER_Status = SFU_LOADER_OK;
    }
    else
    {
      e_ret_status = SFU_ERROR;
    }
  }

  return e_ret_status;
}

/**
  * @}
  */

/** @defgroup SFU_LOADER_Callback_Functions Callback Functions
  * @{
  */

/**
  * @brief  Ymodem Header Packet Transfer completed callback.
  * @param  uFileSize: Dimension of the file that will be received.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_YMODEM_HeaderPktRxCpltCallback(uint32_t uFileSize)
{
  /*Reset of the ymodem variables */
  m_uFileSizeYmodem = 0U;
  m_uPacketsReceived = 0U;
  m_uNbrBlocksYmodem = 0U;

  /*Filesize information is stored*/
  m_uFileSizeYmodem = uFileSize;

  /*Compute the number of 1K blocks */
  m_uNbrBlocksYmodem = (m_uFileSizeYmodem + (SFU_COM_YMODEM_PACKET_1K_SIZE - 1U)) / SFU_COM_YMODEM_PACKET_1K_SIZE;

  /* NOTE : delay inserted for Ymodem protocol*/
  HAL_Delay(1000U);

  return SFU_SUCCESS;
}

/**
  * @brief  Ymodem Data Packet Transfer completed callback.
  * @param  pData: Pointer to the buffer.
  * @param  uSize: Packet dimension.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_YMODEM_DataPktRxCpltCallback(uint8_t *pData, uint32_t uSize)
{
  /* The local loader must make a copy of the Firmware metadata,
   * because this memory area is not copied when calling the SE_Decrypt_Init() primitive.
   * Hence we must make sure this memory area still contains the FW header when SE_Decrypt_Finish() is called. */
  static uint8_t fw_header[SE_FW_HEADER_TOT_LEN] __attribute__((aligned(4)));
  /* Size of downloaded Image initialized with first packet (header) and checked along download process */
  static uint32_t m_uDwlImgSize = 0U;

  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  SFU_FLASH_StatusTypeDef x_flash_info;
  SFU_LOADER_StatusTypeDef e_SFU_LOADER_Status;
  uint32_t uOldSize;

  /* Check the pointers allocation */
  if (pData == NULL)
  {
    return SFU_ERROR;
  }

  /*Increase the number of received packets*/
  m_uPacketsReceived++;

  /* Last packet : size of data to write could be different than SFU_COM_YMODEM_PACKET_1K_SIZE */
  if (m_uPacketsReceived == m_uNbrBlocksYmodem)
  {
    /*Extracting actual payload from last packet*/
    if (0 == (m_uFileSizeYmodem % SFU_COM_YMODEM_PACKET_1K_SIZE))
    {
      /* The last packet must be fully considered */
      uSize = SFU_COM_YMODEM_PACKET_1K_SIZE;
    }
    else
    {
      /* The last packet is not full, drop the extra bytes */
      uSize = m_uFileSizeYmodem - ((uint32_t)(m_uFileSizeYmodem / SFU_COM_YMODEM_PACKET_1K_SIZE) *
                                   SFU_COM_YMODEM_PACKET_1K_SIZE);
    }
  }

  /* First packet : Contains the FW header (SE_FW_HEADER_TOT_LEN bytes length) which is not encrypted  */
  if (m_uPacketsReceived == 1)
  {

    m_uDwlAreaAddress = m_uDwlAreaStart;
    memcpy(fw_header, pData, SE_FW_HEADER_TOT_LEN);

    /* Verify header */
    e_ret_status = SFU_LOADER_VerifyFwHeader(&e_SFU_LOADER_Status, pData);
    if (e_ret_status == SFU_SUCCESS)
    {
      if (e_SFU_LOADER_Status != SFU_LOADER_OK)
      {
        e_ret_status = SFU_ERROR;
      }
      else
      {
        /* Downloaded Image size : Header size + gap for image alignment to (UpdateFwOffset % sector size) +
           Partial Image size */
        m_uDwlImgSize = ((SE_FwRawHeaderTypeDef *)fw_header)->PartialFwSize +
                        (((SE_FwRawHeaderTypeDef *)fw_header)->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE) +
                        SFU_IMG_IMAGE_OFFSET;
      }
    }

    /* Clear Download application area (including TRAILERS area) */
    if ((e_ret_status == SFU_SUCCESS)
        && (SFU_LL_FLASH_Erase_Size(&x_flash_info, (void *) m_uDwlAreaAddress, SFU_IMG_SLOT_DWL_REGION_SIZE) !=
            SFU_SUCCESS))
    {
      e_ret_status = SFU_ERROR;
    }

    if (e_ret_status == SFU_SUCCESS)
    {
      /* Write the FW header (SE_FW_HEADER_TOT_LEN bytes length) at start of DWL area */
      if (SFU_LL_FLASH_Write(&x_flash_info, (void *)m_uDwlAreaAddress, pData, SE_FW_HEADER_TOT_LEN) == SFU_SUCCESS)
      {
        /* Shift the DWL area pointer, to align image with (PartialFwOffset % sector size) in DWL area */
        m_uDwlAreaAddress += SFU_IMG_IMAGE_OFFSET +
                             ((SE_FwRawHeaderTypeDef *)fw_header)->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE;

        /* Update remaining packet size to write */
        uSize -= SE_FW_HEADER_TOT_LEN;

        /* Update pData pointer to received packet data */
        pData += SE_FW_HEADER_TOT_LEN;
      }
      else
      {
        e_ret_status = SFU_ERROR;
      }
    }
  }

  /* Check size to avoid writing beyond DWL image size */
  if ((m_uDwlAreaAddress + uSize) > (m_uDwlAreaStart + m_uDwlImgSize))
  {
    e_ret_status = SFU_ERROR;
  }

  /* Set dimension to the appropriate length for FLASH programming.
   * Example: 64-bit length for L4.
   */
  if ((uSize % (uint32_t)sizeof(SFU_LL_FLASH_write_t)) != 0)
  {
    /* By construction, the length of the buffer (fw_decrypted_chunk or pData) must be a multiple of
       sizeof(SFU_IMG_write_t) to avoid reading out of the buffer */
    uOldSize = uSize;
    uSize = uSize + ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - (uSize % (uint32_t)sizeof(SFU_LL_FLASH_write_t)));
    while (uOldSize < uSize)
    {
      pData[uOldSize] = 0xFF;
      uOldSize++;
    }
  }

  /* Check size to avoid writing beyond DWL area */
  if ((m_uDwlAreaAddress + uSize) > (m_uDwlAreaStart + m_uDwlAreaSize))
  {
    e_ret_status = SFU_ERROR;
  }

  /* Write Data in Flash */
  if (e_ret_status == SFU_SUCCESS)
  {
    if (SFU_LL_FLASH_Write(&x_flash_info, (void *)m_uDwlAreaAddress, pData, uSize) == SFU_SUCCESS)
    {
      m_uDwlAreaAddress += (uSize);
    }
    else
    {
      e_ret_status = SFU_ERROR;
    }
  }


  /* Last packet : reset m_uPacketsReceived */
  if (m_uPacketsReceived == m_uNbrBlocksYmodem)
  {
    m_uPacketsReceived = 0U;
  }

  /* Reset data counters in case of error */
  if (e_ret_status == SFU_ERROR)
  {
    /*Reset of the ymodem variables */
    m_uFileSizeYmodem = 0U;
    m_uPacketsReceived = 0U;
    m_uNbrBlocksYmodem = 0U;
  }

  return e_ret_status;
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

#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
