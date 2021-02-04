/**
  ******************************************************************************
  * @file    se_crypto_bootloader.c
  * @author  MCD Application Team
  * @brief   Secure Engine CRYPTO module.
  *          This file provides set of firmware functions to manage SE Crypto
  *          functionalities. These services are used by the bootloader.
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
#include <string.h>               /* added for memcpy */
#include "se_crypto_bootloader.h"
#include "se_low_level.h"         /* required for assert_param */
#include "se_key.h"               /* required to access the keys when not provided as input parameter (metadata 
                                     authentication) */
#if defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#elif defined(__CC_ARM)
#include "mapping_sbsfu.h"
#endif /* __ICCARM__ || __GNUC__  */

/** @addtogroup SE Secure Engine
  * @{
  */

/** @addtogroup SE_CORE SE Core
  * @{
  */

/** @defgroup  SE_CRYPTO SE Crypto
  * @brief Crypto services (used by the bootloader and common crypto functions)
  * @{
  */

/** @defgroup  SE_CRYPTO_BOOTLOADER SE Crypto for Bootloader
  * @brief Crypto functions used by the bootloader.
  * @note In this example: \li the AES GCM crypto scheme is supported (@ref SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM),
  *                        \li the ECC DSA without encryption crypto scheme is supported
  *                        \li (@ref SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256),
  *                        \li the ECC DSA+AES-CBC crypto scheme is supported
  *                        \li (@ref SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256).
  * @{
  */

