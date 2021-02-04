/**
  ******************************************************************************
  * @file    sfu_low_level_security.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for Secure Firmware Update security
  *          low level interface.
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
#ifndef SFU_LOW_LEVEL_SECURITY_H
#define SFU_LOW_LEVEL_SECURITY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "sfu_fwimg_regions.h"
#include "sfu_def.h"


/** @addtogroup SFU Secure Secure Boot / Firmware Update
  * @{
  */

/** @addtogroup SFU_LOW_LEVEL
  * @{
  */

/** @defgroup SFU_LOW_LEVEL_SECURITY Security Low Level Interface
  * @{
  */

/** @defgroup SFU_SECURITY_Configuration Security Configuration
  * @{
  */



/** @defgroup SFU_CONFIG_WRP_AREAS WRP Protected Areas Configuration
  * @{
  */
/*!< Bank 1, Area A  used to protect Vector Table */
#define SFU_PROTECT_WRP_AREA_1          (OB_WRPAREA_BANK1_AREAA)

/*!< First page including the Vector Table: 0 based */
#define SFU_PROTECT_WRP_PAGE_START_1    ((uint32_t)((SFU_BOOT_BASE_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE))

/*!< Last page: (code_size-1)/page_size because the page
     indexes start from 0 */
#define SFU_PROTECT_WRP_PAGE_END_1      ((uint32_t)((SFU_AREA_ADDR_END - FLASH_BASE) / FLASH_PAGE_SIZE))
/**
  * @}
  */

/** @defgroup SFU_CONFIG_PCROP_AREAS PCROP Protected Areas Configuration
  * @{
  */
#define SFU_PROTECT_PCROP_AREA          (FLASH_BANK_1)                            /*!< PCROP Area */
#define SFU_PROTECT_PCROP_ADDR_START    ((uint32_t)SFU_KEYS_AREA_ADDR_START)      /*!< PCROP Start Address (included) */
#define SFU_PROTECT_PCROP_ADDR_END      ((uint32_t)SFU_KEYS_AREA_ADDR_END)        /*!< PCROP End Address*/
/**
  * @}
  */

/** @defgroup SFU_CONFIG_FIREWALL_AREAS FIREWALL Configuration
  * @{
  */
#define SFU_PROTECT_FWALL_CODE_ADDR_START   ((uint32_t) SFU_SENG_AREA_ADDR_START)/*!< Firewall protection CODE area 
                                                                                      START address*/
#define SFU_PROTECT_FWALL_CODE_SIZE         ((uint32_t) SFU_SENG_AREA_SIZE)      /*!< Firewall protection CODE area 
                                                                                      SIZE*/
#define SFU_PROTECT_FWALL_NVDATA_ADDR_START ((uint32_t)(FLASH_BASE+(FLASH_BANK_SIZE)))/*!< Firewall protection NVDATA 
                                                                                           area START address*/
#define SFU_PROTECT_FWALL_NVDATA_SIZE       (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE + SFU_IMG_IMAGE_OFFSET - \
                                             SFU_PROTECT_FWALL_NVDATA_ADDR_START)/*!< Firewall protection NVDATA area
SIZE */
#define SFU_PROTECT_FWALL_VDATA_ADDR_START  ((uint32_t) SFU_SENG_RAM_ADDR_START) /*!< Firewall protection VDATA area 
                                                                                      START address*/
#define SFU_PROTECT_FWALL_VDATA_SIZE        ((uint32_t) SFU_SENG_RAM_SIZE)       /*!< Firewall protection VDATA area 
                                                                                      SIZE */
/**
  * @}
  */

/** @defgroup SFU_CONFIG_MPU_REGIONS MPU Regions Configuration
  * @{
  */

/**
  * @brief The regions can overlap, and can be nested. The region 7 has the highest priority
  *    and the region 0 has the lowest one and this governs how overlapping the regions behave.
  *    The priorities are fixed, and cannot be changed.
  */


