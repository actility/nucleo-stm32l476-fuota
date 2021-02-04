/**
  ******************************************************************************
  * @file    se_def_metadata.h
  * @author  MCD Application Team
  * @brief   This file contains metadata definitions for SE functionalities.
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
#ifndef SE_DEF_METADATA_H
#define SE_DEF_METADATA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "se_crypto_config.h"

/** @addtogroup  SE Secure Engine
  * @{
  */

/** @addtogroup  SE_CORE SE Core
  * @{
  */

/** @addtogroup  SE_CORE_DEF SE Definitions
  * @{
  */

/** @defgroup SE_DEF_METADATA SE Metadata Definitions
  *  @brief definitions related to FW metadata (header).
  * @{
  */

/** @defgroup SE_DEF_METADATA_Exported_Constants Exported Constants
  * @brief  Firmware Image Header (FW metadata) constants
  * @{
  */

#define SE_SYMKEY_LEN           ((int32_t) 16)  /*!< SE Symmetric Key length (bytes)*/
#define SE_FW_HEADER_TOT_LEN    ((int32_t) sizeof(SE_FwRawHeaderTypeDef))   /*!< FW INFO header Total Length*/

#if (SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM)
/* AES-GCM encryption and FW Tag */
#define SE_NONCE_LEN            ((int32_t) 12)  /*!< Secure Engine Nonce Length (Bytes)*/
#define SE_TAG_LEN              ((int32_t) 16)  /*!< Secure Engine Tag Length (Bytes): AES-GCM for the FW tag */
#elif ((SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256))
/* AES-CBC encryption (or no encryption) and SHA256 for FW tag */
#define SE_IV_LEN               ((int32_t) 16)  /*!< Secure Engine IV Length (Bytes): same size as an AES block*/
#define SE_TAG_LEN              ((int32_t) 32)  /*!< Secure Engine Tag Length (Bytes): SHA-256 for the FW tag */
#else
#error "The current example does not support the selected crypto scheme."
#endif /* SECBOOT_CRYPTO_SCHEME */

#if ((SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256) || (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256))
/*
 * For this crypto scheme we use
 *     - a SHA256 tag signed with ECCDSA to authenticate the Firmware Metadata: SE_MAC_LEN
 *     - asymmetric keys
 */
#define SE_MAC_LEN              ((int32_t) 64)  /*!< Firmware Header MAC LEN*/
#define SE_ASYM_PUBKEY_LEN      ((int32_t) 64)  /*!< SE Asymmetric Public Key length (bytes)*/
#endif /* SECBOOT_CRYPTO_SCHEME */

/* Image type: complete or partial image */
#define SE_FW_IMAGE_COMPLETE    ((int32_t) 0)   /*!< Complete Fw Image */
#define SE_FW_IMAGE_PARTIAL     ((int32_t) 1)   /*!< Partial Fw Image */


/*!< FW Metadata INFO header Length: i.e. header length from beginning to reserved (excluded) */
#if SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM
#define SE_FW_HEADER_METADATA_LEN  (SE_FW_HEADER_TOT_LEN-(112+SE_TAG_LEN))
#elif ( SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256 )
#define SE_FW_HEADER_METADATA_LEN  (SE_FW_HEADER_TOT_LEN-(28+SE_MAC_LEN))
#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)
#define SE_FW_HEADER_METADATA_LEN  (SE_FW_HEADER_TOT_LEN-(44+SE_MAC_LEN))
#endif /* SECBOOT_CRYPTO_SCHEME */
/**
  * @}
  */

/** @defgroup SE_DEF_METADATA_Exported_Types Exported Types
  * @{
  */