/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Variables Private Variables
  * @{
  */

#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM) )
/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Variables_Symmeric_Key Symmetric Key Handling
  *  @brief Variable(s) used to handle the symmetric key(s).
  *  @note All these variables must be located in protected memory.
  *  @{
  */

static uint8_t m_aSE_FirmwareKey[SE_SYMKEY_LEN];        /* Variable used to store the Key inside the protected area */

/**
  * @}
  */

#endif /* SECBOOT_CRYPTO_SCHEME */

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256)
/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Variables_AES_CBC AES CBC variables
  *  @brief  Advanced Encryption Standard (AES), CBC (Cipher-Block Chaining) with support for Ciphertext Stealing
  *  @note   We do not use local variable(s) because we want this to be in the protected area (and the stack is not
  *          protected),
  *          and also because the contexts are used to store internal states.
  *  @{
  */
typedef struct
{
  mbedtls_cipher_context_t mbedAesCbcCtx; /*<! mbedTLS AES cbc context                   */
} AES_CBC_CTX_t;
static AES_CBC_CTX_t m_AESCBCctx; /*!<Variable used to store the AES CBC context */
/**
  * @}
  */
#endif /* SECBOOT_CRYPTO_SCHEME */

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
typedef struct
{
  mbedtls_cipher_context_t mbedAesGcmCtx;  /*<! mbedTLS AES GCM context                           */
  uint8_t aTag[SE_TAG_LEN];                /*<! AES GCM tag to check (Firmware tag or Header tag) */
  size_t TagSize;                          /*<! AES GCM tag length                                */
} AES_GCM_CTX_t;
static AES_GCM_CTX_t m_AESGCMctx; /*!<Variable used to store the AES GCM context */
#endif /* SECBOOT_CRYPTO_SCHEME */

#if ((SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256))
/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Variables_SHA256 SHA256 variables
  *  @brief  Secure Hash Algorithm SHA-256.
  *  @note   We do not use local variable(s) because the context is used to store internal states.
  *  @{
  */
static mbedtls_sha256_context m_SHA256ctx;
/**
  * @}
  */
#endif /* SECBOOT_CRYPTO_SCHEME */



/**
  * @}
  */

/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Macros Private Macros
  * @{
  */


/**
  * @brief Clean up the RAM area storing the Firmware key.
  *        This applies only to the secret symmetric key loaded with SE_ReadKey().
  */
#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM) )

#define SE_CLEAN_UP_FW_KEY() \
  do { \
    (void)memcpy(m_aSE_FirmwareKey, (void const *)(SE_STARTUP_REGION_ROM_START + (SysTick->VAL % 0xFFF)), \
                 SE_SYMKEY_LEN); \
  } while(0)

#else

#define SE_CLEAN_UP_FW_KEY() do { /* do nothing */; } while(0)

#endif /* SECBOOT_CRYPTO_SCHEME */

/**
  * @brief Clean up the RAM area storing the ECC Public Key.
  *        This applies only to the public asymmetric key loaded with SE_ReadKey_Pub().
  */
#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )

#define SE_CLEAN_UP_PUB_KEY() \
  do { \
    (void)memcpy(m_aSE_PubKey, (void const *)(SE_STARTUP_REGION_ROM_START + (SysTick->VAL % 0xFFF)), \
                 SE_ASYM_PUBKEY_LEN); \
  } while(0)

#else

#define SE_CLEAN_UP_PUB_KEY() do { /* do nothing */; } while(0)

#endif /* SECBOOT_CRYPTO_SCHEME */

/**
  * @}
  */

/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Functions Private Functions
  *  @brief These are private functions used internally by the crypto services provided to the bootloader
  *         i.e. the exported functions implemented in this file (@ref SE_CRYPTO_BOOTLOADER_Exported_Functions).
  *  @note  These functions are not part of the common crypto code because they are used by the bootloader services
  *         only.
  * @{
  */

/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Functions_AES AES Functions
  *  @brief Helpers for AES: high level wrappers for the Cryptolib.
  *  @note This group of functions is empty because we do not provide high level wrappers for AES.
  * @{
  */

/* no high level wrappers provided for AES primitives: the Cryptolib APIs are called directly from  the services
   (@ref SE_CRYPTO_BOOTLOADER_Exported_Functions) */

/**
  * @}
  */


/** @defgroup SE_CRYPTO_BOOTLOADER_Private_Functions_HASH Hash Functions
  *  @brief Hash algorithm(s): high level wrapper(s) for the Cryptolib.
  * @{
  */


#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )

static int32_t SE_CRYPTO_SHA256_HASH_DigestCompute(const uint8_t *InputMessage, const int32_t InputMessageLength,
                                                   uint8_t *MessageDigest, int32_t *MessageDigestLength);

/**
  * @brief  SHA256 HASH digest compute example.
  * @param  InputMessage: pointer to input message to be hashed.
  * @param  InputMessageLength: input data message length in byte.
  * @param  MessageDigest: pointer to output parameter that will handle message digest
  * @param  MessageDigestLength: pointer to output digest length.
  * @retval error status: 0 if success
  */
static int32_t SE_CRYPTO_SHA256_HASH_DigestCompute(const uint8_t *InputMessage, const int32_t InputMessageLength,
                                                   uint8_t *MessageDigest, int32_t *MessageDigestLength)
{
  int32_t ret;
  int32_t error_status = -1;

  /*
   * InputMessageLength is never negative (see SE_CRYPTO_Authenticate_Metadata)
   * so the cast to size_t is valid
   */

  /* init of a local sha256 context */
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);

  ret = mbedtls_sha256_starts_ret(&ctx, 0);   /* 0 for sha256 */

  if (0 == ret)
  {
    ret = mbedtls_sha256_update_ret(&ctx, InputMessage, (size_t)InputMessageLength); /* casting is fine because size_t
                                                                                        is unsigned int in ARM C */

    if (0 == ret)
    {
      ret = mbedtls_sha256_finish_ret(&ctx, MessageDigest);

      if (0 == ret)
      {
        error_status = 0;
        *MessageDigestLength = 32; /* sha256 */
      }
      else
      {
        *MessageDigestLength = 0;
      }
    }
  }

  mbedtls_sha256_free(&ctx);

  return error_status;
}


#endif /* SECBOOT_CRYPTO_SCHEME */

/**
  * @}
  */

/**
  * @}
  */


/** @defgroup SE_CRYPTO_BOOTLOADER_Exported_Functions Exported Functions
  * @brief The implementation of these functions is crypto-dependent (but the API is crypto-agnostic).
  *        These functions use the generic SE_FwRawHeaderTypeDef structure to fill crypto specific structures.
  * @{
  */

/**
  * @brief Secure Engine Encrypt Init function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  * @param pxSE_Metadata: Firmware metadata.
  * @param SE_FwType: Type of Fw Image.
  *        This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Encrypt_Init(SE_FwRawHeaderTypeDef *pxSE_Metadata, int32_t SE_FwType)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  int32_t ret;
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /* The bootloader does not need the encrypt service in this crypto scheme: reject this request later on */
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM */

  UNUSED(SE_FwType);

  /* Check the pointers allocation */
  if (pxSE_Metadata == NULL)
  {
    return e_ret_status;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* Read the Symmetric Key */
  SE_ReadKey(&(m_aSE_FirmwareKey[0]));

  mbedtls_cipher_init(&(m_AESGCMctx.mbedAesGcmCtx));
  ret = mbedtls_cipher_setup(&(m_AESGCMctx.mbedAesGcmCtx), mbedtls_cipher_info_from_values(MBEDTLS_CIPHER_ID_AES,
                             SE_SYMKEY_LEN * 8, MBEDTLS_MODE_GCM));
  ret += mbedtls_cipher_setkey(&(m_AESGCMctx.mbedAesGcmCtx), m_aSE_FirmwareKey, SE_SYMKEY_LEN * 8, MBEDTLS_ENCRYPT);
  ret += mbedtls_cipher_set_iv(&(m_AESGCMctx.mbedAesGcmCtx), pxSE_Metadata->Nonce, SE_NONCE_LEN);
  ret += mbedtls_cipher_reset(&(m_AESGCMctx.mbedAesGcmCtx));
  ret += mbedtls_cipher_update_ad(&(m_AESGCMctx.mbedAesGcmCtx), NULL, 0); /* required for mbedtls_gcm_starts */

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));
  }

#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /* The bootloader does not need the encrypt service in this crypto scheme: reject this request */
  e_ret_status = SE_ERROR;
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine Header Append function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  * @param pInputBuffer: pointer to Input Buffer.
  * @param InputSize: Input Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Header_Append(const uint8_t *pInputBuffer, int32_t InputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  int32_t ret; /* mbedTLS return code */
#endif /* SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM */

  /* Check the pointers allocation */
  if (pInputBuffer == NULL)
  {
    return e_ret_status;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  if (InputSize < 0)
  {
    return (e_ret_status);
  }
  /* else the cast to size_t is valid */

  /*
   * mbedTLS : AAD (Additional Authenticated Data).
   * In the SBSFU context this is for the Firmware Header authentication.
   */
  ret = mbedtls_cipher_update_ad(&(m_AESGCMctx.mbedAesGcmCtx), pInputBuffer, (size_t)InputSize);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /* The bootloader does not need this service in this crypto scheme: reject this request */
  e_ret_status = SE_ERROR;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(InputSize);
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256 || SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256 */
  /* Return status*/
  return e_ret_status;
} /* SECBOOT_CRYPTO_SCHEME */

/**
  * @brief Secure Engine Encrypt Append function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  * @param pInputBuffer: pointer to Input Buffer.
  * @param InputSize: Input Size (bytes).
  * @param pOutputBuffer: pointer to Output Buffer.
  * @param pOutputSize: pointer to Output Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Encrypt_Append(const uint8_t *pInputBuffer, int32_t InputSize, uint8_t *pOutputBuffer,
                                        int32_t *pOutputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  int32_t ret; /* mbedTLS return code */
#endif /* SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM */

  /* Check the pointers allocation */
  if ((pInputBuffer == NULL) || (pOutputBuffer == NULL) || (pOutputSize == NULL))
  {
    return e_ret_status;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  if (InputSize < 0)
  {
    return (e_ret_status);
  }
  /* else the cast to size_t is valid */

  /* The cast "(size_t *)pOutputSize" is valid because we should not get more bytes than InputSize so this fits in an
     int32_t */
  ret = mbedtls_cipher_update(&(m_AESGCMctx.mbedAesGcmCtx), pInputBuffer, (size_t)InputSize, pOutputBuffer,
                              (size_t *)pOutputSize);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /* The bootloader does not need the encrypt service in this crypto scheme: reject this request */
  e_ret_status = SE_ERROR;

  /* Prevent unused argument(s) compilation warning */
  UNUSED(InputSize);
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256 || SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256 */

  /* Return status*/
  return e_ret_status;
} /* SECBOOT_CRYPTO_SCHEME */

/**
  * @brief Secure Engine Encrypt Finish function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  * @param pOutputBuffer: pointer to Output Buffer.
  * @param pOutputSize: pointer to Output Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Encrypt_Finish(uint8_t *pOutputBuffer, int32_t *pOutputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  int32_t ret; /* mbedTLS return code */
#endif /* SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM */

  /* Check the pointers allocation */
  if ((pOutputBuffer == NULL) || (pOutputSize == NULL))
  {
    /* Clean-up the key in RAM */
    SE_CLEAN_UP_FW_KEY();

    return e_ret_status;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* mbedTLS : the cast to (size_t *) is valid because we expect the value to be 0 (checked afterwards) */
  ret = mbedtls_cipher_finish(&(m_AESGCMctx.mbedAesGcmCtx), pOutputBuffer, (size_t *)pOutputSize);

  if (0 != *pOutputSize)
  {
    ret = -1;
  }
  else
  {
    /* Write GCM tag */
    ret = mbedtls_cipher_write_tag(&(m_AESGCMctx.mbedAesGcmCtx), pOutputBuffer, SE_TAG_LEN);
  }

  mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));

  if (0 == ret)
  {
    *pOutputSize = SE_TAG_LEN;
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    /* authentication failure */
    *pOutputSize = 0;
  }

#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /* The bootloader does not need the encrypt service in this crypto scheme: reject this request */
  e_ret_status = SE_ERROR;
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Clean-up the key in RAM */
  SE_CLEAN_UP_FW_KEY();

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine Decrypt Init function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  * @param pxSE_Metadata: Firmware metadata.
  * @param SE_FwType: Type of Fw Image.
  *        This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Decrypt_Init(SE_FwRawHeaderTypeDef *pxSE_Metadata, int32_t SE_FwType)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM) )
  int32_t ret; /* mbedTLS return code */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* Variables to handle partial/complete FW image */
  uint8_t *fw_tag;
#endif /* SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM */
#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)
  /*
   * In this crypto scheme the Firmware is not encrypted.
   * The Decrypt operation is called anyhow before installing the firmware.
   * Indeed, it allows moving the Firmware image blocks in FLASH.
   * These moves are mandatory to create the appropriate mapping in FLASH
   * allowing the swap procedure to run without using the swap area at each and every move.
   *
   * See in SB_SFU project: @ref SFU_IMG_PrepareCandidateImageForInstall.
   */
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)
  UNUSED(SE_FwType);
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Check the pointers allocation */
  if (pxSE_Metadata == NULL)
  {
    return SE_ERROR;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* Check the parameters value and set fw_size and fw_tag to check */
  if (SE_FwType == SE_FW_IMAGE_COMPLETE)
  {
    fw_tag = pxSE_Metadata->FwTag;
  }
  else if (SE_FwType == SE_FW_IMAGE_PARTIAL)
  {
    fw_tag = pxSE_Metadata->PartialFwTag;
  }
  else
  {
    return SE_ERROR;
  }

  /* Read the Symmetric Key */
  SE_ReadKey(&(m_aSE_FirmwareKey[0]));

  /* mbedTLS */
  mbedtls_cipher_init(&(m_AESGCMctx.mbedAesGcmCtx));
  ret = mbedtls_cipher_setup(&(m_AESGCMctx.mbedAesGcmCtx), mbedtls_cipher_info_from_values(MBEDTLS_CIPHER_ID_AES,
                             SE_SYMKEY_LEN * 8, MBEDTLS_MODE_GCM));
  ret += mbedtls_cipher_setkey(&(m_AESGCMctx.mbedAesGcmCtx), m_aSE_FirmwareKey, SE_SYMKEY_LEN * 8, MBEDTLS_DECRYPT);
  ret += mbedtls_cipher_set_iv(&(m_AESGCMctx.mbedAesGcmCtx), pxSE_Metadata->Nonce, SE_NONCE_LEN);
  ret += mbedtls_cipher_reset(&(m_AESGCMctx.mbedAesGcmCtx));
  ret += mbedtls_cipher_update_ad(&(m_AESGCMctx.mbedAesGcmCtx), NULL, 0); /* required for mbedtls_gcm_starts */

  if (0 == ret)
  {
    /* save the GCM tag too : this is the firmware tag (we will authenticate the firmware at the end of the decrypt 
       operation) */
    memcpy(m_AESGCMctx.aTag, (uint8_t *)fw_tag, SE_TAG_LEN);
    m_AESGCMctx.TagSize = (size_t)SE_TAG_LEN;

    e_ret_status = SE_SUCCESS;
  }
  else
  {
    mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));
  }

