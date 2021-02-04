/**
  ******************************************************************************
  * @file    mapping_export.h
  * @author  MCD Application Team
  * @brief   This file contains the definitions exported from mapping linker files.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright(c) 2017 STMicroelectronics International N.V.
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
#ifndef MAPPING_EXPORT_H
#define MAPPING_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_APP SFU Application Configuration
  * @{
  */

/** @defgroup SFU_APP_Exported_Types Exported Types
  * @{
  */
/** @defgroup SFU_CONFIG_SBSFU_MEMORY_MAPPING SBSFU Memory Mapping
  * @{
  */
#if defined (__ICCARM__) || defined(__GNUC__)
extern uint32_t __ICFEDIT_intvec_start__;
#define INTVECT_START ((uint32_t)& __ICFEDIT_intvec_start__)
extern uint32_t __ICFEDIT_SE_Startup_region_ROM_start__;
#define SE_STARTUP_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_Startup_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_Code_region_ROM_start__;
#define SE_CODE_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_Code_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_Code_region_ROM_end__;
#define SE_CODE_REGION_ROM_END ((uint32_t)& __ICFEDIT_SE_Code_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_IF_region_ROM_start__;
#define SE_IF_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_IF_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_IF_region_ROM_end__;
#define SE_IF_REGION_ROM_END ((uint32_t)& __ICFEDIT_SE_IF_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_Key_region_ROM_start__;
#define SE_KEY_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_Key_region_ROM_start__)
extern uint32_t __ICFEDIT_SE_Key_region_ROM_end__;
#define SE_KEY_REGION_ROM_END ((uint32_t)& __ICFEDIT_SE_Key_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_CallGate_region_ROM_start__; 
#define SE_CALLGATE_REGION_ROM_START ((uint32_t)& __ICFEDIT_SE_CallGate_region_ROM_start__)
extern uint32_t __ICFEDIT_SB_region_ROM_start__;
#define SB_REGION_ROM_START ((uint32_t)& __ICFEDIT_SB_region_ROM_start__)
extern uint32_t __ICFEDIT_SB_region_ROM_end__;
#define SB_REGION_ROM_END ((uint32_t)& __ICFEDIT_SB_region_ROM_end__)
extern uint32_t __ICFEDIT_SE_region_SRAM1_start__;
#define SE_REGION_SRAM1_START ((uint32_t)& __ICFEDIT_SE_region_SRAM1_start__)
extern uint32_t __ICFEDIT_SE_region_SRAM1_end__ ;
#define SE_REGION_SRAM1_END ((uint32_t)& __ICFEDIT_SE_region_SRAM1_end__)
extern uint32_t __ICFEDIT_SB_region_SRAM1_start__ ;
#define SB_REGION_SRAM1_START ((uint32_t)& __ICFEDIT_SB_region_SRAM1_start__)
extern uint32_t __ICFEDIT_SB_region_SRAM1_end__ ;
#define SB_REGION_SRAM1_END ((uint32_t)& __ICFEDIT_SB_region_SRAM1_end__)
extern uint32_t __ICFEDIT_SE_region_SRAM1_stack_top__;
#define SE_REGION_SRAM1_STACK_TOP ((uint32_t)& __ICFEDIT_SE_region_SRAM1_stack_top__)
#elif defined(__CC_ARM)
extern uint32_t Image$$vector_start$$Base;
#define  INTVECT_START ((uint32_t)& Image$$vector_start$$Base)
#endif


/**
  * @}
  */
  
/** @defgroup SFU_CONFIG_FW_MEMORY_MAPPING Firmware Slots Memory Mapping
  * @{
  */
#if defined (__ICCARM__) || defined(__GNUC__)
extern uint32_t __ICFEDIT_region_SLOT_0_start__;
#define REGION_SLOT_0_START ((uint32_t)& __ICFEDIT_region_SLOT_0_start__)
extern uint32_t __ICFEDIT_region_SLOT_0_end__;
#define REGION_SLOT_0_END ((uint32_t)& __ICFEDIT_region_SLOT_0_end__)
extern uint32_t __ICFEDIT_region_SLOT_1_start__;
#define REGION_SLOT_1_START ((uint32_t)& __ICFEDIT_region_SLOT_1_start__)
extern uint32_t __ICFEDIT_region_SLOT_1_end__;
#define REGION_SLOT_1_END ((uint32_t)& __ICFEDIT_region_SLOT_1_end__)
extern uint32_t __ICFEDIT_region_SWAP_start__;
#define REGION_SWAP_START ((uint32_t)& __ICFEDIT_region_SWAP_start__)
extern uint32_t __ICFEDIT_region_SWAP_end__;
#define REGION_SWAP_END ((uint32_t)& __ICFEDIT_region_SWAP_end__)
#endif

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

#endif /* MAPPING_EXPORT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