/**
  * @brief Region 0 - The Aliases Region and all User Flash & internal RAM. When executing inside SB/SFU by default all
  *                   the Aliases and all the User Flash (used for UserApp1 and UserApp2) area must not be executable,
  *                   but used only to read and write the downloaded code RAM is also not executable.
  *                   From this full Area we'll enable the Execution permission only on the SB/SFU Flash Area.
  */

#define SFU_PROTECT_MPU_AREA_USER_START         ((uint32_t)0x00000000U)
#define SFU_PROTECT_MPU_AREA_USER_SIZE          MPU_REGION_SIZE_1GB  /*!< up to 0x3FFFFFFF */
#define SFU_PROTECT_MPU_AREA_USER_PERM          MPU_REGION_FULL_ACCESS
#define SFU_PROTECT_MPU_AREA_USER_EXEC          MPU_INSTRUCTION_ACCESS_DISABLE
#define SFU_PROTECT_MPU_AREA_USER_SREG          0x00U      /*!< All subregions activated */

/**
  * @brief Region 1 and Region 2 - Enable the execution for SB/SFU Full area (SBSFU + SE + Keys). Inner region inside
  *                                the Region 0
  */
#define SFU_PROTECT_MPU_MAX_NB_SUBREG           (8U)       /*!< 8 sub-regions is the maximum */
#define SFU_PROTECT_MPU_AREA_SFUEN_START_0      FLASH_BASE /*!< Flash memory area */
#define SFU_PROTECT_MPU_AREA_SFUEN_START_1      (0)        /*!< compute at run time */
#define SFU_PROTECT_MPU_AREA_SFUEN_SIZE_0       MPU_REGION_SIZE_128KB
#define SFU_PROTECT_MPU_AREA_SFUEN_SIZE_1       MPU_REGION_SIZE_16KB
#define SFU_PROTECT_MPU_AREA_SFUEN_PERM         MPU_REGION_FULL_ACCESS
#define SFU_PROTECT_MPU_AREA_SFUEN_EXEC         MPU_INSTRUCTION_ACCESS_ENABLE
#define SFU_PROTECT_MPU_AREA_SFUEN_SREG_0       0xFFU      /*!< Subregion mask to deactivate region 1 unexpected MPU 
                                                                subregions: compute at run time */
#define SFU_PROTECT_MPU_AREA_SFUEN_SREG_1       0xFFU      /*!< Subregion mask to deactivate region 2 unexpected MPU 
                                                                subregions: compute at run time */

/**
  * @brief Region 3 - Vector Table: Vector Table must be Read-Only under privileged access. Inner region inside the
  *                   Region 1
  */
#define SFU_PROTECT_MPU_AREA_VECT_START         ((uint32_t) INTVECT_START) /*!< Vector table memory area */
#define SFU_PROTECT_MPU_AREA_VECT_SIZE          MPU_REGION_SIZE_512B
#define SFU_PROTECT_MPU_AREA_VECT_PERM          MPU_REGION_PRIV_RO
#define SFU_PROTECT_MPU_AREA_VECT_EXEC          MPU_INSTRUCTION_ACCESS_ENABLE
#define SFU_PROTECT_MPU_AREA_VECT_SREG          0x00U                   /*!< All subregions activated */

/**
  * @brief Region 4 - Inner region inside the Region 0 . The Option Bytes. Once the Option Bytes have been set,
  *                   only reading will be possible
  */

/*
 * L4xx
 * Only Bank 1 matters because the protections (WRP...) are set on the content of Bank 1.
 * [ 1FFF7800 1FFF7808 1FFF7810 1FFF7818 1FFF7820]: 40 bytes to be protected
 * The MPU can be used to protect up to eight memory regions.
 * These, in turn can have eight subregions, if the region is at least 256 bytes.
 * Here, we define a 64B region so subregions cannot be used.
 */
