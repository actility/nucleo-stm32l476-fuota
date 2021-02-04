/**
  ******************************************************************************
  * @file    sfu_fwimg_core.c
  * @author  MCD Application Team
  * @brief   This file provides set of firmware functions to manage the Firmware Images.
  *          This file contains the "core" functionalities of the image handling.
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

#define SFU_FWIMG_CORE_C

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "sfu_fsm_states.h" /* needed for sfu_error.h */
#include "sfu_error.h"
#include "sfu_low_level_flash.h"
#include "sfu_low_level_security.h"
#include "se_interface_bootloader.h"
#include "sfu_fwimg_regions.h"
#include "sfu_fwimg_services.h" /* to have definitions like SFU_IMG_InitStatusTypeDef
                                   (required by sfu_fwimg_internal.h) */
#include "sfu_fwimg_internal.h"
#include "sfu_trace.h"
#include "sfu_boot.h"


/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup SFU_IMG SFU Firmware Image
  * @brief Firmware Images Handling  (FWIMG)
  * @{
  */

/** @defgroup SFU_IMG_CORE SFU Firmware Image Core
  * @brief Core functionalities of the FW image handling (trailers, magic, swap...).
  * @{
  */


/** @defgroup SFU_IMG_CORE_Private_Defines Private Defines
  * @brief FWIMG Defines used only by sfu_fwimg_core
  * @{
  */

/**
  * @brief RAM chunk used for decryption / comparison / swap
  * it is the size of RAM buffer allocated in stack and used for decrypting/moving images.
  * some function allocates 2 buffer of this size in stack.
  * As image are encrypted by 128 bits blocks, this value is 16 bytes aligned.
  * it can be equal to SWAP_SIZE , or SWAP_SIZE = N  * CHUNK_SIZE
  */
#define VALID_SIZE (3*MAGIC_LENGTH)
#define CHUNK_SIZE_SIGN_VERIFICATION (1024U)  /*!< Signature verification chunk size*/
#define SFU_IMG_CHUNK_SIZE  (512U)

/**
  * @brief  FW slot and swap offset
  */
#define TRAILER_INDEX  ((uint32_t)(SFU_IMG_SLOT_0_REGION_SIZE) / (uint32_t)(SFU_IMG_SWAP_REGION_SIZE))
/*  position of image begin on 1st block */
#define TRAILER_HEADER  ( FW_INFO_TOT_LEN+ FW_INFO_TOT_LEN + MAGIC_LENGTH  )
#define COUNTER_HEADER  TRAILER_HEADER
/*                            SLOT1_REGION_SIZE                                                                    */
/*  <----------------------------------------------------------------------------------------------------------->  */
/*  |..|FW_INFO_TOT_LEN|FW_INFO_TOT_LEN|MAGIC_LENGTH|N*sizeof(SFU_LL_FLASH_write_t)|N*sizeof(SFU_LL_FLASH_write_t) */
/*  |..| header 1      | header 2      |SWAP magic  | N*CPY_TO_SLOT0               | N*CPY_TO_SLOT1                */
/*                                                                                                                 */
/* Please note that the size of the trailer area (N*CPY_TO_SLOTx) depends directly on SFU_LL_FLASH_write_t type,   */
/* so it can differ from one platform to another (this is FLASH-dependent)                                         */
#define TRAILER_SIZE (sizeof(SFU_LL_FLASH_write_t)*(TRAILER_INDEX)\
                      + sizeof(SFU_LL_FLASH_write_t)*(TRAILER_INDEX) + (uint32_t)(TRAILER_HEADER))
#define TRAILER_BEGIN  (( uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN\
                                     + (uint32_t)SFU_IMG_SLOT_1_REGION_SIZE - TRAILER_SIZE))
#define TRAILER_CPY_TO_SLOT1(i) ((void*)((uint32_t)TRAILER_BEGIN\
                                         + sizeof(SFU_LL_FLASH_write_t)*(TRAILER_INDEX) \
                                         + (uint32_t)TRAILER_HEADER+((i)*sizeof(SFU_LL_FLASH_write_t))))
#define TRAILER_CPY_TO_SLOT0(i) ((void*)((uint32_t)TRAILER_BEGIN\
                                         + (uint32_t)TRAILER_HEADER+((i)*sizeof(SFU_LL_FLASH_write_t))))
#define TRAILER_SWAP_ADDR  (( uint8_t *)((uint32_t)TRAILER_BEGIN + TRAILER_HEADER - MAGIC_LENGTH))
#define TRAILER_HDR_VALID (( uint8_t *)(TRAILER_BEGIN))
#define TRAILER_HDR_TEST  (( uint8_t *)(TRAILER_BEGIN + FW_INFO_TOT_LEN))

/*  read of this info can cause double ECC NMI , fix me , the field must be considered , only one double ECC error is
 *  possible, if several error are observed , aging issue or bug */

#define CHECK_TRAILER_MAGIC CheckMagic(TRAILER_SWAP_ADDR, TRAILER_HDR_VALID, TRAILER_HDR_TEST)

#define WRITE_TRAILER_MAGIC WriteMagic(TRAILER_SWAP_ADDR, TRAILER_HDR_VALID, TRAILER_HDR_TEST)
#define CLEAN_TRAILER_MAGIC CleanMagicValue(TRAILER_SWAP_ADDR)

#define CHUNK_1_ADDR(A,B) ((uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN\
                                       +(SFU_IMG_SWAP_REGION_SIZE*A)+SFU_IMG_CHUNK_SIZE*B))
#define CHUNK_0_ADDR(A,B) ((uint8_t *)((uint32_t)SFU_IMG_SLOT_0_REGION_BEGIN\
                                       +(SFU_IMG_SWAP_REGION_SIZE*A)+SFU_IMG_CHUNK_SIZE*B))

#define CHUNK_0_ADDR_MODIFIED(A,B) (((A==0) && (B==0))?\
                                    ((uint8_t*)((uint32_t) SFU_IMG_SLOT_0_REGION_BEGIN + SFU_IMG_IMAGE_OFFSET)) : \
                                    ((uint8_t *)((uint32_t) SFU_IMG_SLOT_0_REGION_BEGIN + (SFU_IMG_SWAP_REGION_SIZE*A) \
                                                 + SFU_IMG_CHUNK_SIZE*B)))
#define CHUNK_SWAP_ADDR(B) ((uint8_t *)((uint32_t) SFU_IMG_SWAP_REGION_BEGIN+SFU_IMG_CHUNK_SIZE*B))

#define AES_BLOCK_SIZE (16U)  /*!< Size of an AES block to check padding needs for decrypting */