#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256)

  /* Read the Symmetric Key */
  SE_ReadKey(&(m_aSE_FirmwareKey[0]));

  /* mbedTLS */
  mbedtls_cipher_init(&(m_AESCBCctx.mbedAesCbcCtx));
  ret = mbedtls_cipher_setup(&(m_AESCBCctx.mbedAesCbcCtx), mbedtls_cipher_info_from_values(MBEDTLS_CIPHER_ID_AES,
                             SE_SYMKEY_LEN * 8, MBEDTLS_MODE_CBC));
  ret += mbedtls_cipher_setkey(&(m_AESCBCctx.mbedAesCbcCtx), m_aSE_FirmwareKey, SE_SYMKEY_LEN * 8, MBEDTLS_DECRYPT);
  ret += mbedtls_cipher_set_iv(&(m_AESCBCctx.mbedAesCbcCtx), pxSE_Metadata->InitVector, SE_IV_LEN);
  ret += mbedtls_cipher_set_padding_mode(&(m_AESCBCctx.mbedAesCbcCtx), MBEDTLS_PADDING_NONE);
  ret += mbedtls_cipher_reset(&(m_AESCBCctx.mbedAesCbcCtx));

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    mbedtls_cipher_free(&(m_AESCBCctx.mbedAesCbcCtx));
  }

