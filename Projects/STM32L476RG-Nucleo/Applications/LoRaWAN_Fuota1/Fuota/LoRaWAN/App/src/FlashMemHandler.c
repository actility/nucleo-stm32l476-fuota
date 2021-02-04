/**
  ******************************************************************************
  * @file    FlashMemHandler.c
  * @author  MCD Application Team
  * @brief   FLASH Memory Handler.
  *          This file provides set of firmware functions to manage Flash
  *          Interface functionalities.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright(c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup USER_APP User App Example
  * @{
  */

/** @addtogroup USER_APP_COMMON Common
  * @{
  */
/* Includes ------------------------------------------------------------------*/
#include "FlashMemHandler.h"
#include <stdbool.h>
#include "string.h"
#include "util_console.h"

/* Uncomment the line below if you want some debug logs */
#ifdef DEBUG
#define FLASHMEMHANDLER_DBG
#endif

#ifdef FLASHMEMHANDLER_DBG
#define FLASHMEMHANDLER_TRACE(...) PRINTF(__VA_ARGS__)
#else
#define FLASHMEMHANDLER_TRACE(...)
#endif /* FLASHMEMHANDLER_DBG */


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define NB_PAGE_SECTOR_PER_ERASE  2U    /*!< Nb page erased per erase */

/** @defgroup SFU_FLASH_Private_Variables Private Variables
  * @{
  */
static __IO uint32_t DoubleECC_Error_Counter = 0U;
static __IO bool DoubleECC_Check;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint32_t GetPage(uint32_t uAddr);
static uint32_t GetBank(uint32_t uAddr);
static uint32_t GetBankAddr(uint32_t bank);

static HAL_StatusTypeDef FlashMemHandler_Read(void *pSource, void *pDestination, uint32_t Length);
static HAL_StatusTypeDef FlashMemHandler_Write(void *pDestination, const void *pSource, uint32_t uLength);
static HAL_StatusTypeDef FlashMemHandler_Erase_Size(void *pStart, uint32_t uLength);
static HAL_StatusTypeDef FlashMemHandler_Init(void);