/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Private_Types Private Types
  * @brief FWIMG Types used only by sfu_fwimg_core
  * @{
  */

/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Private_Variable Private Variables
  * @brief FWIMG Module Variables used only by sfu_fwimg_core
  * @{
  */
/**
  * FW header (metadata) of the candidate FW in slot #1: raw format (access by bytes)
  */
static uint8_t fw_header_to_test[FW_INFO_TOT_LEN] __attribute__((aligned(4)));

/**
  * FW header (metadata) of the active FW in slot #0: raw format (access by bytes)
  */
static uint8_t fw_header_validated[FW_INFO_TOT_LEN] __attribute__((aligned(4)));
static uint8_t fw_tag_validated[SE_TAG_LEN];

/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Private_Functions Private Functions
  *  @brief Private functions used by fwimg_core internally.
  *  @note All these functions should be declared as static and should NOT have the SFU_IMG prefix.
  * @{
  */


/**
  * @brief  Memory compare with constant time execution.
  * @note   Objective is to avoid basic attacks based on time execution
  * @param  pAdd1 Address of the first buffer to compare
  * @param  pAdd2 Address of the second buffer to compare
  * @param  Size Size of the comparison
  * @retval SFU_ SUCCESS if equal, a SFU_error otherwise.
  */
static SFU_ErrorStatus MemoryCompare(uint8_t *pAdd1, uint8_t *pAdd2, uint32_t Size)
{
  uint8_t result = 0x00;
  uint32_t i;

  for (i = 0; i < Size; i++)
  {
    result |= pAdd1[i] ^ pAdd2[i];
  }

  if (result == 0x00)
  {
    return SFU_SUCCESS;
  }
  else
  {
    return SFU_ERROR;
  }
}


/**
  * @brief  Check raw header
  * @param  pFWinfoInput: pointer to raw header. This must match SE_FwRawHeaderTypeDef.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus VerifyFwRawHeaderTag(uint8_t *pFWInfoInput)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef se_status;

  if (SE_VerifyFwRawHeaderTag(&se_status, (SE_FwRawHeaderTypeDef *)pFWInfoInput) == SE_SUCCESS)
  {
    FLOW_STEP(uFlowCryptoValue, FLOW_STEP_AUTHENTICATE);
    e_ret_status = SFU_SUCCESS;
  }

  return e_ret_status;
}


/**
  * @brief Secure Engine Firmware TAG verification (FW in non contiguous area).
  *        It handles Firmware TAG verification of a complete buffer by calling
  *        SE_AuthenticateFW_Init, SE_AuthenticateFW_Append and SE_AuthenticateFW_Finish inside the firewall.
  * @note: AES_GCM tag: In order to verify the TAG of a buffer, the function will re-encrypt it
  *        and at the end compare the obtained TAG with the one provided as input
  *        in pSE_GMCInit parameter.
  * @note: SHA-256 tag: a hash of the firmware is performed and compared with the digest stored in the Firmware header.
  * @param pSE_Status: Secure Engine Status.
  *        This parameter can be a value of @ref SE_Status_Structure_definition.
  * @param pSE_Metadata: Firmware metadata.
  * @param pSE_Payload: pointer to Payload Buffer descriptor.
  * @param SE_FwType: Type of Fw Image.
  *        This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
static SFU_ErrorStatus VerifyTagScatter(SE_StatusTypeDef *pSeStatus, SE_FwRawHeaderTypeDef *pSE_Metadata,
                                        SE_Ex_PayloadDescTypeDef  *pSE_Payload, int32_t SE_FwType)
{
  SE_ErrorStatus se_ret_status = SE_ERROR;
  SFU_ErrorStatus sfu_ret_status = SFU_SUCCESS;
  /* Loop variables */
  uint32_t i = 0;
  uint32_t j = 0;
  /* Variables to handle the FW image chunks to be injected in the verification procedure and the result */
  int32_t fw_tag_len = 0;             /* length of the authentication tag to be verified */
  int32_t fw_verified_total_size = 0; /* number of bytes that have been processed during authentication check */
  int32_t fw_chunk_size;              /* size of a FW chunk to be verified */
  /* Authentication tag computed in this procedure (to be compared with the one stored in the FW metadata) */
  uint8_t fw_tag_output[SE_TAG_LEN] __attribute__((aligned(4)));
  /* FW chunk produced by the verification procedure if any   */
  uint8_t fw_chunk[CHUNK_SIZE_SIGN_VERIFICATION] __attribute__((aligned(4)));
  /* FW chunk provided as input to the verification procedure */
  uint8_t fw_image_chunk[CHUNK_SIZE_SIGN_VERIFICATION] __attribute__((aligned(4)));
  /* Variables to handle the FW image (this will be split in chunks) */
  int32_t payloadsize;
  uint8_t *ppayload;
  uint32_t scatter_nb;
  /* Variables to handle FW image size and tag */
  uint32_t fw_size;
  uint8_t *fw_tag;

  /* Check the pointers allocation */
  if ((pSeStatus == NULL) || (pSE_Metadata == NULL) || (pSE_Payload == NULL))
  {
    return SFU_ERROR;
  }
  if ((pSE_Payload->pPayload[0] == NULL) || ((pSE_Payload->pPayload[1] == NULL) && pSE_Payload->PayloadSize[1]))
  {
    return SFU_ERROR;
  }

  /* Check the parameters value and set fw_size and fw_tag to check */
  if (SE_FwType == SE_FW_IMAGE_COMPLETE)
  {
    fw_size = pSE_Metadata->FwSize;
    fw_tag = pSE_Metadata->FwTag;
  }
  else if (SE_FwType == SE_FW_IMAGE_PARTIAL)
  {
    fw_size = pSE_Metadata->PartialFwSize;
    fw_tag = pSE_Metadata->PartialFwTag;
  }
  else
  {
    return SFU_ERROR;
  }

  if ((pSE_Payload->PayloadSize[0] + pSE_Payload->PayloadSize[1]) != fw_size)
  {
    return SFU_ERROR;
  }

  /*  fix number of scatter block */
  if (pSE_Payload->PayloadSize[1])
  {
    scatter_nb = 2;
  }
  else
  {
    scatter_nb = 1;
  }


  /* Encryption process*/
  se_ret_status = SE_AuthenticateFW_Init(pSeStatus, pSE_Metadata, SE_FwType);

  /* check for initialization errors */
  if ((se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK))
  {
    for (j = 0; j < scatter_nb; j++)
    {
      payloadsize = pSE_Payload->PayloadSize[j];
      ppayload = pSE_Payload->pPayload[j];
      i = 0;
      fw_chunk_size = CHUNK_SIZE_SIGN_VERIFICATION;

      while ((i < (payloadsize / CHUNK_SIZE_SIGN_VERIFICATION)) && (*pSeStatus == SE_OK) &&
             (sfu_ret_status == SFU_SUCCESS))
      {

        sfu_ret_status = SFU_LL_FLASH_Read(fw_image_chunk, ppayload, fw_chunk_size) ;
        if (sfu_ret_status == SFU_SUCCESS)
        {
          se_ret_status = SE_AuthenticateFW_Append(pSeStatus, fw_image_chunk, fw_chunk_size,
                                                   fw_chunk, &fw_chunk_size);
        }
        else
        {
          *pSeStatus = SE_ERR_FLASH_READ;
          se_ret_status = SE_ERROR;
          sfu_ret_status = SFU_ERROR;
        }
        ppayload += fw_chunk_size;
        fw_verified_total_size += fw_chunk_size;
        i++;
      }
      /* this the last path , size can be smaller */
      fw_chunk_size = (uint32_t)pSE_Payload->pPayload[j] + pSE_Payload->PayloadSize[j] - (uint32_t)ppayload;
      if ((fw_chunk_size) && (se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK))
      {
        sfu_ret_status = SFU_LL_FLASH_Read(fw_image_chunk, ppayload, fw_chunk_size) ;
        if (sfu_ret_status == SFU_SUCCESS)
        {

          se_ret_status = SE_AuthenticateFW_Append(pSeStatus, fw_image_chunk,
                                                   payloadsize - i * CHUNK_SIZE_SIGN_VERIFICATION, fw_chunk,
                                                   &fw_chunk_size);
        }
        else
        {
          *pSeStatus = SE_ERR_FLASH_READ;
          se_ret_status = SE_ERROR;
          sfu_ret_status = SFU_ERROR;
        }
        fw_verified_total_size += fw_chunk_size;
      }
    }
  }

  if ((sfu_ret_status == SFU_SUCCESS) && (se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK))
  {
    if (fw_verified_total_size <= fw_size)
    {
      /* Do the Finalization, check the authentication TAG*/
      fw_tag_len = sizeof(fw_tag_output);
      se_ret_status =   SE_AuthenticateFW_Finish(pSeStatus, fw_tag_output, &fw_tag_len);

      if ((se_ret_status == SE_SUCCESS) && (*pSeStatus == SE_OK) && (fw_tag_len == SE_TAG_LEN))
      {
        /* Firmware tag verification */
        if (MemoryCompare(fw_tag_output, fw_tag, SE_TAG_LEN) != SFU_SUCCESS)
        {
          *pSeStatus = SE_SIGNATURE_ERR;
          se_ret_status = SE_ERROR;
          sfu_ret_status = SFU_ERROR;
          memset(fw_tag_validated, 0x00, SE_TAG_LEN);
        }
        else
        {
          FLOW_STEP(uFlowCryptoValue, FLOW_STEP_INTEGRITY);
          memcpy(fw_tag_validated, fw_tag, SE_TAG_LEN);
        }
      }
      else
      {
        sfu_ret_status = SFU_ERROR;
      }
    }
    else
    {
      sfu_ret_status = SFU_ERROR;
    }
  }
  else
  {
    sfu_ret_status = SFU_ERROR;
  }
  return sfu_ret_status;
}


/**
  * @brief Secure Engine Firmware TAG verification (FW in contiguous area).
  *        It handles Firmware TAG verification of a complete buffer by calling
  *        SE_AuthenticateFW_Init, SE_AuthenticateFW_Append and SE_AuthenticateFW_Finish inside the firewall.
  * @note: AES_GCM tag: In order to verify the TAG of a buffer, the function will re-encrypt it
  *        and at the end compare the obtained TAG with the one provided as input
  *        in pSE_GMCInit parameter.
  * @note: SHA-256 tag: a hash of the firmware is performed and compared with the digest stored in the Firmware header.
  * @param pSE_Status: Secure Engine Status.
  *        This parameter can be a value of @ref SE_Status_Structure_definition.
  * @param pSE_Metadata: Firmware metadata.
  * @param pPayload: pointer to Payload Buffer.
  * @param SE_FwType: Type of Fw Image.
  *        This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
static SFU_ErrorStatus VerifyTag(SE_StatusTypeDef *pSeStatus, SE_FwRawHeaderTypeDef *pSE_Metadata,
                                 uint8_t  *pPayload, int32_t SE_FwType)
{
  SE_Ex_PayloadDescTypeDef  pse_payload;
  uint32_t fw_size;
  uint32_t fw_offset;

  if (NULL == pSE_Metadata)
  {
    /* This should not happen */
    return SFU_ERROR;
  }

  /* Check SE_FwType parameter, and fix size and offset accordingly */
  if (SE_FwType == SE_FW_IMAGE_COMPLETE)
  {
    fw_size = pSE_Metadata->FwSize;
    fw_offset = 0;
  }
  else if (SE_FwType == SE_FW_IMAGE_PARTIAL)
  {
    fw_size = pSE_Metadata->PartialFwSize;
    fw_offset = pSE_Metadata->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE;
  }
  else
  {
    return SFU_ERROR;
  }

  pse_payload.pPayload[0] = pPayload + fw_offset;
  pse_payload.PayloadSize[0] = fw_size;
  pse_payload.pPayload[1] = NULL;
  pse_payload.PayloadSize[1] = 0;

  return  VerifyTagScatter(pSeStatus, pSE_Metadata, &pse_payload, SE_FwType);

}


/**
  * @brief Fill authenticated info in SE_FwImage.
  * @param SFU_APP_Status
  * @param pBuffer
  * @param BufferSize
  * @retval SFU_SUCCESS if successful, a SFU_ERROR otherwise.
  */
static SFU_ErrorStatus ParseFWInfo(SE_FwRawHeaderTypeDef *pFwHeader, uint8_t *pBuffer)
{
  /* Check the pointers allocation */
  if ((pFwHeader == NULL) || (pBuffer == NULL))
  {
    return SFU_ERROR;
  }
  memcpy(pFwHeader, pBuffer, sizeof(*pFwHeader));
  return SFU_SUCCESS;
}


/**
  * @brief  Check  that a file header is valid
  *         (Check if the header has a VALID tag)
  * @param  phdr  pointer to header to check
  * @retval SFU_ SUCCESS if valid, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus CheckHeaderValidated(uint8_t *phdr)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  if (memcmp(phdr + FW_INFO_TOT_LEN - FW_INFO_MAC_LEN, phdr + FW_INFO_TOT_LEN, MAGIC_LENGTH))
  {
    return  e_ret_status;
  }
  if (memcmp(phdr + FW_INFO_TOT_LEN - FW_INFO_MAC_LEN, phdr + FW_INFO_TOT_LEN + MAGIC_LENGTH, MAGIC_LENGTH))
  {
    return  e_ret_status;
  }
  return SFU_SUCCESS;
}


/**
  * @brief  Verify image signature of binary not contiguous in flash
  * @param  pSeStatus pointer giving the SE status result
  * @param  pFwImageHeader pointer to fw header
  * @param  pPayloadDesc description of non contiguous fw
  * @param  SE_FwType: Type of Fw Image.
  *         This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus VerifyFwSignatureScatter(SE_StatusTypeDef *pSeStatus, SE_FwRawHeaderTypeDef *pFwImageHeader,
                                                SE_Ex_PayloadDescTypeDef *pPayloadDesc, int32_t SE_FwType)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  uint32_t fw_size;
  uint32_t fw_offset;

  if ((pFwImageHeader == NULL) || (pPayloadDesc == NULL))
  {
    return  e_ret_status;
  }
  if ((pPayloadDesc->pPayload[0] == NULL) || (pPayloadDesc->PayloadSize[0] < SFU_IMG_IMAGE_OFFSET))
  {
    return e_ret_status;
  }

  /* Check SE_FwType parameter, and fix size and offset accordingly */
  if (SE_FwType == SE_FW_IMAGE_COMPLETE)
  {
    fw_size = pFwImageHeader->FwSize;
    fw_offset = SFU_IMG_IMAGE_OFFSET;
  }
  else if (SE_FwType == SE_FW_IMAGE_PARTIAL)
  {
    fw_size = pFwImageHeader->PartialFwSize;
    fw_offset = (SFU_IMG_IMAGE_OFFSET + (pFwImageHeader->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE)) %
                SFU_IMG_SWAP_REGION_SIZE;
  }
  else
  {
    return SFU_ERROR;
  }

  /*
   * Adjusting the description of the way the Firmware is written in FLASH.
   */
  pPayloadDesc->pPayload[0] = (uint8_t *)((uint32_t)(pPayloadDesc->pPayload[0]) + fw_offset);

  /* The first part contains the execution offset so the payload size must be adjusted accordingly */
  pPayloadDesc->PayloadSize[0] = pPayloadDesc->PayloadSize[0] - fw_offset;

  if (fw_size <= pPayloadDesc->PayloadSize[0])
  {
    /* The firmware is written fully in a contiguous manner */
    pPayloadDesc->PayloadSize[0] = fw_size;
    pPayloadDesc->PayloadSize[1] = 0;
    pPayloadDesc->pPayload[1] = NULL;
  }
  else
  {
    /*
     * The firmware is too big to be contained in the first payload slot.
     * So, the firmware is split in 2 non-contiguous parts
     */

    if ((pPayloadDesc->pPayload[1] == NULL)
        || (pPayloadDesc->PayloadSize[1] < (fw_size - pPayloadDesc->PayloadSize[0])))
    {
      return  e_ret_status;
    }

    /* The second part contains the end of the firmware so the size is the total size - size already stored in the first
       area */
    pPayloadDesc->PayloadSize[1] = (fw_size - pPayloadDesc->PayloadSize[0]);

  }

  /* Signature Verification */
  return VerifyTagScatter(pSeStatus, pFwImageHeader, pPayloadDesc, SE_FwType);
}