#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)
  /* Nothing to do as we won't decrypt anything */
  e_ret_status = SE_SUCCESS;
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine Decrypt Append function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  * @param pInputBuffer: pointer to Input Buffer.
  * @param InputSize: Input Size (bytes).
  * @param pOutputBuffer: pointer to Output Buffer.
  * @param pOutputSize: pointer to Output Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Decrypt_Append(const uint8_t *pInputBuffer, int32_t InputSize, uint8_t *pOutputBuffer,
                                        int32_t *pOutputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM) )
  int32_t ret; /* mbedTLS return code */
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* DecryptImageInSlot1() always starts by calling the Decrypt service with a 0 byte buffer */
  if (0 == InputSize)
  {
    /* Nothing to do but we must return a success for the decrypt operation to continue */
    return (SE_SUCCESS);
  }

  /* Check the pointers allocation */
  if ((pInputBuffer == NULL) || (pOutputBuffer == NULL) || (pOutputSize == NULL))
  {
    return SE_ERROR;
  }

  if (InputSize < 0)
  {
    *pOutputSize = 0;
    return (SE_ERROR);
  }
  /* else casting to size_t is valid */

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* mbedTLS : casting to (size_t *) is fine here because the length cannot be more than InputSize so no risk of
     overflow */
  ret = mbedtls_cipher_update(&(m_AESGCMctx.mbedAesGcmCtx), pInputBuffer, (size_t)InputSize, pOutputBuffer,
                              (size_t *)pOutputSize);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }

#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256)
  /*
   * standard mbedTLS call for AES CBC decrypt : the cast to (size_t *) is valid because the size cannot be more than
   * InputSize so no risk of overflow.
   * The buffer is provided to mbedTLS without checking the AES block alignment: it is up to the caller to make sure
   * the InputSize is a multiple of the AES block (16 bytes).
   */
  ret = mbedtls_cipher_update(&(m_AESCBCctx.mbedAesCbcCtx), pInputBuffer, (size_t)InputSize, pOutputBuffer,
                              (size_t *)pOutputSize);

  if (ret == 0)
  {
    e_ret_status = SE_SUCCESS;
  }

#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)
  /*
   * The firmware is not encrypted.
   * The only thing we need to do is to recopy the input buffer in the output buffer
   */
  (void)memcpy(pOutputBuffer, pInputBuffer, (uint32_t)InputSize);
  *pOutputSize = InputSize;
  e_ret_status = SE_SUCCESS;
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine Decrypt Finish function.
  *        It is a wrapper of the mbedTLS crypto function (mbedtls_cipher API) included in the protected area.
  *        This parameter can be a value of @ref SE_Status_Structure_definition.
  * @param pOutputBuffer: pointer to Output Buffer.
  * @param puOutputSize: pointer to Output Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Decrypt_Finish(uint8_t *pOutputBuffer, int32_t *pOutputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM) )
  int32_t ret; /* mbedTLS return code */
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Check the pointers allocation */
  if ((pOutputBuffer == NULL) || (pOutputSize == NULL))
  {
    /* Clean-up the key in RAM */
    SE_CLEAN_UP_FW_KEY();

    /* clean-up */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
    mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));
    m_AESGCMctx.TagSize = 0;
    memset(m_AESGCMctx.aTag, 0x00, SE_TAG_LEN);
#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256)
    mbedtls_cipher_free(&(m_AESCBCctx.mbedAesCbcCtx));