#define SFU_PROTECT_MPU_AREA_OB_BANK1_START     ((uint32_t)0x1FFF7800U) /*!< Option Bytes in bank 1        */
#define SFU_PROTECT_MPU_AREA_OB_BANK1_SIZE      MPU_REGION_SIZE_64B     /*!< Protecting more than required */
#define SFU_PROTECT_MPU_AREA_OB_BANK1_PERM      MPU_REGION_NO_ACCESS
#define SFU_PROTECT_MPU_AREA_OB_BANK1_EXEC      MPU_INSTRUCTION_ACCESS_DISABLE
#define SFU_PROTECT_MPU_AREA_OB_BANK1_SREG      0x00U                   /*!< Subregion mask to deactivate unexpected 
                                                                             MPU subregions: 0x1F should be set but it 
                                                                             would have no effect as we have a 64B 
                                                                             region */

/**
  * @brief  Region 5 - Peripherals Area
  */
#define SFU_PROTECT_MPU_AREA_PERIPH_START       PERIPH_BASE                  /*!< Peripheral memory area */
#define SFU_PROTECT_MPU_AREA_PERIPH_SIZE        MPU_REGION_SIZE_512MB        /*!< To be coherent with the peripherals 
                                                                                  memory area */
#define SFU_PROTECT_MPU_AREA_PERIPH_PERM        MPU_REGION_FULL_ACCESS
#define SFU_PROTECT_MPU_AREA_PERIPH_EXEC        MPU_INSTRUCTION_ACCESS_DISABLE
#define SFU_PROTECT_MPU_AREA_PERIPH_SREG        0x00U                        /*!< All subregions activated */

/**
  * MPU configuration for UserApp execution
  * =======================================
  * @brief Region 6 & 7 - Enable the execution of the slot#0
  * MPU constraint = Region base address should be aligned on Region size
  */
/**
  * Region definition : from 0x0808 0000 ==> 0x080F FFFF
  * From 0x0808 0000 ==> 0x0808 6000 : No execution capability required but no risk as under firewall protection
  * Remove execution capability from 0x080F 0000 ==> 0x080F FFFF with region 7
  */
#define APP_PROTECT_MPU_AREA_SLOT0_START        (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE / 0x80000 * 0x80000)
#define APP_PROTECT_MPU_AREA_SLOT0_SIZE         MPU_REGION_SIZE_512KB
#define APP_PROTECT_MPU_AREA_SLOT0_PERM         MPU_REGION_PRIV_RO
#define APP_PROTECT_MPU_AREA_SLOT0_EXEC         MPU_INSTRUCTION_ACCESS_ENABLE
#define APP_PROTECT_MPU_AREA_SLOT0_SREG         0x00U                        /*!< 512 Kbytes */

#define APP_PROTECT_MPU_AREA_HIDE_START         (SFU_IMG_SLOT_0_REGION_BEGIN_VALUE + SFU_IMG_SLOT_0_REGION_SIZE)
#define APP_PROTECT_MPU_AREA_HIDE_SIZE          MPU_REGION_SIZE_64KB
#define APP_PROTECT_MPU_AREA_HIDE_PERM          MPU_REGION_FULL_ACCESS
#define APP_PROTECT_MPU_AREA_HIDE_EXEC          MPU_INSTRUCTION_ACCESS_DISABLE
#define APP_PROTECT_MPU_AREA_HIDE_SREG          0x00U                        /*!< 64 Kbytes */



/**
  * @}
  */

/** @defgroup SFU_CONFIG_TAMPER Tamper Configuration
  * @{
  */
#define TAMPER_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOA_CLK_ENABLE()
#define RTC_TAMPER_ID                      RTC_TAMPER_2
#define RTC_TAMPER_ID_INTERRUPT            RTC_TAMPER2_INTERRUPT

/**
  * @}
  */

/** @defgroup SFU_CONFIG_DBG Debug Port Configuration
  * @{
  */
#define SFU_DBG_PORT            GPIOA
#define SFU_DBG_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()
#define SFU_DBG_SWDIO_PIN       GPIO_PIN_13
#define SFU_DBG_SWCLK_PIN       GPIO_PIN_14