/**
  * @brief  Check the magic from trailer or counter
  * @param  pMagicAddr: pointer to magic in Flash .
  * @param  pValidHeaderAddr: pointer to valid fw header in Flash
  * @param  pTestHeaderAddr: pointer to test fw header in Flash
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus CheckMagic(uint8_t *pMagicAddr, uint8_t *pValidHeaderAddr, uint8_t *pTestHeaderAddr)
{
  /*  as long as it is 128 bit key it is fine */
  uint8_t  magic[MAGIC_LENGTH];
  uint8_t  signature_valid[MAGIC_LENGTH / 2];
  uint8_t  signature_test[MAGIC_LENGTH / 2];

  if (SFU_LL_FLASH_Read(&magic, pMagicAddr, sizeof(magic)) != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
  if (SFU_LL_FLASH_Read(&signature_valid, (void *)((uint32_t)pValidHeaderAddr + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2),
                        sizeof(signature_valid)) != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
  if (SFU_LL_FLASH_Read(&signature_test, (void *)((uint32_t) pTestHeaderAddr + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2),
                        sizeof(signature_test)) != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
  if ((memcmp(magic, signature_valid, sizeof(signature_valid)))
      || (memcmp(&magic[MAGIC_LENGTH / 2], signature_test, sizeof(signature_test))))
  {
    return SFU_ERROR;
  }
  return SFU_SUCCESS;
}

/**
  * @brief  Write the minimum value possible on the flash
  * @param  pAddr: pointer to address to write .
  * @param  pValue: pointer to the value to write
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  *
  * @note This function should be FLASH dependent.
  *       We abstract this dependency thanks to the type SFU_LL_FLASH_write_t.
  *       See @ref SFU_LL_FLASH_write_t
  */
static SFU_ErrorStatus AtomicWrite(uint8_t *pAddr, SFU_LL_FLASH_write_t *pValue)
{
  SFU_FLASH_StatusTypeDef flash_if_info;

  return SFU_LL_FLASH_Write(&flash_if_info, pAddr, pValue, sizeof(SFU_LL_FLASH_write_t));
}

/**
  * @brief  Clean Magic value
  * @param  pMagicAddr: pointer to magic address to write .
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus CleanMagicValue(uint8_t *pMagicAddr)
{
  SFU_FLASH_StatusTypeDef flash_if_info;
  return SFU_LL_FLASH_Write(&flash_if_info, pMagicAddr, MAGIC_NULL, MAGIC_LENGTH);
}


/**
  * @brief  Compute Magic and Write Magic
  * @param  pMagicAddr: pointer to magic address to write .
  * @param  pHeaderValid: pointer to header of firmware validated
  * @param  pHeaderToInstall: pointer to header of firmware to install
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus WriteMagic(uint8_t *pMagicAddr, uint8_t *pHeaderValid, uint8_t *pHeaderToInstall)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  uint8_t  magic[MAGIC_LENGTH];
  uint32_t i;
  SFU_FLASH_StatusTypeDef flash_if_info;

  (void)SFU_LL_FLASH_Read(&magic[0], (void *)((uint32_t)pHeaderValid + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2),
                          MAGIC_LENGTH / 2);
  (void)SFU_LL_FLASH_Read(&magic[MAGIC_LENGTH / 2],
                          (void *)((uint32_t)pHeaderToInstall + FW_INFO_TOT_LEN - MAGIC_LENGTH / 2), MAGIC_LENGTH / 2);
  for (i = 0; i < MAGIC_LENGTH ; i++)
  {
    /* Write MAGIC only if not MAGIC_NULL */
    if (magic[i] != 0)
    {
      e_ret_status = SFU_LL_FLASH_Write(&flash_if_info, pMagicAddr, magic, sizeof(magic));
      break;
    }
  }
  return  e_ret_status;
}

/**
  * @brief  Write Trailer Header
  * @param  pTestHeader: pointer in ram to header of fw to test
  * @param  pValidHeader: pointer in ram to header of valid fw to backup
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
static SFU_ErrorStatus WriteTrailerHeader(uint8_t *pTestHeader, uint8_t *pValidHeader)
{
  /* everything is in place , just compute from present data and write it */
  SFU_ErrorStatus e_ret_status;
  SFU_FLASH_StatusTypeDef flash_if_info;

  e_ret_status = SFU_LL_FLASH_Write(&flash_if_info, TRAILER_HDR_TEST, pTestHeader, FW_INFO_TOT_LEN);
  if (e_ret_status == SFU_SUCCESS)
  {
    e_ret_status = SFU_LL_FLASH_Write(&flash_if_info, TRAILER_HDR_VALID, pValidHeader, FW_INFO_TOT_LEN);
  }
  return e_ret_status;
}

/**
  * @brief  Erase the size of the swap area in a given slot sector.
  * @note   The erasure occurs at @: @slot + index*swap_area_size
  * @param  SlotNumber This is Slot #0 or Slot #1
  * @param  Index This is the number of "swap size" we jump from the slot start
  * @retval SFU_ SUCCESS if valid, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus EraseSlotIndex(uint32_t SlotNumber, uint32_t index)
{
  SFU_FLASH_StatusTypeDef flash_if_status;
  SFU_ErrorStatus e_ret_status;
  uint8_t *pbuffer;
  if (SlotNumber > (SFU_SLOTS - 1))
  {
    return SFU_ERROR;
  }
  pbuffer = (uint8_t *) SlotHeaderAddress[SlotNumber];
  pbuffer = pbuffer + (SFU_IMG_SWAP_REGION_SIZE * index);
  e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, pbuffer, SFU_IMG_SWAP_REGION_SIZE) ;
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED)
  return e_ret_status;
}

/**
  * @brief  Verify image signature of binary after decryption
  * @param  pSeStatus pointer giving the SE status result
  * @param  pFwImageHeader pointer to fw header
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus VerifyFwSignatureAfterDecrypt(SE_StatusTypeDef *pSeStatus,
                                                     SE_FwRawHeaderTypeDef *pFwImageHeader)
{
  SE_Ex_PayloadDescTypeDef payload_desc;
  /*
   * The values below are not necessarily matching the way the firmware
   * has been spread in FLASH but this is adjusted in @ref VerifyFwSignatureScatter.
   */
  payload_desc.pPayload[0] = (uint8_t *)SFU_IMG_SWAP_REGION_BEGIN;
  payload_desc.PayloadSize[0] = (uint32_t) SFU_IMG_SWAP_REGION_SIZE;
  payload_desc.pPayload[1] = (uint8_t *)SFU_IMG_SLOT_1_REGION_BEGIN;
  payload_desc.PayloadSize[1] = (uint32_t)SFU_IMG_SLOT_1_REGION_SIZE;
  return VerifyFwSignatureScatter(pSeStatus, pFwImageHeader, &payload_desc, SE_FW_IMAGE_PARTIAL);
}