#endif /* SECBOOT_CRYPTO_SCHEME */

    return SE_ERROR;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* mbedTLS : casting to (size_t *) is valid because we expect 0 bytes (checked afterwards)  */
  ret = mbedtls_cipher_finish(&(m_AESGCMctx.mbedAesGcmCtx), pOutputBuffer, (size_t *)pOutputSize);

  if (0 != *pOutputSize)
  {
    /* error */
    ret = -1;
    *pOutputSize = 0;
  }
  else
  {
    /* Check the GCM tag */
    ret = mbedtls_cipher_check_tag(&(m_AESGCMctx.mbedAesGcmCtx), m_AESGCMctx.aTag, m_AESGCMctx.TagSize);
  }

  /* clean-up */
  mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));
  m_AESGCMctx.TagSize = 0;
  memset(m_AESGCMctx.aTag, 0x00, SE_TAG_LEN);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }

#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256)
  /* mbedTLS : casting to (size_t *) is valid because we expect 0 bytes (checked afterwards) */
  ret = mbedtls_cipher_finish(&(m_AESCBCctx.mbedAesCbcCtx), pOutputBuffer, (size_t *)pOutputSize);
  mbedtls_cipher_free(&(m_AESCBCctx.mbedAesCbcCtx));

  if ((0 == ret) && (0 == *pOutputSize))
  {
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    /* error */
    *pOutputSize = 0;
  }
#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)
  /* Nothing to do */
  e_ret_status = SE_SUCCESS;
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Clean-up the key in RAM */
  SE_CLEAN_UP_FW_KEY();

  /* Return status*/
  return e_ret_status;
}


/**
  * @brief Secure Engine AuthenticateFW Init function.
  *        It is a wrapper of the mbedTLS crypto or hash function included in the protected area used to initialize the
  *        FW authentication procedure.
  * @param pKey: pointer to the key.
  * @param pxSE_Metadata: Firmware metadata.
  * @param SE_FwType: Type of Fw Image.
  *        This parameter can be SE_FW_IMAGE_COMPLETE or SE_FW_IMAGE_PARTIAL.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_AuthenticateFW_Init(SE_FwRawHeaderTypeDef *pxSE_Metadata, int32_t SE_FwType)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;

  /*
   * Depending on the crypto scheme, the Firmware Tag (signature) can be:
   *   - either an AES GCM tag
   *   - or a SHA256 digest (encapsulated in the authenticated FW metadata)
   */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  e_ret_status = SE_CRYPTO_Encrypt_Init(pxSE_Metadata, SE_FwType);
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  int32_t ret; /* mbedTLS return code */

  UNUSED(pxSE_Metadata);
  UNUSED(SE_FwType);

  mbedtls_sha256_init(&m_SHA256ctx);

  ret = mbedtls_sha256_starts_ret(&m_SHA256ctx, 0); /* is224:0 for sha256 */

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }
  else
  {
    mbedtls_sha256_free(&m_SHA256ctx);
  }
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine AuthenticateFW Append function.
  *        It is a wrapper of the mbedTLS crypto or hash function included in the protected area used during the FW
  *        authentication procedure.
  * @param pInputBuffer: pointer to Input Buffer.
  * @param InputSize: Input Size (bytes).
  * @param pOutputBuffer: pointer to Output Buffer.
  * @param pOutputSize: pointer to Output Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_AuthenticateFW_Append(const uint8_t *pInputBuffer, int32_t InputSize, uint8_t *pOutputBuffer,
                                               int32_t *pOutputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
  /*
   * Depending on the crypto scheme, the Firmware Tag (signature) can be:
   *   - either an AES GCM tag
   *   - or a SHA256 digest
   */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  e_ret_status = SE_CRYPTO_Encrypt_Append(pInputBuffer, InputSize, pOutputBuffer, pOutputSize);
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  int32_t ret; /* mbedTLS return code */

  /* The parameters below are useless for the HASH but are needed for API compatibility with other procedures */
  (void)pOutputBuffer;
  (void)pOutputSize;

  if (InputSize < 0)
  {
    return (SE_ERROR);
  }
  /* else the cast to size_t is valid */

  ret = mbedtls_sha256_update_ret(&m_SHA256ctx, pInputBuffer, (size_t)InputSize);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine AuthenticateFW Finish function.
  *        It is a wrapper of the mbedTLS crypto or hash function included in the protected area used during the FW
  *        authentication procedure..
  * @param pOutputBuffer: pointer to Output Buffer.
  * @param pOutputSize: pointer to Output Size (bytes).
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_AuthenticateFW_Finish(uint8_t *pOutputBuffer, int32_t *pOutputSize)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
  /*
   * Depending on the crypto scheme, the Firmware Tag (signature) can be:
   *   - either an AES GCM tag
   *   - or a SHA256 digest
   */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  e_ret_status = SE_CRYPTO_Encrypt_Finish(pOutputBuffer, pOutputSize);
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  int32_t ret = mbedtls_sha256_finish_ret(&m_SHA256ctx, pOutputBuffer);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
    *pOutputSize = 32; /* sha256 so 32 bytes */
  }
  else
  {
    *pOutputSize = 0;
  }

  mbedtls_sha256_free(&m_SHA256ctx);
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
  return e_ret_status;
}