const struct  FlashMemHandlerFct_s FlashMemHandlerFct =
{
  .Init = FlashMemHandler_Init,
  .Erase_Size = FlashMemHandler_Erase_Size,
  .Write = FlashMemHandler_Write,
  .Read = FlashMemHandler_Read,

};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Gets the page of a given address
  * @param  uAddr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t uAddr)
{
  uint32_t page = 0U;

  if (uAddr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (uAddr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (uAddr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}
/**
  * @brief  Gets the bank of a given address
  * @param  uAddr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t uAddr)
{
  uint32_t bank = 0U;

  if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0U)
  {
    /* No Bank swap */
    if (uAddr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
    /* Bank swap */
    if (uAddr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }

  return bank;
}


/**
  * @brief  Gets the address of a bank
  * @param  Bank: Bank ID
  * @retval Address of the bank
  */
static uint32_t GetBankAddr(uint32_t Bank)
{
  if (Bank == FLASH_BANK_2)
  {
    return  FLASH_BASE + FLASH_BANK_SIZE;
  }
  else
  {
    return FLASH_BASE;
  }
}

/* Public functions ---------------------------------------------------------*/
/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval HAL Status.
  */
static HAL_StatusTypeDef FlashMemHandler_Init(void)
{
  HAL_StatusTypeDef ret = HAL_ERROR;

  /* Unlock the Program memory */
  if (HAL_FLASH_Unlock() == HAL_OK)
  {

    /* Clear all FLASH flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    /* Unlock the Program memory */
    if (HAL_FLASH_Lock() == HAL_OK)
    {
      ret = HAL_OK;
    }
#ifdef FLASHMEMHANDLER_DBG
    else
    {
      FLASHMEMHANDLER_TRACE("[FLASH_IF] Lock failure\r\n");
    }
#endif /* FLASHMEMHANDLER_DBG */
  }
#ifdef FLASHMEMHANDLER_DBG
  else
  {
    FLASHMEMHANDLER_TRACE("[FLASH_IF] Unlock failure\r\n");
  }
#endif /* FLASHMEMHANDLER_DBG */
  return ret;
}

/**
  * @brief  This function does an erase of n (depends on Length) pages in user flash area
  * @param  pStart: Start of user flash area
  * @param  uLength: number of bytes.
  * @retval HAL status.
  */
static HAL_StatusTypeDef FlashMemHandler_Erase_Size(void *pStart, uint32_t uLength)
{
  uint32_t page_error = 0U;
  uint32_t uStart = (uint32_t)pStart;
  FLASH_EraseInitTypeDef x_erase_init;
  HAL_StatusTypeDef e_ret_status = HAL_ERROR;
  uint32_t first_page = 0U, nb_pages = 0U;
  uint32_t chunk_nb_pages;
  uint32_t erase_command = 0U;
  uint32_t bank_number = 0U;

  /* Initialize Flash */
  e_ret_status = FlashMemHandler_Init();

  if (e_ret_status == HAL_OK)
  {
    /* Unlock the Flash to enable the flash control register access *************/
    if (HAL_FLASH_Unlock() == HAL_OK)
    {
      do
      {
        /* Get the 1st page to erase */
        first_page = GetPage(uStart);
        bank_number = GetBank(uStart);
        if (GetBank(uStart + uLength - 1U) == bank_number)
        {
          /* Get the number of pages to erase from 1st page */
          nb_pages = GetPage(uStart + uLength - 1U) - first_page + 1U;

          /* Fill EraseInit structure*/
          x_erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
          x_erase_init.Banks = bank_number;

          /* Erase flash per NB_PAGE_SECTOR_PER_ERASE to avoid watch-dog */
          do
          {
            chunk_nb_pages = (nb_pages >= NB_PAGE_SECTOR_PER_ERASE) ? NB_PAGE_SECTOR_PER_ERASE : nb_pages;
            x_erase_init.Page = first_page;
            x_erase_init.NbPages = chunk_nb_pages;
            first_page += chunk_nb_pages;
            nb_pages -= chunk_nb_pages;
            if (HAL_FLASHEx_Erase(&x_erase_init, &page_error) != HAL_OK)
            {
              HAL_FLASH_GetError();
              e_ret_status = HAL_ERROR;
            }
            /* Refresh Watchdog */
            /* WRITE_REG(IWDG->KR, IWDG_KEY_RELOAD); */
          }
          while (nb_pages > 0);
          erase_command = 1U;
        }
        else
        {
          uint32_t startbank2 = GetBankAddr(FLASH_BANK_2);
          nb_pages = GetPage(startbank2 - 1U) - first_page + 1U;
          x_erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
          x_erase_init.Banks       = bank_number;
          uLength = uLength  - (startbank2 - uStart);
          uStart = startbank2;

          /* Erase flash per NB_PAGE_SECTOR_PER_ERASE to avoid watch-dog */
          do
          {
            chunk_nb_pages = (nb_pages >= NB_PAGE_SECTOR_PER_ERASE) ? NB_PAGE_SECTOR_PER_ERASE : nb_pages;
            x_erase_init.Page = first_page;
            x_erase_init.NbPages = chunk_nb_pages;
            first_page += chunk_nb_pages;
            nb_pages -= chunk_nb_pages;
            if (HAL_FLASHEx_Erase(&x_erase_init, &page_error) != HAL_OK)
            {
              HAL_FLASH_GetError();
              e_ret_status = HAL_ERROR;
            }
            /* Refresh Watchdog */
            /* WRITE_REG(IWDG->KR, IWDG_KEY_RELOAD); */
          }
          while (nb_pages > 0);
        }
      }
      while (erase_command == 0);
      /* Lock the Flash to disable the flash control register access (recommended
      to protect the FLASH memory against possible unwanted operation) *********/
      HAL_FLASH_Lock();

    }
    else
    {
      e_ret_status = HAL_ERROR;
    }
  }

  return e_ret_status;
}

/**
  * @brief  This function writes a data buffer in flash (data are 64-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  pDestination: Start address for target location
  * @param  pSource: pointer on buffer with data to write
  * @param  uLength: Length of data buffer in byte. It has to be 64-bit aligned.
  * @retval HAL Status.
  */
static HAL_StatusTypeDef FlashMemHandler_Write(void *pDestination, const void *pSource, uint32_t uLength)
{
  HAL_StatusTypeDef e_ret_status = HAL_ERROR;
  uint32_t i = 0U;
  uint32_t pdata = (uint32_t)pSource;

  /* Initialize Flash */
  e_ret_status = FlashMemHandler_Init();

  if (e_ret_status == HAL_OK)
  {
    /* Unlock the Flash to enable the flash control register access *************/
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
      PRINTF("ERROR ==> Unlock not possible\r\n");
      return HAL_ERROR;
    }
    else
    {
      /* DataLength must be a multiple of 64 bit */
      for (i = 0U; i < uLength; i += 8U)
      {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
        be done by word */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)pDestination,  *((uint64_t *)(pdata + i))) == HAL_OK)
        {
          /* Check the written value */
          if (*(uint64_t *)pDestination != *(uint64_t *)(pdata + i))
          {
            /* Flash content doesn't match SRAM content */
            e_ret_status = HAL_ERROR;
            PRINTF("ERROR ==> Memory check failure\r\n");
            break;
          }
          /* Increment FLASH Destination address */
          pDestination = (void *)((uint32_t)pDestination + 8U);
        }
        else
        {
          /* Error occurred while writing data in Flash memory */
          e_ret_status = HAL_ERROR;
          PRINTF("ERROR ==> Memory write failure\r\n");
          break;
        }
      }
      /* Lock the Flash to disable the flash control register access (recommended
      to protect the FLASH memory against possible unwanted operation) *********/
      HAL_FLASH_Lock();
    }
  }
  return e_ret_status;
}

/**
  * @brief  This function reads flash
  * @param  pDestination: pointer on buffer with data to write
  * @param  pSource: Start address for flash location
  * @param  Length: Length in bytes of data buffer
  * @retval HAL_StatusTypeDef HAL_OK if successful, HAL_ERROR otherwise.
  */
static HAL_StatusTypeDef FlashMemHandler_Read(void *pSource, void *pDestination, uint32_t Length)
{
  HAL_StatusTypeDef e_ret_status = HAL_ERROR;

  DoubleECC_Error_Counter = 0U;
  DoubleECC_Check = true;
  memcpy(pDestination, pSource, Length);
  DoubleECC_Check = false;
  if (DoubleECC_Error_Counter == 0U)
  {
    e_ret_status = HAL_OK;
  }
  DoubleECC_Error_Counter = 0U;

  return e_ret_status;
}


/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