/**
  * @brief  Swap Slot 0 with decrypted FW to install
  *         Relies on following global in ram, already filled by the check: fw_image_header_to_test.
  *         With the 2 images implementation, installing a new Firmware Image means swapping Slot #0 and Slot #1.
  *         To perform this swap, the image to be installed is split in blocks of the swap region size:
  *         SFU_IMG_SLOT_1_REGION_SIZE / SFU_IMG_SWAP_REGION_SIZE blocks to be swapped .
  *         Each of these blocks is swapped using smaller chunks of SFU_IMG_CHUNK_SIZE size.
  *         The swap starts from the tail of the image and ends with the beginning of the image ("swap from tail to
  *         head").
  * @param  None.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus SwapFirmwareImages(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SFU_FLASH_StatusTypeDef flash_if_status;
  SFU_LL_FLASH_write_t trailer;
  int32_t index_slot0;
  int32_t index_slot1_write;
  int32_t index_slot1_read;
  int32_t chunk;
  uint8_t buffer[SFU_IMG_CHUNK_SIZE] __attribute__((aligned(4)));
  int32_t index_slot0_partial_begin;
  int32_t index_slot0_partial_end;
  int32_t index_slot0_final_end;
  int32_t index_slot1_partial_end;
  int32_t index_slot0_empty_begin;
  /* number_of_index_slot0 is the number of blocks in slot #0 (block of SFU_IMG_SWAP_REGION_SIZE bytes) */
  uint32_t number_of_index_slot0 = SFU_IMG_SLOT_0_REGION_SIZE / SFU_IMG_SWAP_REGION_SIZE;
  /* number_of_index_slot1 is the number of blocks in slot #1 (block of SFU_IMG_SWAP_REGION_SIZE bytes) */
  uint32_t number_of_index_slot1 = SFU_IMG_SLOT_1_REGION_SIZE / SFU_IMG_SWAP_REGION_SIZE;
  /* number_of_chunk is the number of chunks used to swap 1 block (moving a block of SFU_IMG_SWAP_REGION_SIZE bytes
     split in number_of_chunk chunks of SFU_IMG_CHUNK_SIZE bytes) */
  uint32_t number_of_chunk = SFU_IMG_SWAP_REGION_SIZE / SFU_IMG_CHUNK_SIZE;
  uint32_t write_len;
  uint32_t offset_block_partial_begin;
  uint32_t offset_block_partial_end;
  uint32_t offset_block_final_end;

  TRACE("\r\n\t  Image preparation done.\r\n\t  Swapping the firmware images");

  /* index_slot0_partial_begin is the index of first block (of SFU_IMG_SWAP_REGION_SIZE bytes) in slot #0 impacted
     by partial image */
  index_slot0_partial_begin = (SFU_IMG_IMAGE_OFFSET + fw_image_header_to_test.PartialFwOffset) /
                              SFU_IMG_SWAP_REGION_SIZE;

  /* offset_block_partial_begin is the offset of first byte of partial image inside index_slot0_partial_begin block */
  offset_block_partial_begin = (SFU_IMG_IMAGE_OFFSET + fw_image_header_to_test.PartialFwOffset) %
                               SFU_IMG_SWAP_REGION_SIZE;

  /* index_slot0_partial_end is the index of block (of SFU_IMG_SWAP_REGION_SIZE bytes) of first byte following partial
     image in slot #0 */
  index_slot0_partial_end = (SFU_IMG_IMAGE_OFFSET + fw_image_header_to_test.PartialFwOffset +
                             fw_image_header_to_test.PartialFwSize) / SFU_IMG_SWAP_REGION_SIZE;

  /* offset_block_partial_end is the offset of first byte following partial image, inside index_slot0_partial_end
     block */
  offset_block_partial_end = (SFU_IMG_IMAGE_OFFSET + fw_image_header_to_test.PartialFwOffset +
                              fw_image_header_to_test.PartialFwSize) % SFU_IMG_SWAP_REGION_SIZE;

  /* index_slot1_partial_end is the index of block (of SFU_IMG_SWAP_REGION_SIZE bytes) of first byte following partial
     image in slot #1 or swap area */
  index_slot1_partial_end = (((SFU_IMG_IMAGE_OFFSET + (fw_image_header_to_test.PartialFwOffset %
                                                       SFU_IMG_SWAP_REGION_SIZE)) % SFU_IMG_SWAP_REGION_SIZE +
                              fw_image_header_to_test.PartialFwSize) / SFU_IMG_SWAP_REGION_SIZE) - 1;

  /* index_slot0_final_end is the index of block (of SFU_IMG_SWAP_REGION_SIZE bytes) of first byte following final
     image in slot #0 */
  index_slot0_final_end = ((SFU_IMG_IMAGE_OFFSET + fw_image_header_to_test.FwSize) / SFU_IMG_SWAP_REGION_SIZE);

  /* offset_block_final_end is the offset of first byte following final image, inside index_slot0_final_end block */
  offset_block_final_end = (SFU_IMG_IMAGE_OFFSET + fw_image_header_to_test.FwSize) % SFU_IMG_SWAP_REGION_SIZE;

  /* set index_slot0: starting from the end, block index from slot #0 to copy in slot #1 */
  index_slot0 = index_slot0_partial_end;

  /* set index_slot1_read: starting from the end, block index in slot #1 or swap area to copy updated image to
     slot #0 */
  index_slot1_read = index_slot1_partial_end;

  /* set index_slot1_write: starting from the end, block index in slot #1 or swap area to receive block from slot #0 */
  if (index_slot0_partial_end == (number_of_index_slot0 - 1))
  {
    /* last block of slot #1 can only receive last block of slot #0, due to trailer presence */
    index_slot1_write = number_of_index_slot1 - 1;
  }
  else
  {
    index_slot1_write = number_of_index_slot1 - 2;
  }

  /* Adjust indexes in case offset is 0: in this case, first block to swap is the previous one */
  if (offset_block_partial_end == 0)
  {
    index_slot0--;
    index_slot1_read--;
  }

  /*
   * Slot #1 read index should be lower than slot #1 write index.
   * If not the case, the slot 1 is not big enough to manage this partial firmware image (depends on offset and size).
   */
  if (index_slot1_read >= index_slot1_write)
  {
    return SFU_ERROR;
  }

  /* Swap one block at each loop, until all targeted blocks of slot #0 have been copied
   * Concerned blocks are:
   * 1- block impacted by partial image
   * 2- first block containing the header
   */
  while (index_slot0 >= 0)
  {
    SFU_LL_SECU_IWDG_Refresh();

    TRACE(".");

    if ((index_slot1_write < -1) || (index_slot1_read < -1))
    {
      return SFU_ERROR;
    }

    /* If CPY_TO_SLOT1(i) is still virgin (value NOT_SWAPPED, and no NMI), then swap the block from slot #0 to
       slot #1 */
    e_ret_status = SFU_LL_FLASH_Read(&trailer, TRAILER_CPY_TO_SLOT1(TRAILER_INDEX - 1 - index_slot0), sizeof(trailer));
    if ((e_ret_status == SFU_SUCCESS) && (memcmp(&trailer, NOT_SWAPPED, sizeof(trailer)) == 0))
    {
      /* Erase destination block in slot #1 or swap, if not the trailer block */
      if (index_slot1_write != (number_of_index_slot1 - 1))
      {
        if (index_slot1_write == -1)
        {
          /* Erase the swap */
          e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *) SFU_IMG_SWAP_REGION_BEGIN_VALUE,
                                                 SFU_IMG_SWAP_REGION_SIZE);
          StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED)
        }
        else /* index_slot1_write >= 0 */
        {
          /* erase the size of "swap area" at @: slot #1 + index_slot1_write*swap_area_size */
          e_ret_status = EraseSlotIndex(1, index_slot1_write);
        }
        if (e_ret_status !=  SFU_SUCCESS)
        {
          return SFU_ERROR;
        }
      }

      /* Copy the block from slot #0 to slot #1 or swap (using "number_of_chunk" chunks) */
      for (chunk = (number_of_chunk - 1); chunk >= 0 ; chunk--)
      {
        /* ignore return value,  no double ecc error is expected, area already read before */
        (void)SFU_LL_FLASH_Read(buffer, CHUNK_0_ADDR(index_slot0, chunk), sizeof(buffer));
        write_len = sizeof(buffer);
        if (index_slot1_write == -1)
        {
          /* Destination block is the swap */
          e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, CHUNK_SWAP_ADDR(chunk), buffer, write_len);
          StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
          if (e_ret_status != SFU_SUCCESS)
          {
            return SFU_ERROR;
          }
        }
        else /* index_slot1_write >= 0 */
        {
          /* Destination block is in slot #1: Do not overwrite the trailer. */
          if (((uint32_t)CHUNK_1_ADDR(index_slot1_write, chunk)) < (uint32_t)TRAILER_BEGIN)
          {
            /*  write is possible length can be modified  */
            if ((uint32_t)(CHUNK_1_ADDR(index_slot1_write, chunk) + write_len) > (uint32_t)TRAILER_BEGIN)
            {
              write_len = TRAILER_BEGIN - CHUNK_1_ADDR(index_slot1_write, chunk);
            }
            e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, CHUNK_1_ADDR(index_slot1_write, chunk), buffer,
                                              write_len);
            StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
            if (e_ret_status != SFU_SUCCESS)
            {
              return SFU_ERROR;
            }
          }
        }
      }

      /*
       * The block of the active firmware has been backed up.
       * The trailer is updated to memorize this: the CPY bytes at the appropriate index are set to SWAPPED.
       */
      e_ret_status  = AtomicWrite(TRAILER_CPY_TO_SLOT1((TRAILER_INDEX - 1 - index_slot0)),
                                  (SFU_LL_FLASH_write_t *) SWAPPED);

      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
      if (e_ret_status != SFU_SUCCESS)
      {
        return e_ret_status;
      }
    }

    /* If CPY_TO_SLOT0(i) is still virgin (value NOT_SWAPPED, and no NMI), then swap the block from slot #1 to
    slot #0 */
    e_ret_status = SFU_LL_FLASH_Read(&trailer, TRAILER_CPY_TO_SLOT0(TRAILER_INDEX - 1 - index_slot0), sizeof(trailer));
    if ((e_ret_status == SFU_SUCCESS) && (memcmp(&trailer, NOT_SWAPPED, sizeof(trailer)) == 0))
    {
      /* erase the size of "swap area" at @: slot #0 + index_slot0*swap_area_size*/
      e_ret_status = EraseSlotIndex(0, index_slot0);

      if (e_ret_status !=  SFU_SUCCESS)
      {
        return SFU_ERROR;
      }

      /*
       * Fill block in slot #0:
       * The appropriate update image block of slot #1 together with initial image block that has been backed up in
       * slot #1,
       * are associated to constitute final block in slot #0 (installing the block of the new firmware image).
       */
      for (chunk = (number_of_chunk - 1); chunk >= 0 ; chunk--)
      {
        /* Read complete chunk of updated image */
        if (index_slot1_read == -1)
        {
          /* ignore return value, no double ecc error is expected, area already read before */
          (void)SFU_LL_FLASH_Read(buffer, CHUNK_SWAP_ADDR(chunk), sizeof(buffer));
        }
        else /* index_slot1_read >= 0 */
        {
          /* ignore return value, no double ecc error is expected, area already read before */
          (void)SFU_LL_FLASH_Read(buffer, CHUNK_1_ADDR(index_slot1_read, chunk), sizeof(buffer));
        }

        /* Last impacted block: end of block has to be updated with initial image */
        if (index_slot0 == index_slot0_partial_end)
        {
          /* If chunk is not impacted by updated image, chunk fully takes initial image content */
          if (CHUNK_1_ADDR(index_slot1_write, chunk) >= (CHUNK_1_ADDR(index_slot1_write, 0) + offset_block_partial_end))
          {
            /* check return value, as this area has not been read before */
            e_ret_status = SFU_LL_FLASH_Read(buffer, CHUNK_1_ADDR(index_slot1_write, chunk), sizeof(buffer));
            StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
            if (e_ret_status != SFU_SUCCESS)
            {
              return SFU_ERROR;
            }
          }

          /* If chunk is partially impacted by updated image, end of chunk is updated with the initial image content */
          if ((CHUNK_1_ADDR(index_slot1_write, chunk) < (CHUNK_1_ADDR(index_slot1_write, 0) + offset_block_partial_end))
              && ((CHUNK_1_ADDR(index_slot1_write, (chunk + 1))) > (CHUNK_1_ADDR(index_slot1_write, 0) +
                                                                    offset_block_partial_end)))
          {
            /* check return value, as this area has not been read before */
            e_ret_status = SFU_LL_FLASH_Read(buffer + (offset_block_partial_end % SFU_IMG_CHUNK_SIZE),
                                             CHUNK_1_ADDR(index_slot1_write, chunk) + (offset_block_partial_end %
                                                                                       SFU_IMG_CHUNK_SIZE),
                                             sizeof(buffer) - (offset_block_partial_end % SFU_IMG_CHUNK_SIZE));
            StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
            if (e_ret_status != SFU_SUCCESS)
            {
              return SFU_ERROR;
            }
          }
        }

        /* First impacted block: beginning of block has to be updated with initial image */
        if (index_slot0 == index_slot0_partial_begin)
        {
          /* If chunk is not impacted by updated image, chunk fully takes initial image content */
          if (CHUNK_1_ADDR(index_slot1_write, (chunk + 1)) <= (CHUNK_1_ADDR(index_slot1_write, 0) +
                                                               offset_block_partial_begin))
          {
            /* check return value, as this area has not been read before */
            e_ret_status = SFU_LL_FLASH_Read(buffer, CHUNK_1_ADDR(index_slot1_write, chunk), sizeof(buffer));
            StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
            if (e_ret_status != SFU_SUCCESS)
            {
              return SFU_ERROR;
            }
          }

          /* If chunk is partially impacted by updated image, beginning of chunk is updated with the initial image
             content */
          if ((CHUNK_1_ADDR(index_slot1_write, chunk) < (CHUNK_1_ADDR(index_slot1_write, 0) +
                                                         offset_block_partial_begin))
              && ((CHUNK_1_ADDR(index_slot1_write, (chunk + 1))) > (CHUNK_1_ADDR(index_slot1_write, 0) +
                                                                    offset_block_partial_begin)))
          {
            /* check return value, as this area has not been read before */
            e_ret_status = SFU_LL_FLASH_Read(buffer, CHUNK_1_ADDR(index_slot1_write, chunk), offset_block_partial_begin
                                             % SFU_IMG_CHUNK_SIZE);
            StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
            if (e_ret_status != SFU_SUCCESS)
            {
              return SFU_ERROR;
            }
          }
        }
        /* Header block not impacted by partial image: complete block, except header, is updated with initial image */
        else if (index_slot0 == 0)
        {
          /* check return value, as this area has not been read before */
          e_ret_status = SFU_LL_FLASH_Read(buffer, CHUNK_1_ADDR(index_slot1_write, chunk), sizeof(buffer));
          StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
          if (e_ret_status != SFU_SUCCESS)
          {
            return SFU_ERROR;
          }
        }

        /* Last block of final image: end of block has to be cleaned with empty data */
        if (index_slot0 == index_slot0_final_end)
        {
          /* If chunk is completely beyond the final image, then chunk takes empty data */
          if (CHUNK_0_ADDR(index_slot0, chunk) >= CHUNK_0_ADDR(index_slot0_final_end, 0) + offset_block_final_end)
          {
            memset(buffer, 0xFF, sizeof(buffer));
          }

          /* If chunk is partially beyond the final image, then chunk is partially updated with empty data */
          if ((CHUNK_0_ADDR(index_slot0, chunk) < (CHUNK_0_ADDR(index_slot0_final_end, 0) + offset_block_final_end)) &&
              (CHUNK_0_ADDR(index_slot0, (chunk + 1)) > (CHUNK_0_ADDR(index_slot0_final_end, 0) +
                                                         offset_block_final_end)))
          {
            memset(buffer + (offset_block_final_end % SFU_IMG_CHUNK_SIZE), 0xFF,
                   sizeof(buffer) - (offset_block_final_end % SFU_IMG_CHUNK_SIZE));
          }
        }

        write_len = SFU_IMG_CHUNK_SIZE;
        /*  don't copy header on slot 0, fix me chunk minimum size is SFU_IMG_IMAGE_OFFSET  */
        if ((index_slot0 == 0) && (chunk == 0))
        {
          write_len = write_len - SFU_IMG_IMAGE_OFFSET;
        }
        e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, CHUNK_0_ADDR_MODIFIED(index_slot0, chunk), buffer,
                                          write_len);
        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
        if (e_ret_status != SFU_SUCCESS)
        {
          return SFU_ERROR;
        }
      }

      /*
       * The block of the active firmware has been backed up.
       * The trailer is updated to memorize this: the CPY bytes at the appropriate index are set to SWAPPED.
       * Except for block 0, because block 0 is incomplete at this stage (final header installation not yet done)
       */
      if (index_slot0 != 0)
      {
        e_ret_status  = AtomicWrite(TRAILER_CPY_TO_SLOT0((TRAILER_INDEX - 1 - index_slot0)),
                                    (SFU_LL_FLASH_write_t *) SWAPPED);

        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
        if (e_ret_status != SFU_SUCCESS)
        {
          return e_ret_status;
        }
      }
    }

    /* Decrement all block indexes */
    index_slot1_write--;
    index_slot1_read--;
    index_slot0--;

    /*
     * After having copied the impacted image blocks from slot 0, copy also the first block
     * due to header, if not already copied among the impacted image blocks.
     */
    if ((index_slot0 < index_slot0_partial_begin) && (index_slot0 >= 0))
    {
      index_slot0 = 0;
      index_slot1_read = -1;  /* force read from swap area, to avoid exiting slot 1 / swap index allowed range */
    }
  }

  /*
   * Now, erase the blocks in slot #0, located after final image
   */
  if (offset_block_final_end == 0)
  {
    index_slot0_empty_begin = index_slot0_final_end;
  }
  else
  {
    index_slot0_empty_begin = index_slot0_final_end + 1;
  }

  index_slot0 = number_of_index_slot0 - 1;
  while (index_slot0 >= index_slot0_empty_begin)
  {
    e_ret_status = EraseSlotIndex(0, index_slot0);
    if (e_ret_status !=  SFU_SUCCESS)
    {
      return SFU_ERROR;
    }

    /* Decrement block index */
    index_slot0--;
  }

  return e_ret_status;
}