/**
  * @brief Secure Engine Authenticate Metadata function.
  *        Authenticates the header containing the Firmware metadata.
  * @param pxSE_Metadata: Firmware metadata.
  * @retval SE_ErrorStatus SE_SUCCESS if successful, SE_ERROR otherwise.
  */
SE_ErrorStatus SE_CRYPTO_Authenticate_Metadata(SE_FwRawHeaderTypeDef *pxSE_Metadata)
{
  SE_ErrorStatus e_ret_status = SE_ERROR;
  int32_t ret; /* mbedTLS return code */

#if ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /*
   * Variable used to store the Asymmetric Key inside the protected area (even if this is a public key it is stored
   * like the secret key).
   * Please make sure this variable is placed in protected SRAM1.
   */
  static uint8_t m_aSE_PubKey[SE_ASYM_PUBKEY_LEN];
#endif /* SECBOOT_CRYPTO_SCHEME */

  /*
   * Local variables for authentication procedure.
   */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  uint8_t fw_raw_header_output[SE_FW_HEADER_TOT_LEN];
  int32_t fw_raw_header_output_length;
#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  int32_t status = 0;
  const uint8_t *pSign_r;
  const uint8_t *pSign_s;
  /* Firmware metadata to be authenticated and reference MAC */
  const uint8_t *pPayload;    /* Metadata payload */
  int32_t payloadSize;        /* Metadata length to be considered for hash */
  uint8_t *pSign;             /* Reference MAC (ECCDSA signed SHA256 of the FW metadata) */
  uint8_t MessageDigest[32];      /* The message digest is a sha256 so 32 bytes */
  int32_t MessageDigestLength = 0;
  mbedtls_ecp_group grp; /* Elliptic curve definition*/
  mbedtls_ecp_point Q;   /* Public ECC key */
  mbedtls_mpi r;         /* ECDSA Signature to be verified (r part) */
  mbedtls_mpi s;         /* ECDSA Signature to be verified (s part) */
#endif /* SECBOOT_CRYPTO_SCHEME */


  /* the key to be used for crypto operations (as this is a pointer to m_aSE_FirmwareKey or m_aSE_PubKey it can be a
     local variable, the pointed data is protected) */
  uint8_t *pKey;

  if (NULL == pxSE_Metadata)
  {
    return SE_ERROR;
  }

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* Initialize the key */
  SE_ReadKey(&(m_aSE_FirmwareKey[0]));
  pKey = &(m_aSE_FirmwareKey[0]);

  /* Verify the HeaderMAC with mbedTLS and the AES GCM Additional Authenticated Data */
  mbedtls_cipher_init(&(m_AESGCMctx.mbedAesGcmCtx));
  ret = mbedtls_cipher_setup(&(m_AESGCMctx.mbedAesGcmCtx), mbedtls_cipher_info_from_values(MBEDTLS_CIPHER_ID_AES,
                             SE_SYMKEY_LEN * 8, MBEDTLS_MODE_GCM));
  ret += mbedtls_cipher_setkey(&(m_AESGCMctx.mbedAesGcmCtx), pKey, SE_SYMKEY_LEN * 8, MBEDTLS_DECRYPT);
  ret += mbedtls_cipher_set_iv(&(m_AESGCMctx.mbedAesGcmCtx), pxSE_Metadata->Nonce, SE_NONCE_LEN);
  ret += mbedtls_cipher_reset(&(m_AESGCMctx.mbedAesGcmCtx));

  if (0 != ret)
  {
    mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));
    return SE_ERROR;
  }

  /*
   * Save the GCM tag of the header too : this is the tag to check to authenticate the Firmware Header.
   * This is not really needed as pxSE_Metadata is still available when calling mbedtls_cipher_check_tag,
   * but it is consistent with the SE_CRYPTO_AuthenticateFW_ primitives.
   */
  memcpy(m_AESGCMctx.aTag, (uint8_t *)pxSE_Metadata->HeaderMAC, SE_TAG_LEN);
  m_AESGCMctx.TagSize = (size_t)SE_TAG_LEN;

  /* Provide only additional data to be authenticated: the AAD is the firmware header (without its TAG) */
  ret = mbedtls_cipher_update_ad(&(m_AESGCMctx.mbedAesGcmCtx), (uint8_t *)pxSE_Metadata,
                                 (((int32_t)SE_FW_HEADER_TOT_LEN) - SE_TAG_LEN));

  if (0 == ret)
  {
    /* We expect 0 byte to be flushed in the buffer "fw_raw_header_output" so the cats to (size_t *) is valid */
    ret = mbedtls_cipher_finish(&(m_AESGCMctx.mbedAesGcmCtx), fw_raw_header_output,
                                (size_t *)&fw_raw_header_output_length);
  }

  if ((0 == ret) && (0 == fw_raw_header_output_length))
  {
    /* Check the GCM tag (Firmware Header Tag) */
    ret = mbedtls_cipher_check_tag(&(m_AESGCMctx.mbedAesGcmCtx), m_AESGCMctx.aTag, m_AESGCMctx.TagSize);
  }

  /* Clean-up */
  mbedtls_cipher_free(&(m_AESGCMctx.mbedAesGcmCtx));
  m_AESGCMctx.TagSize = 0;
  memset(m_AESGCMctx.aTag, 0x00, SE_TAG_LEN);

  if (0 == ret)
  {
    e_ret_status = SE_SUCCESS;
  }