/**
  * @}
  */

/**
  * @}
  */

/** @defgroup SFU_SECURITY_Exported_Constants Exported Constants
  * @{
  */

/** @defgroup SFU_SECURITY_Exported_Constants_BOOL SFU Bool definition
  * @{
  */
typedef enum
{
  SFU_FALSE = 0U,
  SFU_TRUE = !SFU_FALSE
} SFU_BoolTypeDef;

/**
  * @}
  */

/** @defgroup SFU_SECURITY_Exported_Constants_Wakeup FU WAKEUP ID Type definition
  * @{
  */
typedef enum
{
  SFU_RESET_UNKNOWN = 0x00U,
  SFU_RESET_FIREWALL,
  SFU_RESET_WDG_RESET,
  SFU_RESET_LOW_POWER,
  SFU_RESET_HW_RESET,
  SFU_RESET_BOR_RESET,
  SFU_RESET_SW_RESET,
  SFU_RESET_OB_LOADER,
} SFU_RESET_IdTypeDef;

/**
  * @}
  */

/** @defgroup SFU_SECURITY_Exported_Constants_Protections FU SECURITY Protections_Constants
  * @{
  */
#define SFU_PROTECTIONS_NONE                 ((uint32_t)0x00000000U)   /*!< Protection configuration unchanged */
#define SFU_STATIC_PROTECTION_RDP            ((uint32_t)0x00000001U)   /*!< RDP protection level 1 is applied */
#define SFU_STATIC_PROTECTION_WRP            ((uint32_t)0x00000002U)   /*!< Constants section in Flash. Needed as
                                                                            separate section to support PCRoP */
#define SFU_STATIC_PROTECTION_PCROP          ((uint32_t)0x00000004U)   /*!< SFU App section in Flash */
#define SFU_STATIC_PROTECTION_LOCKED         ((uint32_t)0x00000008U)   /*!< RDP Level2 is applied. The device is Locked!
                                                                            Std Protections cannot be
                                                                            added/removed/modified */
#define SFU_STATIC_PROTECTION_BFB2           ((uint32_t)0x00000010U)   /*!< BFB2 is disabled. The device shall always
                                                                            boot in bank1! */

#define SFU_RUNTIME_PROTECTION_MPU           ((uint32_t)0x00000100U)   /*!< Shared Info section in Flash */
#define SFU_RUNTIME_PROTECTION_FWALL         ((uint32_t)0x00000200U)   /*!< Firewall protection */
#define SFU_RUNTIME_PROTECTION_IWDG          ((uint32_t)0x00000400U)   /*!< Independent Watchdog */
#define SFU_RUNTIME_PROTECTION_DAP           ((uint32_t)0x00000800U)   /*!< Debug Access Port control */
#define SFU_RUNTIME_PROTECTION_DMA           ((uint32_t)0x00001000U)   /*!< DMA protection, disable DMAs */
#define SFU_RUNTIME_PROTECTION_ANTI_TAMPER   ((uint32_t)0x00002000U)   /*!< Anti-Tampering protections */
#define SFU_RUNTIME_PROTECTION_CLOCK_MONITOR ((uint32_t)0x00004000U)   /*!< Activate a clock monitoring */
#define SFU_RUNTIME_PROTECTION_TEMP_MONITOR  ((uint32_t)0x00008000U)   /*!< Activate a Temperature monitoring */

#define SFU_STATIC_PROTECTION_ALL           (SFU_STATIC_PROTECTION_RDP   | SFU_STATIC_PROTECTION_WRP   | \
                                             SFU_STATIC_PROTECTION_PCROP | SFU_STATIC_PROTECTION_LOCKED)
/*!< All the static protections */