/**
  * @brief Decrypt Image in slot #1
  * @ note Decrypt is done from slot 1 to slot 1 + swap with 2 images (swap contains 1st sector)
  * @param  pFwImageHeader
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
static SFU_ErrorStatus DecryptImageInSlot1(SE_FwRawHeaderTypeDef *pFwImageHeader)
{
  SFU_ErrorStatus  e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status;
  SE_ErrorStatus   se_ret_status;
  uint32_t NumberOfChunkPerSwap = SFU_IMG_SWAP_REGION_SIZE / SFU_IMG_CHUNK_SIZE;
  SFU_FLASH_StatusTypeDef flash_if_status;
  /*  chunk size is the maximum , the 1st block can be smaller */
  /*  the chunk is static to avoid  large stack */
  uint8_t fw_decrypted_chunk[SFU_IMG_CHUNK_SIZE] __attribute__((aligned(4)));
  uint8_t fw_encrypted_chunk[SFU_IMG_CHUNK_SIZE] __attribute__((aligned(4)));
  uint8_t *pfw_source_address;
  uint32_t fw_dest_address_write = 0U;
  uint32_t fw_dest_erase_address = 0U;
  int32_t fw_decrypted_total_size = 0;
  int32_t size;
  int32_t oldsize;
  int32_t fw_decrypted_chunk_size;
  int32_t fw_tag_len = 0;
  uint8_t fw_tag_output[SE_TAG_LEN];
  uint32_t pass_index = 0;
  uint32_t erase_index = 0;

  if ((pFwImageHeader == NULL))
  {
    return e_ret_status;
  }

  /* Decryption process*/
  se_ret_status = SE_Decrypt_Init(&e_se_status, pFwImageHeader, SE_FW_IMAGE_PARTIAL);
  if ((se_ret_status == SE_SUCCESS) && (e_se_status == SE_OK))
  {
    e_ret_status = SFU_SUCCESS;
    size = SFU_IMG_CHUNK_SIZE;

    /* Decryption loop*/
    while ((e_ret_status == SFU_SUCCESS) && (fw_decrypted_total_size < (pFwImageHeader->PartialFwSize)) &&
           (e_se_status == SE_OK))
    {
      if (pass_index == NumberOfChunkPerSwap)
      {
        fw_dest_address_write = (uint32_t) SFU_IMG_SLOT_1_REGION_BEGIN;
        fw_dest_erase_address =  fw_dest_address_write;
        erase_index = NumberOfChunkPerSwap;
      }
      if (pass_index == 0)
      {
        pfw_source_address = (uint8_t *)((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + SFU_IMG_IMAGE_OFFSET +
                                         (pFwImageHeader->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE));
        fw_dest_erase_address = (uint32_t)SFU_IMG_SWAP_REGION_BEGIN;
        fw_dest_address_write = fw_dest_erase_address + ((SFU_IMG_IMAGE_OFFSET + (pFwImageHeader->PartialFwOffset %
                                                                                  SFU_IMG_SWAP_REGION_SIZE)) %
                                                         SFU_IMG_SWAP_REGION_SIZE);
        fw_decrypted_chunk_size = sizeof(fw_decrypted_chunk) - ((SFU_IMG_IMAGE_OFFSET + (pFwImageHeader->PartialFwOffset
                                                                 % SFU_IMG_SWAP_REGION_SIZE)) %
                                                                sizeof(fw_decrypted_chunk));
        if (fw_decrypted_chunk_size > pFwImageHeader->PartialFwSize)
        {
          fw_decrypted_chunk_size = pFwImageHeader->PartialFwSize;
        }
        pass_index = ((SFU_IMG_IMAGE_OFFSET + (pFwImageHeader->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE)) /
                      sizeof(fw_decrypted_chunk));
      }
      else
      {
        fw_decrypted_chunk_size = sizeof(fw_decrypted_chunk);

        /* For the last 2 pass, divide by 2 remaining buffer to ensure that :
         *     - chunk size greater than 16 bytes : minimum size of a block to be decrypted
         *     - except last one chunk size is 16 bytes aligned
         *
         * Pass n - 1 : remaining bytes / 2 with (16 bytes alignment for crypto AND sizeof(SFU_LL_FLASH_write_t) for
         *              flash programming)
         * Pass n : remaining bytes
         */

        /* Last pass : n */
        if ((pFwImageHeader->PartialFwSize - fw_decrypted_total_size) < fw_decrypted_chunk_size)
        {
          fw_decrypted_chunk_size = pFwImageHeader->PartialFwSize - fw_decrypted_total_size;
        }
        /* Previous pass : n - 1 */
        else if ((pFwImageHeader->PartialFwSize - fw_decrypted_total_size) < ((2 * fw_decrypted_chunk_size) - 16U))
        {
          fw_decrypted_chunk_size = ((pFwImageHeader->PartialFwSize - fw_decrypted_total_size) / 32U) * 16U;

          /* Set dimension to the appropriate length for FLASH programming.
           * Example: 64-bit length for L4.
           */
          if ((fw_decrypted_chunk_size & ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - 1U)) != 0)
          {
            fw_decrypted_chunk_size = fw_decrypted_chunk_size + ((uint32_t)sizeof(SFU_LL_FLASH_write_t) -
                                                                 (fw_decrypted_chunk_size %
                                                                  (uint32_t)sizeof(SFU_LL_FLASH_write_t)));
          }
        }
        /* All others pass */
        else
        {
          /* nothing */
        }
      }

      size = fw_decrypted_chunk_size;

      /* Decrypt Append*/
      e_ret_status = SFU_LL_FLASH_Read(fw_encrypted_chunk, pfw_source_address, size);
      if (e_ret_status != SFU_SUCCESS)
      {
        break;
      }
      if (size != 0)
      {
        se_ret_status = SE_Decrypt_Append(&e_se_status, (uint8_t *)fw_encrypted_chunk, size, fw_decrypted_chunk,
                                          &fw_decrypted_chunk_size);
      }
      else
      {
        e_ret_status = SFU_SUCCESS;
        fw_decrypted_chunk_size = 0;
      }
      if ((se_ret_status == SE_SUCCESS) && (e_se_status == SE_OK) && (fw_decrypted_chunk_size == size))
      {
        /* Erase Page */
        if ((pass_index == erase_index)
            || (pass_index == ((SFU_IMG_IMAGE_OFFSET + (pFwImageHeader->PartialFwOffset % SFU_IMG_SWAP_REGION_SIZE)) /
                               sizeof(fw_decrypted_chunk))))
        {
          SFU_LL_SECU_IWDG_Refresh();
          e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)fw_dest_erase_address,
                                                 SFU_IMG_SWAP_REGION_SIZE) ;
          erase_index += NumberOfChunkPerSwap;
          fw_dest_erase_address += SFU_IMG_SWAP_REGION_SIZE;
        }
        StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);

        if (e_ret_status == SFU_SUCCESS)
        {
          /*
           * For last pass with remaining size not aligned on 16 bytes : Set dimension AFTER decrypt to the appropriate
           * length for FLASH programming.
           * Example: 64-bit length for L4.
           */
          if ((size & ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - 1U)) != 0)
          {
            /*
            * By construction, SFU_IMG_CHUNK_SIZE is a multiple of sizeof(SFU_LL_FLASH_write_t) so there is no risk to
             * read out of the buffer
             */
            oldsize = size;
            size = size + ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - (size % (uint32_t)sizeof(SFU_LL_FLASH_write_t)));
            while (oldsize < size)
            {
              fw_decrypted_chunk[oldsize] = 0xFF;
              oldsize++;
            }
          }

          /* Write Decrypted Data in Flash - size has to be 32-bit aligned */
          e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, (void *)fw_dest_address_write,  fw_decrypted_chunk, size);
          StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);

          if (e_ret_status == SFU_SUCCESS)
          {
            /* Update flash pointer */
            fw_dest_address_write  += (size);

            /* Update source pointer */
            pfw_source_address += size;
            fw_decrypted_total_size += size;
            memset(fw_decrypted_chunk, 0xff, sizeof(fw_decrypted_chunk));
            pass_index += 1;

          }
        }
      }
    }
  }

#if (SFU_IMAGE_PROGRAMMING_TYPE == SFU_ENCRYPTED_IMAGE)
#if defined(SFU_VERBOSE_DEBUG_MODE)
  TRACE("\r\n\t  %d bytes of ciphertext decrypted.", fw_decrypted_total_size);
#endif /* SFU_VERBOSE_DEBUG_MODE */
#endif /* SFU_ENCRYPTED_IMAGE */

  if ((se_ret_status == SE_SUCCESS) && (e_ret_status == SFU_SUCCESS) && (e_se_status == SE_OK))
  {
    /* Do the Finalization, check the authentication TAG*/
    fw_tag_len = sizeof(fw_tag_output);
    se_ret_status = SE_Decrypt_Finish(&e_se_status, fw_tag_output, &fw_tag_len);
    if ((se_ret_status != SE_SUCCESS) || (e_se_status != SE_OK))
    {
      e_ret_status = SFU_ERROR;
#if defined(SFU_VERBOSE_DEBUG_MODE)
      TRACE("\r\n\t  Decrypt fails at Finalization stage.");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    }
    else
    {
      /* erase the remaining part of the image after shifting inside slot1 */
      if (pass_index <= NumberOfChunkPerSwap)
      {
        fw_dest_erase_address = (uint32_t) SFU_IMG_SLOT_1_REGION_BEGIN;
      }
      e_ret_status = SFU_LL_FLASH_Erase_Size(&flash_if_status, (void *)fw_dest_erase_address, SFU_IMG_SWAP_REGION_SIZE);
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_ERASE_FAILED);
    }
  }
  else
  {
    e_ret_status = SFU_ERROR;
  }
  return e_ret_status;
}


/**
  * @}
  */

/** @defgroup SFU_IMG_CORE_Exported_Functions Exported Functions
  * @brief Functions used by fwimg_services
  * @note All these functions are also listed in the Common services (High Level and Low Level Services).
  * @{
  */