/**
  * @brief  Firmware Header structure definition
  * @note This structure MUST be called SE_FwRawHeaderTypeDef
  * @note This structure MUST contain a field named 'FwVersion'
  * @note This structure MUST contain a field named 'FwSize'
  * @note This structure MUST contain a field named 'FwTag' if you want to support FW authentication (but the header
  *       must be authenticated too if this tag is not a MAC)
  * @note This structure MUST contain a field named 'PartialFwOffset'
  * @note This structure MUST contain a field named 'PartialFwSize'
  * @note This structure MUST contain a field named 'PartialFwTag' if you want to support FW authentication (but the
  *       header must be authenticated too if this tag is not a MAC)
  * @note This structure MUST contain a field named 'HeaderMAC' if you want to support authentication (metadata only or
  *       both metadata + FW)
  * @note In this example, the header size is always a multiple of 32 to match the FLASH constraint on STM32H7.
  *       We keep this alignment for all platforms (even when the FLASH alignment constraint is another value) to have
  *       one unique header size per crypto scheme.
  * @note In this example, the header size is always 192 bytes (for all crypto schemes).
  */
#if SECBOOT_CRYPTO_SCHEME == SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM
typedef struct
{
  uint32_t SFUMagic;               /*!< SFU Magic 'SFUM'*/
  uint16_t ProtocolVersion;        /*!< SFU Protocol version*/
  uint16_t FwVersion;              /*!< Firmware version*/
  uint32_t FwSize;                 /*!< Firmware size (bytes)*/
  uint32_t PartialFwOffset;        /*!< Offset (bytes) of partial firmware vs full firmware */
  uint32_t PartialFwSize;          /*!< Size of partial firmware */
  uint8_t  FwTag[SE_TAG_LEN];      /*!< Firmware Tag*/
  uint8_t  PartialFwTag[SE_TAG_LEN];/*!< Partial firmware Tag */
  uint8_t  Nonce[SE_NONCE_LEN];    /*!< Nonce used to encrypt firmware*/
  uint8_t  Reserved[112];          /*!< Reserved for future use: 112 extra bytes to have a header size of 192 bytes */
  uint8_t  HeaderMAC[SE_MAC_LEN];  /*!< MAC of the full header message */
} SE_FwRawHeaderTypeDef;

#elif ( SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256 )

typedef struct
{
  uint32_t SFUMagic;               /*!< SFU Magic 'SFUM'*/
  uint16_t ProtocolVersion;        /*!< SFU Protocol version*/
  uint16_t FwVersion;              /*!< Firmware version*/
  uint32_t FwSize;                 /*!< Firmware size (bytes)*/
  uint32_t PartialFwOffset;        /*!< Offset (bytes) of partial firmware vs full firmware */
  uint32_t PartialFwSize;          /*!< Size of partial firmware */
  uint8_t  FwTag[SE_TAG_LEN];      /*!< Firmware Tag*/
  uint8_t  PartialFwTag[SE_TAG_LEN];/*!< Partial firmware Tag */
  uint8_t  InitVector[SE_IV_LEN];  /*!< IV used to encrypt firmware */
  uint8_t  Reserved[28];           /*!< Reserved for future use: 28 extra bytes to have a header size of 192 bytes */
  uint8_t  HeaderMAC[SE_MAC_LEN];  /*!< MAC of the full header message */
} SE_FwRawHeaderTypeDef;

#elif (SECBOOT_CRYPTO_SCHEME == SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256)

typedef struct
{
  uint32_t SFUMagic;               /*!< SFU Magic 'SFUM'*/
  uint16_t ProtocolVersion;        /*!< SFU Protocol version*/
  uint16_t FwVersion;              /*!< Firmware version*/
  uint32_t FwSize;                 /*!< Firmware size (bytes)*/
  uint32_t PartialFwOffset;        /*!< Offset (bytes) of partial firmware vs full firmware */
  uint32_t PartialFwSize;          /*!< Size of partial firmware */
  uint8_t  FwTag[SE_TAG_LEN];      /*!< Firmware Tag*/
  uint8_t  PartialFwTag[SE_TAG_LEN];/*!< Partial firmware Tag */
  uint8_t  Reserved[44];           /*!< Reserved for future use: 44 extra bytes to have a header size of 192 bytes */
  uint8_t  HeaderMAC[SE_MAC_LEN];  /*!< MAC of the full header message */
} SE_FwRawHeaderTypeDef;

#else

#error "The current example does not support the selected crypto scheme."

#endif /* SECBOOT_CRYPTO_SCHEME */

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

#endif /* SE_DEF_METADATA_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

