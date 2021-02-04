/**
  ******************************************************************************
  * @file    sfu_fsm_states.h
  * @author  MCD Application Team
  * @brief   This file defines the SB_SFU Finite State Machine states.
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
#ifndef SFU_FSM_STATES_H
#define SFU_FSM_STATES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_sfu.h"

/** @addtogroup SFU_BOOT
  * @{
  */

/** @defgroup SFU_BOOT_State_Machine_States_definition SFU BOOT State Machine states definition
  * @brief  SFU BOOT State Machine states
  * @{
 */

/**
  * FSM states: this enum must start from 0.
  */
typedef enum
{
  SFU_STATE_CHECK_STATUS_ON_RESET = 0U, /*!< checks the reset cause and decide how to proceed     */
#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) || (SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER)
  SFU_STATE_CHECK_NEW_FW_TO_DOWNLOAD,   /*!< SFU checks if a local download must be handled       */
  SFU_STATE_DOWNLOAD_NEW_USER_FW,       /*!< downloads the encrypted firmware to be installed     */
#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) || (SECBOOT_LOADER == SECBOOT_USE_STANDALONE_LOADER) */
  SFU_STATE_VERIFY_USER_FW_STATUS,      /*!< checks the internal FLASH state to derive a status   */
  SFU_STATE_INSTALL_NEW_USER_FW,        /*!< decrypts and installs the firmware stored in slot #1 */
  SFU_STATE_VERIFY_USER_FW_SIGNATURE,   /*!< checks the validity of the active firmware           */
  SFU_STATE_EXECUTE_USER_FW,            /*!< launch the user application                          */
  SFU_STATE_RESUME_INSTALL_NEW_USER_FW, /*!< resume interrupted installation of firmware          */
  SFU_STATE_HANDLE_CRITICAL_FAILURE,    /*!< deal with the critical failures                      */
  SFU_STATE_REBOOT_STATE_MACHINE        /*!< a reboot is forced                                   */
} SFU_BOOT_StateMachineTypeDef; /*!< SFU BOOT State Machine States */

/**
  *  The following strings associated to the SM state are used for debugging purpose.
  *  WARNING: The string array must match perfectly with the SFU_BOOT_StateMachineTypeDef defined above.
  *           And the above enum must be a sequence starting from 0
  */
#ifdef SFU_DEBUG_MODE
#if defined(SFU_VERBOSE_DEBUG_MODE) && defined (SFU_BOOT_C)
char *m_aStateMachineStrings[] = { "Checking Status on Reset.",
#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)
                                   "Checking if new Fw Image available to download.",
                                   "Downloading new Fw Image.",
#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) */
                                   "Verifying Fw Image status.",
                                   "Installing new Fw Image.",
                                   "Verifying Fw Image signature.",
                                   "Executing Fw Image.",
                                   "Resuming installation of new Fw Image.",
                                   "Handling a critical failure.",
                                   "Rebooting the State Machine"
                                 };
#endif /* SFU_VERBOSE_DEBUG_MODE && SFU_BOOT_C */

#endif /* SFU_DEBUG_MODE */

/************ SFU SM States****************************************************/
#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)
#define IS_SFU_SM_STATE(STATE)  (((STATE) == SFU_STATE_CHECK_STATUS_ON_RESET)    || \
                                 ((STATE) == SFU_STATE_CHECK_NEW_FW_TO_DOWNLOAD) || \
                                 ((STATE) == SFU_STATE_DOWNLOAD_NEW_USER_FW)     || \
                                 ((STATE) == SFU_STATE_INSTALL_NEW_USER_FW)      || \
                                 ((STATE) == SFU_STATE_VERIFY_USER_FW_STATUS)    || \
                                 ((STATE) == SFU_STATE_VERIFY_USER_FW_SIGNATURE) || \
                                 ((STATE) == SFU_STATE_EXECUTE_USER_FW)          || \
                                 ((STATE) == SFU_STATE_RESUME_INSTALL_NEW_USER_FW)|| \
                                 ((STATE) == SFU_STATE_HANDLE_CRITICAL_FAILURE)  || \
                                 ((STATE) == SFU_STATE_REBOOT_STATE_MACHINE))   /*!< SFU SM States*/
#else
#define IS_SFU_SM_STATE(STATE)  (((STATE) == SFU_STATE_CHECK_STATUS_ON_RESET)    || \
                                 ((STATE) == SFU_STATE_INSTALL_NEW_USER_FW)      || \
                                 ((STATE) == SFU_STATE_VERIFY_USER_FW_STATUS)    || \
                                 ((STATE) == SFU_STATE_VERIFY_USER_FW_SIGNATURE) || \
                                 ((STATE) == SFU_STATE_EXECUTE_USER_FW)          || \
                                 ((STATE) == SFU_STATE_RESUME_INSTALL_NEW_USER_FW)|| \
                                 ((STATE) == SFU_STATE_HANDLE_CRITICAL_FAILURE)  || \
                                 ((STATE) == SFU_STATE_REBOOT_STATE_MACHINE))   /*!< SFU SM States*/
#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* SFU_FSM_STATES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