/**
  * @brief  Initialize the SFU APP.
  * @param  None
  * @note   Not used in Alpha version -
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_IMG_InitStatusTypeDef SFU_IMG_CoreInit(void)
{
  SFU_IMG_InitStatusTypeDef e_ret_status = SFU_IMG_INIT_OK;

  /*
   * When there is no valid FW in slot 0, the fw_header_validated array is filled with 0s.
   * When installing a first FW (after local download) this means that WRITE_TRAILER_MAGIC will write a SWAP magic
   * starting with 0s.
   * This causes an issue when calling CLEAN_TRAILER_MAGIC (because of this we added an erase that generated
   * side-effects).
   * To avoid all these problems we can initialize fw_header_validated with a non-0  value.
   */
  memset(fw_header_validated, 0xFE, sizeof(fw_header_validated));

  /*
   * Sanity check: let's make sure the slot sizes are correct
   * to avoid discrepancies between linker definitions and constants in sfu_fwimg_regions.h
   */
  if (!(SFU_IMG_REGION_IS_MULTIPLE(SFU_IMG_SLOT_0_REGION_SIZE, SFU_IMG_SWAP_REGION_SIZE)))
  {
    TRACE("\r\n= [FWIMG] The image slot size (%d) must be a multiple of the swap region size (%d)\r\n",
          SFU_IMG_SLOT_0_REGION_SIZE, SFU_IMG_SWAP_REGION_SIZE);
    e_ret_status = SFU_IMG_INIT_SLOTS_SIZE_ERROR;
  }
  else
  {
    e_ret_status = SFU_IMG_INIT_OK;
    TRACE("\r\n= [FWIMG] Slot #0 @: %x / Slot #1 @: %x / Swap @: %x", SFU_IMG_SLOT_0_REGION_BEGIN_VALUE,
          SFU_IMG_SLOT_1_REGION_BEGIN_VALUE, SFU_IMG_SWAP_REGION_BEGIN_VALUE);
  }

  /*
   * Sanity check: let's make sure the swap region size and the slot size is correct with regards to the swap procedure
   * constraints.
   * The swap procedure cannot succeed if the trailer info size is bigger than what a chunk used for swapping can carry.
   */
  if (((int32_t)(SFU_IMG_CHUNK_SIZE - (TRAILER_INDEX * sizeof(SFU_LL_FLASH_write_t)))) < 0)
  {
    e_ret_status = SFU_IMG_INIT_SWAP_SETTINGS_ERROR;
    TRACE("\r\n= [FWIMG] %d bytes required for the swap metadata is too much, please tune your settings",
          (TRAILER_INDEX * sizeof(SFU_LL_FLASH_write_t)));
  } /* else the swap settings are fine from a metadata size perspective */

  /*
   * Sanity check: let's make sure the swap region size and the slot size is correct with regards to the swap procedure
   * constraints.
   * The swap region size must be a multiple of the chunks size used to do the swap.
   */
#if defined(__GNUC__)
  __IO uint32_t swap_size = SFU_IMG_SWAP_REGION_SIZE, swap_chunk = SFU_IMG_CHUNK_SIZE;
  if (0U != ((uint32_t)(swap_size % swap_chunk)))
#else
  if (0U != ((uint32_t)(SFU_IMG_SWAP_REGION_SIZE % SFU_IMG_CHUNK_SIZE)))
#endif /* __GNUC__ */
  {
    e_ret_status = SFU_IMG_INIT_SWAP_SETTINGS_ERROR;
    TRACE("\r\n= [FWIMG] The swap procedure uses chunks of %d bytes but the swap region size (%d) is not a multiple",
          SFU_IMG_CHUNK_SIZE, SFU_IMG_SWAP_REGION_SIZE);
  } /* else the swap settings are fine from a chunk size perspective */

  /*
   * Sanity check: let's make sure the chunks used for the swap procedure are big enough with regards to the offset
   * giving the start @ of the firmware
   */
  if (((int32_t)(SFU_IMG_CHUNK_SIZE - SFU_IMG_IMAGE_OFFSET)) < 0)
  {
    e_ret_status = SFU_IMG_INIT_SWAP_SETTINGS_ERROR;
    TRACE("\r\n= [FWIMG] The swap procedure uses chunks of %d bytes but the firmware start offset is %d bytes",
          SFU_IMG_CHUNK_SIZE, SFU_IMG_IMAGE_OFFSET);
  } /* else the swap settings are fine from a firmware start offset perspective */
  /*
   * Sanity check: let's make sure all slots are properly aligned with regards to flash constraints
   */
  if (!IS_ALIGNED(SFU_IMG_SLOT_0_REGION_BEGIN_VALUE))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] slot 0 (%x) is not properly aligned: please tune your settings",
          SFU_IMG_SLOT_0_REGION_BEGIN_VALUE);
  } /* else slot 0 is properly aligned */

  if (!IS_ALIGNED(SFU_IMG_SLOT_1_REGION_BEGIN_VALUE))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] slot 1 (%x) is not properly aligned: please tune your settings",
          SFU_IMG_SLOT_1_REGION_BEGIN_VALUE);
  } /* else slot 1 is properly aligned */
  if (!IS_ALIGNED(SFU_IMG_SWAP_REGION_BEGIN_VALUE))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] swap region (%x) is not properly aligned: please tune your settings",
          SFU_IMG_SWAP_REGION_BEGIN_VALUE);
  } /* else swap region is properly aligned */

  /*
   * Sanity check: let's make sure the MAGIC patterns used by the internal algorithms match the FLASH constraints.
   */
  if (0 != (uint32_t)(MAGIC_LENGTH % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] magic size (%d) is not matching the FLASH constraints", MAGIC_LENGTH);
  } /* else the MAGIC patterns size is fine with regards to FLASH constraints */

  /*
   * Sanity check: let's make sure the Firmware Header Length is fine with regards to FLASH constraints
   */
  if (0 != (uint32_t)(SE_FW_HEADER_TOT_LEN % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    /* The code writing the FW header in FLASH requires the FW Header length to match the FLASH constraints */
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] FW Header size (%d) is not matching the FLASH constraints", SE_FW_HEADER_TOT_LEN);
  } /* else the FW Header Length is fine with regards to FLASH constraints */

  /*
   * Sanity check: let's make sure the chunks used for decrypt match the FLASH constraints
   */
  if (0 != (uint32_t)(SFU_IMG_CHUNK_SIZE % (uint32_t)sizeof(SFU_LL_FLASH_write_t)))
  {
    /* The size of the chunks used to store the decrypted data must be a multiple of the FLASH write length */
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] Decrypt chunk size (%d) is not matching the FLASH constraints", SFU_IMG_CHUNK_SIZE);
  } /* else the decrypt chunk size is fine with regards to FLASH constraints */

  /*
   * Sanity check: let's make sure the chunk size used for decrypt is fine with regards to AES CBC constraints.
   *               This block alignment constraint does not exist for AES GCM but we do not want to specialize the code
   *               for a specific crypto scheme.
   */
  if (0 != (uint32_t)((uint32_t)SFU_IMG_CHUNK_SIZE % (uint32_t)AES_BLOCK_SIZE))
  {
    /* For AES CBC block encryption/decryption the chunks must be aligned on the AES block size */
    e_ret_status = SFU_IMG_INIT_CRYPTO_CONSTRAINTS_ERROR;
    TRACE("\r\n= [FWIMG] Chunk size (%d) is not matching the AES CBC constraints", SFU_IMG_CHUNK_SIZE);
  }
  /*
   * Sanity check: let's make sure the slot 0 does not overlap SB code area protected by WRP)
   */
  if (((SFU_IMG_SLOT_0_REGION_BEGIN_VALUE - FLASH_BASE) / FLASH_PAGE_SIZE) <= SFU_PROTECT_WRP_PAGE_END_1)
  {
    TRACE("\r\n= [FWIMG] SLOT 0 overlaps SBSFU code area protected by WRP\r\n");
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
  }

  /*
   * Sanity check: let's make sure the slot 1 does not overlap SB code area protected by WRP)
   */
  if (((SFU_IMG_SLOT_1_REGION_BEGIN_VALUE - FLASH_BASE) / FLASH_PAGE_SIZE) <= SFU_PROTECT_WRP_PAGE_END_1)
  {
    TRACE("\r\n= [FWIMG] SLOT 1 overlaps SBSFU code area protected by WRP\r\n");
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
  }

  /*
   * Sanity check: let's make sure the swap area does not overlap SB code area protected by WRP)
   */
  if (((SFU_IMG_SWAP_REGION_BEGIN_VALUE - FLASH_BASE) / FLASH_PAGE_SIZE) <= SFU_PROTECT_WRP_PAGE_END_1)
  {
    TRACE("\r\n= [FWIMG] SWAP overlaps SBSFU code area protected by WRP\r\n");
    e_ret_status = SFU_IMG_INIT_FLASH_CONSTRAINTS_ERROR;
  }


  /* FWIMG core initialization completed */
  return e_ret_status;
}


/**
  * @brief  DeInitialize the SFU APP.
  * @param  None
  * @note   Not used in Alpha version.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_CoreDeInit(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  /*
   *  To Be Reviewed after Alpha.
  */
  return e_ret_status;
}


/**
  * @brief  Check that the FW in slot #0 has been tagged as valid by the bootloader.
  *         This function populates the FWIMG module variable: 'fw_header_validated'
  * @param  None.
  * @note It is up to the caller to make sure that slot#0 contains a valid firmware first.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus  SFU_IMG_CheckSlot0FwValid()
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint8_t fw_header_slot[FW_INFO_TOT_LEN + VALID_SIZE];

  /*
   * It is up to the caller to make sure that slot#0 contains a valid firmware before calling this function.
   * Extract the header.
   */
  e_ret_status = SFU_LL_FLASH_Read(fw_header_slot, (uint8_t *) SFU_IMG_SLOT_0_REGION_BEGIN, sizeof(fw_header_slot));

  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Check the header is tagged as VALID */
    e_ret_status = CheckHeaderValidated(fw_header_slot);
  }

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Populating the FWIMG module variable with the header */
    e_ret_status = SFU_LL_FLASH_Read(fw_header_validated, (uint8_t *) SFU_IMG_SLOT_0_REGION_BEGIN,
                                     sizeof(fw_header_validated));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  }

  return (e_ret_status);
}


/**
  * @brief  Verify Image Header in the slot given as a parameter
  * @param  pFwImageHeader pointer to a structure to handle the header info (filled by this function)
  * @param  SlotNumber slot #0 = active Firmware , slot #1 = downloaded image or backed-up image, slot #2 = swap region
  * @note   Not used in Alpha version -
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_GetFWInfoMAC(SE_FwRawHeaderTypeDef *pFwImageHeader, uint32_t SlotNumber)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint8_t *pbuffer;
  uint8_t  buffer[FW_INFO_TOT_LEN];
  if ((pFwImageHeader == NULL) || (SlotNumber > (SFU_SLOTS - 1)))
  {
    return SFU_ERROR;
  }
  pbuffer = (uint8_t *) SlotHeaderAddress[SlotNumber];

  /* use api read to detect possible ECC error */
  e_ret_status = SFU_LL_FLASH_Read(buffer, pbuffer, sizeof(buffer));
  if (e_ret_status == SFU_SUCCESS)
  {

    e_ret_status = VerifyFwRawHeaderTag(buffer);

    if (e_ret_status == SFU_SUCCESS)
    {
      e_ret_status = ParseFWInfo(pFwImageHeader, buffer);
    }
  }
  /*  cleaning */
  memset(buffer, 0, FW_INFO_TOT_LEN);
  return e_ret_status;
}


/**
  * @brief  Verify image signature of binary contiguous in flash
  * @param  pSeStatus pointer giving the SE status result
  * @param  pFwImageHeader pointer to fw header
  * @param  SlotNumber flash slot to check
  * @param SE_FwType: Type of Fw Image.
  *        This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_VerifyFwSignature(SE_StatusTypeDef  *pSeStatus, SE_FwRawHeaderTypeDef *pFwImageHeader,
                                          uint32_t SlotNumber, int32_t SE_FwType)
{
  uint8_t *pbuffer;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  /*  put it OK, to discriminate error in SFU FWIMG parts */
  *pSeStatus = SE_OK;

  /* Check the parameters value */
  if ((pFwImageHeader == NULL) || (SlotNumber > (SFU_SLOTS - 1)))
  {
    return SFU_ERROR;
  }
  if ((SE_FwType != SE_FW_IMAGE_PARTIAL) && (SE_FwType != SE_FW_IMAGE_COMPLETE))
  {
    return SFU_ERROR;
  }

  pbuffer = (uint8_t *)(SlotHeaderAddress[SlotNumber] + SFU_IMG_IMAGE_OFFSET);


  /* Signature Verification */
  e_ret_status = VerifyTag(pSeStatus, pFwImageHeader, (uint8_t *) pbuffer, SE_FwType);

  return e_ret_status;
}


