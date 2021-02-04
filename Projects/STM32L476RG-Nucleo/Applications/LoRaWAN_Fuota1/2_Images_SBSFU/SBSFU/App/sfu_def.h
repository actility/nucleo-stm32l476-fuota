/**
  ******************************************************************************
  * @file    sfu_def.h
  * @author  MCD Application Team
  * @brief   This file contains the general definitions for SBSFU application.
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
#ifndef SFU_DEF_H
#define SFU_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#if defined (__ICCARM__) || defined(__GNUC__)
#include "mapping_export.h"
#elif defined(__CC_ARM)
#include "mapping_sbsfu.h"
#endif /* __ICCARM__ || __GNUC__ */
#include "app_sfu.h"

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_APP SFU Application Configuration
  * @{
  */
/** @addtogroup SFU_Main SBSFU Global Definition
  * @{
  */

/**
  * @brief  SFU Error Typedef
  */
typedef enum
{
  SFU_ERROR = 0x00001FE1U,
  SFU_SUCCESS = 0x00122F11U
} SFU_ErrorStatus;

/**
  * @}
  */

/** @defgroup SFU_APP_Exported_Types Exported Types
  * @{
  */
/** @defgroup SFU_CONFIG_MEMORY_MAPPING Memory Mapping
  * @{
  */
#if defined(__CC_ARM)
extern uint32_t Image$$vector_start$$Base;
#define  INTVECT_START ((uint32_t)& Image$$vector_start$$Base)
#endif /* __CC_ARM */

#define SFU_SRAM1_BASE           ((uint32_t) SE_REGION_SRAM1_START)
#define SFU_SRAM1_END            ((uint32_t) SB_REGION_SRAM1_END)
#define SFU_SB_SRAM1_BASE        ((uint32_t) SB_REGION_SRAM1_START)
#define SFU_SB_SRAM1_END         ((uint32_t) SB_REGION_SRAM1_END)

/* needed for sfu_test.c */
#define SFU_SRAM2_BASE           ((uint32_t)0x10000000)
#define SFU_SRAM2_END            ((uint32_t)0x10007FFF)

#define SFU_BOOT_BASE_ADDR       ((uint32_t) INTVECT_START)           /* SFU Boot Address */
#if (SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER)
#define SFU_AREA_ADDR_END        ((uint32_t) LOADER_REGION_ROM_END)   /* External Loader end Address (covering all 
                                                                         the SBSFU + external loader executable code) */
#else
#define SFU_AREA_ADDR_END        ((uint32_t) SB_REGION_ROM_END)       /* SBSFU end Address (covering all the SBSFU 
                                                                         executable code) */
#endif /* SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER */

#define SFU_SENG_AREA_ADDR_START ((uint32_t) SE_CODE_REGION_ROM_START)/* Secure Engine area address START */
#define SFU_SENG_AREA_ADDR_END   ((uint32_t) SE_CODE_REGION_ROM_END)  /* Secure Engine area address END - SE includes 
                                                                         everything up to the License */
#define SFU_SENG_AREA_SIZE       ((uint32_t) (SFU_SENG_AREA_ADDR_END - \
                                              SFU_SENG_AREA_ADDR_START + 1U)) /* Secure Engine area size */

#define SFU_KEYS_AREA_ADDR_START ((uint32_t) SE_KEY_REGION_ROM_START) /* Keys Area (Keys + Keys Retrieve function) 
                                                                         START. This is the PCRoP Area */
#define SFU_KEYS_AREA_ADDR_END   ((uint32_t) SE_KEY_REGION_ROM_END)   /* Keys Area (Keys + Keys Retrieve function) END. 
                                                                         This is the PCRoP Area */


#define SFU_SENG_RAM_ADDR_START  ((uint32_t) SE_REGION_SRAM1_START)   /* Secure Engine reserved RAM1 area START 
                                                                         address */
#define SFU_SENG_RAM_ADDR_END    ((uint32_t) SE_REGION_SRAM1_END)     /* Secure Engine reserved RAM1 area END address */
#define SFU_SENG_RAM_SIZE        ((uint32_t) (SFU_SENG_RAM_ADDR_END - \
                                              SFU_SENG_RAM_ADDR_START + 1U)) /* Secure Engine reserved RAM area SIZE */


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

#endif /* SFU_DEF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