#define SFU_RUNTIME_PROTECTION_ALL          (SFU_RUNTIME_PROTECTION_MPU  | SFU_RUNTIME_PROTECTION_FWALL | \
                                             SFU_RUNTIME_PROTECTION_IWDG | SFU_RUNTIME_PROTECTION_DAP   | \
                                             SFU_RUNTIME_PROTECTION_DMA  | SFU_RUNTIME_PROTECTION_ANTI_TAMPER  | \
                                             SFU_RUNTIME_PROTECTION_CLOCK_MONITOR | SFU_RUNTIME_PROTECTION_TEMP_MONITOR)
/*!< All the run-time protections */

#define SFU_INITIAL_CONFIGURATION           (0x00)     /*!< Initial configuration */
#define SFU_SECOND_CONFIGURATION            (0x01)     /*!< Second configuration */
#define SFU_THIRD_CONFIGURATION             (0x02)     /*!< Third configuration */
/**
  * @}
  */

/**
  * @}
  */


/** @defgroup SFU_SECURITY_Exported_Types Exported Types
  * @{
  */

/**
  * @brief  SFU_MPU Type SFU MPU Type
  */
typedef struct
{
  uint8_t                Number;            /*!< Specifies the number of the region to protect. This parameter can be a
                                                 value of CORTEX_MPU_Region_Number */
  uint32_t               BaseAddress;       /*!< Specifies the base address of the region to protect. */
  uint8_t                Size;              /*!< Specifies the size of the region to protect. */
  uint8_t                AccessPermission;  /*!< Specifies the region access permission type. This parameter can be a
                                                 value of CORTEX_MPU_Region_Permission_Attributes */
  uint8_t                DisableExec;       /*!< Specifies the instruction access status. This parameter can be a value
                                                 of  CORTEX_MPU_Instruction_Access */
  uint8_t                SubRegionDisable;  /*!< Specifies the sub region field (region is divided in 8 slices) when bit
                                                 is 1 region sub region is disabled */
} SFU_MPU_InitTypeDef;

typedef uint32_t      SFU_ProtectionTypeDef;  /*!<   SFU HAL IF Protection Type Def*/

/**
  * @}
  */

/** @defgroup SFU_SECURITY_Exported_Macros Exported Macros
  * @{
  */

/** @defgroup SFU_SECURITY_Exported_Macros_Exceptions Exceptions Management
  * @brief Definitions used in order to have the same clean approach in the SFU_BOOT to manage exceptions
  * @{
  */

#define SFU_CALLBACK_Antitamper(void) HAL_RTCEx_Tamper2EventCallback(RTC_HandleTypeDef *hrtc)
/*!<SFU Redirect of RTC Tamper Event Callback*/
#define SFU_CALLBACK_MemoryFault(void) MemManage_Handler(void)  /*!<SFU Redirect of Mem Manage Callback*/
#define SFU_CALLBACK_HardFault(void) HardFault_Handler(void)    /*!<SFU Redirect of Hard Fault Callback*/

/**
  * @}
  */

/** @defgroup SFU_SECURITY_Exported_Functions Exported Functions
  * @{
  */
SFU_ErrorStatus    SFU_LL_SECU_IWDG_Refresh(void);
SFU_ErrorStatus    SFU_LL_SECU_CheckApplyStaticProtections(void);
SFU_ErrorStatus    SFU_LL_SECU_CheckApplyRuntimeProtections(uint8_t uStep);
void               SFU_LL_SECU_GetResetSources(SFU_RESET_IdTypeDef *peResetpSourceId);
void               SFU_LL_SECU_ClearResetSources(void);
#ifdef SFU_MPU_PROTECT_ENABLE
SFU_ErrorStatus    SFU_LL_SECU_SetProtectionMPU(void);
#endif /*SFU_MPU_PROTECT_ENABLE*/
#ifdef SFU_MPU_PROTECT_ENABLE
SFU_ErrorStatus    SFU_LL_SECU_SetProtectionMPU_UserApp(void);
#endif /* SFU_MPU_PROTECT_ENABLE */

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

#endif /* SFU_LOW_LEVEL_SECURITY_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