/**
  * @brief  Write a valid header in slot #0
  * @param  pHeader Address of the header to be installed in slot #0
  * @retval SFU_ SUCCESS if valid, a SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_WriteHeaderValidated(uint8_t *pHeader)
{
  /*  get header from counter area  */
  SFU_ErrorStatus e_ret_status;
  SFU_FLASH_StatusTypeDef flash_if_status;
  /* The VALID magic is made of: 3*active FW header */
  uint8_t  FWInfoInput[FW_INFO_TOT_LEN + VALID_SIZE];
  uint8_t *pDestFWInfoInput;
  uint8_t *pSrcFWInfoInput;
  uint32_t i;
  uint32_t j;

  e_ret_status =  SFU_LL_FLASH_Read(FWInfoInput, pHeader,  FW_INFO_TOT_LEN);
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
  /*  compute Validated Header */
  if (e_ret_status == SFU_SUCCESS)
  {
    for (i = 0U, pDestFWInfoInput = FWInfoInput + FW_INFO_TOT_LEN; i < 3U; i++)
    {
      for (j = 0U, pSrcFWInfoInput = FWInfoInput + FW_INFO_TOT_LEN - FW_INFO_MAC_LEN; j < MAGIC_LENGTH; j++)
      {
        *pDestFWInfoInput = *pSrcFWInfoInput;
        pDestFWInfoInput++;
        pSrcFWInfoInput++;
      }
    }
    /*  write in flash  */
    e_ret_status = SFU_LL_FLASH_Write(&flash_if_status, SFU_IMG_SLOT_0_REGION_BEGIN, FWInfoInput, sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_WRITE_FAILED);
  }
  return e_ret_status;
}

/**
  * @brief Verifies the validity of a slot.
  * @note Control if there is no additional code beyond the firmware image (malicious SW).
  * @param pSlotBegin Start address of a slot.
  * @param uSlotSize Size of a slot.
  * @param uFwSize Size of the firmware image.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, error code otherwise
  */
SFU_ErrorStatus SFU_IMG_VerifySlot(uint8_t *pSlotBegin, uint32_t uSlotSize, uint32_t uFwSize)
{
  uint32_t i;
  uint8_t *pAddress;
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;

  /* Set dimension to the appropriate length for FLASH programming.
   * Example: 64-bit length for L4.
   */
  if ((uFwSize % (uint32_t)sizeof(SFU_LL_FLASH_write_t)) != 0)
  {
    uFwSize = uFwSize + ((uint32_t)sizeof(SFU_LL_FLASH_write_t) - (uFwSize % (uint32_t)sizeof(SFU_LL_FLASH_write_t)));
  }

  for (i = 0, pAddress  = pSlotBegin + SFU_IMG_IMAGE_OFFSET + uFwSize; i < (uSlotSize -
                                                                            (uFwSize + SFU_IMG_IMAGE_OFFSET)); i++)
  {
    if ((pAddress[i] != 0x00) && (pAddress[i] != 0xFF))
    {
      e_ret_status = SFU_ERROR;
    }
  }

  return (e_ret_status);
}

/**
  * @brief  check trailer validity to allow resume installation.
  *         This function populates the FWIMG module variables: 'fw_header_validated'
  *         'fw_image_header_validated', 'fw_header_to_test' and 'fw_image_header_to_test'
  *         in case of valid trailer detection.
  *         This function does not modify flash content.
  * @param  None.
  * @retval SFU_SUCCESS if successful, a SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus  SFU_IMG_CheckTrailerValid(void)
{
  SFU_ErrorStatus e_ret_status;
  SFU_ErrorStatus hasActiveFw;
  uint8_t FWInfoInput[FW_INFO_TOT_LEN];
  uint8_t FWInfoValid[FW_INFO_TOT_LEN + VALID_SIZE];

  /* check trailer Magic */
  e_ret_status = CHECK_TRAILER_MAGIC;
  if (e_ret_status != SFU_SUCCESS)
  {
    /* Trailer Magic is not present: no resume possible */
    return SFU_ERROR;
  }

  /* Trailer magic is present: trailer validity has to be checked prior allowing the resume procedure */

  /* Read header in slot #0 if any */
  e_ret_status = SFU_LL_FLASH_Read(FWInfoValid, SLOT_0_HDR, sizeof(FWInfoValid));
  StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);

  if (e_ret_status == SFU_SUCCESS)
  {
    /* Check header test field in trailer is signed */
    e_ret_status = VerifyFwRawHeaderTag(FWInfoValid);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    /* Check header in slot #0 is validated */
    e_ret_status = CheckHeaderValidated(FWInfoValid);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    /* Header in slot #0 is signed and validated, in protected area: it can be trusted. */

    /* Populate global variables: fw_header_validated and fw_image_header_validated */
    memcpy(fw_header_validated, FWInfoValid, sizeof(fw_header_validated));
    ParseFWInfo(&fw_image_header_validated, fw_header_validated);
    hasActiveFw = SFU_SUCCESS;

    /* Read header valid field in trailer */
    e_ret_status = SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_VALID, sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Check if header in slot #0 is same as header valid in trailer */
      if (memcmp(FWInfoInput, fw_header_validated, sizeof(FWInfoInput)) != 0)
      {
        /*
         * Header in slot #0 is not identical to header valid field:
         * Either install is already complete (if header in slot #0 is same as header test field in trailer),
         * or there is hack tentative.
         * In both cases, resume install must not be triggered.
         */
        e_ret_status = SFU_ERROR;
      }
    }
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Read header test field in trailer */
      e_ret_status = SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_TEST, sizeof(FWInfoInput));
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
    }
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Check header test field in trailer is signed */
      e_ret_status = VerifyFwRawHeaderTag(FWInfoInput);
    }
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Populate global variables: fw_header_to_test and fw_image_header_to_test */
      memcpy(fw_header_to_test, FWInfoInput, sizeof(fw_header_to_test));
      ParseFWInfo(&fw_image_header_to_test, fw_header_to_test);

      /* Check candidate image version */
      e_ret_status = SFU_IMG_CheckFwVersion((int32_t)fw_image_header_validated.FwVersion,
                                            (int32_t)fw_image_header_to_test.FwVersion);
    }

    /* At this point, if (e_ret_status == SFU_SUCCESS) then resume of installation is allowed */
  }
  else
  {
    /*
     * Header in slot #0 is not valid.
     * It is considered this can not be hack attempt, because header in slot #0 is in
     * protected area.
     * Possible reasons:
     * - Swap has been interrupted during very first image installation (slot #0 was empty)
     * - Swap has been interrupted during reconstitution of first sector of slot #0
     * If header test field in trailer is signed and with proper version, we resume install.
     */

    /* Read header test field in trailer */
    e_ret_status = SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_TEST, sizeof(FWInfoInput));
    StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Check header test field in trailer is signed */
      e_ret_status = VerifyFwRawHeaderTag(FWInfoInput);
    }
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Populate global variables: fw_header_to_test and fw_image_header_to_test */
      memcpy(fw_header_to_test, FWInfoInput, sizeof(fw_header_to_test));
      ParseFWInfo(&fw_image_header_to_test, fw_header_to_test);

      /* Read header valid field in trailer */
      e_ret_status = SFU_LL_FLASH_Read(FWInfoInput, TRAILER_HDR_VALID, sizeof(FWInfoInput));
      StatusFWIMG(e_ret_status == SFU_ERROR, SFU_IMG_FLASH_READ_FAILED);
    }
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Check header valid field in trailer is signed */
      hasActiveFw = VerifyFwRawHeaderTag(FWInfoInput);

      if (hasActiveFw == SFU_SUCCESS)
      {
        /* slot #0 was containing active image */

        /* Populate global variables: fw_header_validated and fw_image_header_validated */
        memcpy(fw_header_validated, FWInfoInput, sizeof(fw_header_validated));
        ParseFWInfo(&fw_image_header_validated, fw_header_validated);
      }
      else
      {
        /* slot #0 was empty or was containing bricked Fw image */

        /*
         * Global variables keep their default init value (important):
         * - fw_header_validated = {0} => CurrentFwVersion is considered as 0
         * - fw_image_header_validated = {0xFE} => MagicTrailerValue is not 0, so that it can be cleaned
         */
      }

      /* Check candidate image version */
      e_ret_status = SFU_IMG_CheckFwVersion((int32_t)fw_image_header_validated.FwVersion,
                                            (int32_t)fw_image_header_to_test.FwVersion);
    }

    /* At this point, if (e_ret_status == SFU_SUCCESS) then resume of installation is allowed */
  }

  return e_ret_status;
}

/**
  * @brief  Resume from an interrupted FW installation
  *         Relies on following global in ram, already filled by the check:
  *         fw_header_validated, fw_header_to_test, fw_image_header_validated, fw_image_header_to_test
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_Resume(void)
{
  SFU_ErrorStatus e_ret_status;

  /* resume swap */
  e_ret_status = SwapFirmwareImages();

  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_SWAP);
    return e_ret_status;
  }

  /* validate immediately the new active FW */
  e_ret_status = SFU_IMG_Validation(TRAILER_HDR_TEST); /* the header of the newly installed FW is in the trailer */

  if (SFU_SUCCESS == e_ret_status)
  {
    /*
     * We have just installed a new Firmware so we need to reset the counters
     */

    /*
     * Consecutive init counter:
     *   a reset of this counter is useless as long as the installation is triggered by a Software Reset
     *   and this SW Reset is considered as a valid boot-up cause (because the counter has already been reset in
     *   SFU_BOOT_ManageResetSources()).
     *   If you change this behavior in SFU_BOOT_ManageResetSources() then you need to reset the consecutive init
     *   counter here.
     */

  }
  else
  {
    /*
     * Else do nothing (except logging the error cause), we will end up in local download at the next reboot.
     * By not cleaning the swap pattern we may retry this installation but we do not want to do this as we might end up
     * in an infinite loop.
     */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
  }

  /*  clear swap pattern */
  e_ret_status = CLEAN_TRAILER_MAGIC;

  if (e_ret_status != SFU_SUCCESS)
  {
    /* Memorize the error */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
  }

#if defined(SFU_VERBOSE_DEBUG_MODE)
  if (e_ret_status == SFU_SUCCESS)
  {
    TRACE("\r\n=         Resume procedure completed.");
  }
  else
  {
    TRACE("\r\n=         Resume procedure cannot be finalized!");
  }
#endif /* SFU_VERBOSE_DEBUG_MODE */

  return e_ret_status;
}

/**
  * @brief  Check that there is an Image to Install
  * @retval SFU_SUCCESS if Image can be installed, a SFU_ERROR  otherwise.
  */