#elif ( (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256) )
  /* Retrieve the ECC Public Key */
  SE_ReadKey_Pub(&(m_aSE_PubKey[0]));
  pKey = &(m_aSE_PubKey[0]);

  /* Set the local variables required to handle the Firmware Metadata during the authentication procedure */
  pPayload = (const uint8_t *)pxSE_Metadata;
  payloadSize = SE_FW_HEADER_TOT_LEN - SE_MAC_LEN;
  pSign = pxSE_Metadata->HeaderMAC;

  /* signature to be verified with r and s components */
  pSign_r = pSign;
  pSign_s = (uint8_t *)(pSign + 32);

  /* Compute the SHA256 of the Firmware Metadata */
  status = SE_CRYPTO_SHA256_HASH_DigestCompute(pPayload,
                                               payloadSize,
                                               (uint8_t *)MessageDigest,
                                               &MessageDigestLength);

  if (0 == status)
  {
    /* mbedTLS resources */
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

    if (ret != 0)
    {
      e_ret_status = SE_ERROR;
    }
    else
    {
      /* We must use the mbedTLS format for the public key */
      unsigned char aKey[65];
      aKey[0] = 0x04; /* uncompressed point */
      (void)memcpy(&aKey[1], pKey, 64);

      /* Public key loaded as point Q */
      ret = mbedtls_ecp_point_read_binary(&grp, &Q, aKey, 65);

      if (ret != 0)
      {
        e_ret_status = SE_ERROR;
      }
      else
      {
        /*
         * pSign_r and pSign_s are already set.
         * Prepare the signature to be verified (r and s components).
         */
        ret = mbedtls_mpi_read_binary(&r, pSign_r, 32);
        ret += mbedtls_mpi_read_binary(&s, pSign_s, 32);

        if (0 == ret)
        {
          /* verify the signature with the sha256 */
          ret = mbedtls_ecdsa_verify(&grp, MessageDigest, MessageDigestLength, &Q, &r, &s);
        }

        if (ret != 0)
        {
          e_ret_status = SE_ERROR;
        }
        else
        {
          e_ret_status = SE_SUCCESS;
        }
      }

      /* clean-up the mbedTLS resources */
      mbedtls_mpi_free(&r);
      mbedtls_mpi_free(&s);
      mbedtls_ecp_point_free(&Q);
      mbedtls_ecp_group_free(&grp);
    }
  } /* else the status is already SE_ERROR */

#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Clean-up the key in RAM */
#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
  /* Symmetric key */
  SE_CLEAN_UP_FW_KEY();
#else
  /* ECC public key */
  SE_CLEAN_UP_PUB_KEY();
#endif /* SECBOOT_CRYPTO_SCHEME */

  /* Return status*/
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
