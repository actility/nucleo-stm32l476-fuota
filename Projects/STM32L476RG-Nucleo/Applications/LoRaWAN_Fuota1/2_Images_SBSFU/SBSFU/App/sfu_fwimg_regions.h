/**
  ******************************************************************************
  * @file    sfu_fwimg_regions.h
  * @author  MCD Application Team
  * @brief   This file contains FLASH regions definitions for SFU_FWIMG functionalities
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
#ifndef SFU_FWIMG_REGIONS_H
#define SFU_FWIMG_REGIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#if defined(__CC_ARM)
#include "mapping_fwimg.h"
#elif defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#endif /* __CC_ARM */
#include "se_crypto_config.h"


/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_IMG SFU Firmware Image
  * @{
  */

/** @addtogroup SFU_IMG_SERVICES
  * @{
  */

/** @defgroup SFU_IMG_SERVICES_REGIONS Firmware Slots
  * @brief FLASH Regions containing the Firmware Slots
  * @{
  */

/** @addtogroup SFU_IMG_Exported_Defines_REGIONS Exported Defines
  * @{
  */

/*
 * Nominal settings for SB_SFU configuration
 */
#define SFU_IMG_SLOT_0_REGION_BEGIN_VALUE ((uint32_t)REGION_SLOT_0_START)
#define SFU_IMG_SLOT_0_REGION_BEGIN ((uint8_t *)SFU_IMG_SLOT_0_REGION_BEGIN_VALUE)
#define SFU_IMG_SLOT_0_REGION_SIZE ((uint32_t)(REGION_SLOT_0_END - REGION_SLOT_0_START + 1U))

#define SFU_IMG_SLOT_1_REGION_BEGIN_VALUE ((uint32_t)REGION_SLOT_1_START)
#define SFU_IMG_SLOT_1_REGION_BEGIN ((uint8_t*) SFU_IMG_SLOT_1_REGION_BEGIN_VALUE)
#define SFU_IMG_SLOT_1_REGION_SIZE ((uint32_t)(REGION_SLOT_1_END - REGION_SLOT_1_START + 1U))

#define SFU_IMG_SWAP_REGION_BEGIN_VALUE  ((uint32_t)REGION_SWAP_START)
#define SFU_IMG_SWAP_REGION_BEGIN ((uint8_t *)SFU_IMG_SWAP_REGION_BEGIN_VALUE)
#define SFU_IMG_SWAP_REGION_SIZE ((uint32_t)(REGION_SWAP_END - REGION_SWAP_START + 1U))

/*
 * Design constraint: the image slot size must be a multiple of the swap area size.
 * And of course both image slots must have the same size.
 */
#define SFU_IMG_REGION_IS_MULTIPLE(a,b) ((a / b * b) == a)

/*
 * Checking that the slot sizes are consistent with the .icf file
 * Sizes expressed in bytes (+1 because the end address belongs to the slot)
 * The checks are executed at runtime in the SFU_Img_Init() function.
 */
#define SFU_IMG_REGION_IS_SAME_SIZE(a,b) ((a) == (b))

/*
 * Identify slot used for FW Image download : SLOT 1
 */
#define SFU_IMG_SLOT_DWL_REGION_BEGIN_VALUE SFU_IMG_SLOT_1_REGION_BEGIN_VALUE
#define SFU_IMG_SLOT_DWL_REGION_BEGIN       SFU_IMG_SLOT_1_REGION_BEGIN
#define SFU_IMG_SLOT_DWL_REGION_SIZE        SFU_IMG_SLOT_1_REGION_SIZE


/**
  * @brief Image starting offset to add to the  address of 1st block
  */
#define SFU_IMG_IMAGE_OFFSET ((uint32_t)512U)


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

#endif /* SFU_FWIMG_REGIONS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