SFU_ErrorStatus SFU_IMG_FirmwareToInstall(void)
{
  SFU_ErrorStatus e_ret_status = SFU_SUCCESS;
  uint8_t *pbuffer = (uint8_t *) SFU_IMG_SWAP_REGION_BEGIN;
  uint8_t fw_header_slot[FW_INFO_TOT_LEN + VALID_SIZE]; /* header + VALID tags */

  /*
    * The anti-rollback check is implemented at installation stage (SFU_IMG_InstallNewVersion)
    * to be able to handle a specific error cause.
    */

  /*  Loading the header to verify it */
  e_ret_status = SFU_LL_FLASH_Read(fw_header_to_test, pbuffer, sizeof(fw_header_to_test));
  if (e_ret_status == SFU_SUCCESS)
  {
    /*  check swap header */
    e_ret_status = SFU_IMG_GetFWInfoMAC(&fw_image_header_to_test, 2);
  }
  if (e_ret_status == SFU_SUCCESS)
  {
    /*  compare the header in slot 1 with the header in swap */
    pbuffer = (uint8_t *) SFU_IMG_SLOT_1_REGION_BEGIN;
    e_ret_status = SFU_LL_FLASH_Read(fw_header_slot, pbuffer, sizeof(fw_header_slot));
    if (e_ret_status == SFU_SUCCESS)
    {
      /* image header in slot 1 not consistent with swap header */
      uint32_t trailer_begin = (uint32_t) TRAILER_BEGIN;
      uint32_t end_of_test_image = ((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + fw_image_header_to_test.FwSize +  // fw_image_header_to_test => swap header
                                    SFU_IMG_IMAGE_OFFSET);
      uint32_t end_of_valid_image = ((uint32_t)SFU_IMG_SLOT_1_REGION_BEGIN + fw_image_header_validated.FwSize +
                                     SFU_IMG_IMAGE_OFFSET);
      /* the header in swap must be the same as the header in slot #1 */
      int32_t ret = memcmp(fw_header_slot, fw_header_to_test, sizeof(fw_header_to_test)); /* compare the header length*/
      /*
        * The image to install must not be already validated otherwise this means we try to install a backed-up FW
        * image.
        * The only case to re-install a backed-up image is the rollback use-case, not the new image installation
        * use-case.
        */
      e_ret_status = CheckHeaderValidated(fw_header_slot);
      /* Check if there is enough room for the trailers */
      if ((trailer_begin < end_of_test_image) || (trailer_begin < end_of_valid_image) || (ret) 
          || (SFU_SUCCESS == e_ret_status))
      {
        /*
          * These error causes are not memorized in the BootInfo area because there won't be any error handling
          * procedure.
          * If this function returns that no new firmware can be installed (as this may be a normal case).
          */
        e_ret_status = SFU_ERROR;

#if defined(SFU_VERBOSE_DEBUG_MODE)
        /* Display the debug message(s) only once */
        if (1U == initialDeviceStatusCheck)
        {
          if ((trailer_begin < end_of_test_image) || (trailer_begin < end_of_valid_image))
          {
            TRACE("\r\n= [FWIMG] The binary image to be installed and/or the image to be backed-up overlap with the");
            TRACE("\r\n          trailer area!");
          } /* check next error cause */

          if (SFU_SUCCESS == e_ret_status)
          {
            TRACE("\r\n= [FWIMG] The binary image to be installed is already tagged as VALID!");
          } /* check next error cause */

          if (ret)
          {
            TRACE("\r\n= [FWIMG] The headers in slot #1 and swap area do not match!");
          } /* no more error cause to check */
        } /* else do not print the message(s) again */
#endif /* SFU_VERBOSE_DEBUG_MODE */
      }
      else
      {
        e_ret_status = SFU_SUCCESS;
      }
    }
  }
  return e_ret_status;
}


/**
  * @brief Prepares the Candidate FW image for Installation.
  *        This stage depends on the supported features: in this example this consists in decrypting the candidate
  *        image.
  * @note This function relies on following FWIMG module ram variables, already filled by the checks:
  *       fw_header_to_test,fw_image_header_validated, fw_image_header_to_test
  * @note Even if the Firmware Image is in clear format the decrypt function is called.
  *       But, in this case no decrypt is performed, it is only a set of copy operations
  *       to organize the Firmware Image in FLASH as expected by the swap procedure.
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_PrepareCandidateImageForInstall(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;
  SE_StatusTypeDef e_se_status;

  /*
    * Control if there is no additional code beyond the firmware image (malicious SW)
    */
  e_ret_status = SFU_IMG_VerifySlot(SFU_IMG_SLOT_1_REGION_BEGIN, SFU_IMG_SLOT_1_REGION_SIZE,
                                    fw_image_header_to_test.PartialFwSize + (fw_image_header_to_test.PartialFwOffset %
                                                                             SFU_IMG_SWAP_REGION_SIZE));

  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FLASH_ERROR);
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] Additional code detected beyond FW image!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    return e_ret_status;
  }

  /*
    * Pre-condition: all checks have been performed,
    *                so all FWIMG module variables are populated.
    * Now we can decrypt in FLASH.
    *
    * Initial FLASH state (focus on slot #1 and swap area):
    *
    * Depending on partial FW offset, we can have:
    * <Slot #1>   : {Candidate Image Header + encrypted FW - page k}
    *               {encrypted FW - page k+1}
    *               .....
    *               {encrypted FW - page k+N}
    *               .....
    * </Slot #1>
    * or:
    * <Slot #1>   : {Candidate Image Header }
    *               {encrypted FW - page k}
    *               .....
    *               {encrypted FW - page k+N}
    *               .....
    * </Slot #1>
    *
    * <Swap area> : {Candidate Image Header}
    * </Swap area>
    */
  e_ret_status =  DecryptImageInSlot1(&fw_image_header_to_test);

  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_DECRYPT_FAILURE);
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] Decryption failure!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    return e_ret_status;
  }

  /*
    * At this step, the FLASH content looks like this:
    *
    * <Slot #1>   : {decrypted FW - data from page k+1 in page 0}
    *               {decrypted FW - data from page k+2 in page 1}
    *               .....
    *               {decrypted FW - data from page k+N in page N-1}
    *               {page N is now empty}
    *               .....
    * </Slot #1>
    *
    * <Swap area> : {No Header (shift) + decrypted FW data from page k}
    * </Swap area>
    *
    * The Candidate FW image has been decrypted
    * and a "hole" has been created in slot #1 to be able to swap.
    */

  e_ret_status = VerifyFwSignatureAfterDecrypt(&e_se_status, &fw_image_header_to_test);
  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_SIGNATURE_FAILURE);
#if defined(SFU_VERBOSE_DEBUG_MODE)
    TRACE("\r\n= [FWIMG] The decrypted image is incorrect!");
#endif /* SFU_VERBOSE_DEBUG_MODE */
    return e_ret_status;
  }

  /* Return the result of this preparation */
  return (e_ret_status);
}


/**
  * @brief  Install the new version
  * relies on following global in ram, already filled by the check
  * fw_header_to_test,fw_image_header_validated, fw_image_header_to_test
  * @retval SFU_SUCCESS if successful,SFU_ERROR error otherwise.
  */
SFU_ErrorStatus SFU_IMG_InstallNewVersion(void)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /*
    * At this step, the FLASH content looks like this:
    *
    * <Slot #1>   : {decrypted FW - data from page k+1 in page 0}
    *               {decrypted FW - data from page k+2 in page 1}
    *               .....
    *               {decrypted FW - data from page k+N in page N-1}
    *               {page N is now empty}
    *               .....
    * </Slot #1>
    *
    * <Swap area> : {No Header (shift) + decrypted FW data from page k}
    * </Swap area>
    *
    */

  /*  erase last block */
  e_ret_status = EraseSlotIndex(1, (TRAILER_INDEX - 1)); /* erase the size of "swap area" at the end of slot #1 */
  if (e_ret_status !=  SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FLASH_ERROR);
    return SFU_ERROR;
  }

  /*
    * At this step, the FLASH content looks like this:
    *
    * <Slot #1>   : {decrypted FW - data from page k+1 in page 0}
    *               {decrypted FW - data from page k+2 in page 1}
    *               .....
    *               {decrypted FW - data from page k+N in page N-1}
    *               {page N is now empty}
    *               .....
    *               {at least the swap area size at the end of the last page of slot #1 is empty }
    * </Slot #1>
    *
    * <Swap area> : {No Header (shift) + decrypted FW data from page 0}
    * </Swap area>
    *
    */


  /*  write trailer  */
  e_ret_status = WriteTrailerHeader(fw_header_to_test, fw_header_validated);
  /*  finish with the magic to validate */
  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
    return e_ret_status;
  }
  e_ret_status = WRITE_TRAILER_MAGIC;
  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
    return e_ret_status;
  }

  /*
    * At this step, the FLASH content looks like this:
    *
    * <Slot #1>   : {decrypted FW - data from page k+1 in page 0}
    *               {decrypted FW - data from page k+2 in page 1}
    *               .....
    *               {decrypted FW - data from page k+N in page N-1}
    *               {page N is now empty}
    *               .....
    *               {the last page of slot #1 ends with the trailer magic patterns and the free space for the trailer
    *                info}
    * </Slot #1>
    *
    * <Swap area> : {No Header (shift) + decrypted FW data from page 0}
    * </Swap area>
    *
    * The trailer magic patterns is ActiveFwHeader|CandidateFwHeader|SWAPmagic.
    * This is followed by a free space to store the trailer info (N*CPY_TO_SLOT0) and (N*CPY_TO_SLOT1):
    *
    *<---------------------------------------------- Last page of slot #1 -------------------------------------------->
    *|..|FW_INFO_TOT_LEN|FW_INFO_TOT_LEN|MAGIC_LENGTH|N*sizeof(SFU_LL_FLASH_write_t)|N*sizeof(SFU_LL_FLASH_write_t)
    *|..|header 1       |header 2       |SWAP magic  |N*CPY_TO_SLOT0                |N*CPY_TO_SLOT1
    *
    */

  /*  swap  */
  e_ret_status = SwapFirmwareImages();

  if (e_ret_status != SFU_SUCCESS)
  {
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_SWAP);
    return e_ret_status;
  }

  /* validate immediately the new active FW */
  e_ret_status = SFU_IMG_Validation(TRAILER_HDR_TEST); /* the header of the newly installed FW is in the trailer */

  if (SFU_SUCCESS == e_ret_status)
  {
    /*
      * We have just installed a new Firmware so we need to reset the counters
      */

    /*
      * Consecutive init counter:
      *   a reset of this counter is useless as long as the installation is triggered by a Software Reset
      *   and this SW Reset is considered as a valid boot-up cause (because the counter has already been reset in
      *   SFU_BOOT_ManageResetSources()).
      *   If you change this behavior in SFU_BOOT_ManageResetSources() then you need to reset the consecutive init
      *   counter here.
      */

  }
  else
  {
    /*
      * Else do nothing (except logging the error cause), we will end up in local download at the next reboot.
      * By not cleaning the swap pattern we may retry this installation but we do not want to do this as we might end
      * up in an infinite loop.
      */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
  }

  /*  clear swap pattern */
  e_ret_status = CLEAN_TRAILER_MAGIC;

  if (e_ret_status != SFU_SUCCESS)
  {
    /* Memorize the error */
    (void)SFU_BOOT_SetLastExecError(SFU_EXCPT_FWIMG_MAGIC);
  }

  return e_ret_status;   /* Leave the function now */
}

/**
  * @brief  Control FW tag
  * @note   This control will be done twice for security reasons (first control done in VerifyTagScatter)
  * @param  pTag Tag of slot#0 firmware
  * @retval SFU_SUCCESS if successful,SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_IMG_ControlFwTag(uint8_t *pTag)
{

  if (MemoryCompare(fw_tag_validated, pTag, SE_TAG_LEN) != SFU_SUCCESS)
  {
    return SFU_ERROR;
  }
  else
  {
    FLOW_STEP(uFlowCryptoValue, FLOW_STEP_INTEGRITY);
    return SFU_SUCCESS;
  }
}

/**
  * @brief  Get size of the trailer
  * @note   This area mapped at the end of a slot#1 is not available for the firmware image
  * @param  None
  * @retval Size of the trailer.
  */
uint32_t SFU_IMG_GetTrailerSize(void)
{
  return TRAILER_SIZE;
}

/**
  * @brief  Check candidate image version is allowed.
  * @param  CurrentVersion Version of currently installed image if any
  * @param  CandidateVersion Version of candidate image
  * @retval SFU_SUCCESS if candidate image version is allowed, SFU_ErrorStatus error otherwise.
  */
SFU_ErrorStatus SFU_IMG_CheckFwVersion(int32_t CurrentVersion, int32_t CandidateVersion)
{
  SFU_ErrorStatus e_ret_status = SFU_ERROR;

  /*
    * It is not allowed to install a Firmware with a lower version than the active firmware.
    * But we authorize the re-installation of the current firmware version.
    * We also check that the candidate version is at least the min. allowed version for this device.
    */

  if ((CandidateVersion >= CurrentVersion) && (CandidateVersion >= SFU_FW_VERSION_START_NUM))
  {
    /* Candidate version is allowed */
    e_ret_status = SFU_SUCCESS;
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

/**
  * @}
  */

#undef SFU_FWIMG_CORE_C

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
