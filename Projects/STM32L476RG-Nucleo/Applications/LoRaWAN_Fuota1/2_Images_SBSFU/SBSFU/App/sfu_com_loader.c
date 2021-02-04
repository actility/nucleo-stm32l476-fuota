/**
  ******************************************************************************
  * @file    sfu_com_loader.c
  * @author  MCD Application Team
  * @brief   Secure Firmware Update COM module.
  *          This file provides set of firmware functions to manage SFU Com
  *          functionalities for the local loader.
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
#include "main.h"
#include "sfu_low_level.h"
#include "sfu_low_level_security.h"
#include "sfu_com_loader.h"
#include "sfu_trace.h"


#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @defgroup  SFU_COM_LOADER  SFU Com Local Loader
  * @brief This file provides a set of  functions to manage the Comms interface used for download (local loader with
  * Ymodem protocol over UART).
  * @{
  */


/** @defgroup SFU_COM_LOADER_Private_Defines Private Defines
  * @{
  */
#if defined(__ICCARM__)
#define PUTCHAR_PROTOTYPE int putchar(int ch)
#elif defined(__CC_ARM)
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#elif defined(__GNUC__)
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#endif /* __ICCARM__ */

#define SFU_COM_LOADER_TIME_OUT            ((uint32_t )0x800U)   /*!< COM Transmit and Receive Timeout*/
#define SFU_COM_LOADER_SERIAL_TIME_OUT       ((uint32_t )100U)   /*!< Serial PutByte and PutString Timeout*/

/**
  * @}
  */

/** @defgroup SFU_COM_LOADER_Private_Macros Private Macros
  * @{
  */
#define IS_CAP_LETTER(c)    (((c) >= 'A') && ((c) <= 'F'))
#define IS_LC_LETTER(c)     (((c) >= 'a') && ((c) <= 'f'))
#define IS_09(c)            (((c) >= '0') && ((c) <= '9'))
#define ISVALIDHEX(c)       (IS_CAP_LETTER(c) || IS_LC_LETTER(c) || IS_09(c))
#define ISVALIDDEC(c)       IS_09(c)
#define CONVERTDEC(c)       (c - '0')

#define CONVERTHEX_ALPHA(c) (IS_CAP_LETTER(c) ? ((c) - 'A'+10) : ((c) - 'a'+10))
#define CONVERTHEX(c)       (IS_09(c) ? ((c) - '0') : CONVERTHEX_ALPHA(c))
/**
  * @}
  */


/** @defgroup SFU_COM_LOADER_Private_Variables Private Variables
  * @{
  */
/*!<Array used to store Packet Data*/
static uint8_t m_aPacketData[SFU_COM_YMODEM_PACKET_1K_SIZE + SFU_COM_YMODEM_PACKET_DATA_INDEX +
                             SFU_COM_YMODEM_PACKET_TRAILER_SIZE] __attribute__((aligned(4)));
/*!< Array used to store File Name data */
uint8_t m_aFileName[SFU_COM_YMODEM_FILE_NAME_LENGTH + 1U];

/**
  * @}
  */

/** @defgroup SFU_COM_LOADER_Private_Functions Private Functions
  * @{
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *pData, uint32_t *puLength, uint32_t uTimeOut);
static uint32_t Str2Int(uint8_t *pInputStr, uint32_t *pIntNum);
/**
  * @}
  */

/** @defgroup SFU_COM_LOADER_Exported_Functions Exported Functions
  * @{
  */

/** @defgroup SFU_COM_LOADER_Initialization_Functions Initialization Functions
  * @brief If the debug mode is not enabled then the local loader must also provide the COM init functions
  * @{
  */

#if !defined(SFU_DEBUG_MODE)
/**
  * @brief  SFU Com Init function.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_Init(void)
{
#if defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0);
#endif /* __GNUC__ */
  return SFU_LL_UART_Init();
}

/**
  * @brief  SFU Com DeInit function.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_DeInit(void)
{
  return SFU_LL_UART_DeInit();
}

#endif /* SFU_DEBUG_MODE */
/**
  * @}
  */

/** @defgroup SFU_COM_LOADER_Control_Functions Control Functions
  * @{
  */

/**
  * @brief  SFU COM Transmit data.
  * @param  pData: pointer to the data to write.
  * @param  uDataLength: the length of the data to be written in bytes.
  * @param  uTimeOut: Timeout duration
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_Transmit(uint8_t *pData, uint16_t uDataLength, uint32_t uTimeOut)
{
  return SFU_LL_UART_Transmit(pData, uDataLength, uTimeOut);
}

/**
  * @brief  SFU COM Receive data.
  * @param  pData: pointer to the data to read.
  * @param  uDataLength: the length of the data to be read in bytes.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_Receive(uint8_t *pData, uint16_t uDataLength)
{
  return SFU_LL_UART_Receive(pData, uDataLength, SFU_COM_LOADER_TIME_OUT);
}

/**
  * @brief  SFU COM Flush data.
  * @param  None.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_Flush(void)
{
  return SFU_LL_UART_Flush();
}

/**
  * @brief  Receive a file using the ymodem protocol with SFU_COM_YMODEM_CRC16.
  * @param  peCOMStatus: SFU_COM_YMODEM_StatusTypeDef result of reception/programming.
  * @param  puSize: size of received file.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise
  */
SFU_ErrorStatus SFU_COM_YMODEM_Receive(SFU_COM_YMODEM_StatusTypeDef *peCOMStatus, uint32_t *puSize)
{
  uint32_t i;
  uint32_t packet_length = 0U;
  uint32_t session_done = 0U;
  uint32_t file_done = 0U;
  uint32_t errors = 0U;
  uint32_t session_begin = 0U;
  uint32_t ramsource = 0U;
  uint32_t filesize = 0U;
  uint32_t packets_received = 0;
  uint8_t *file_ptr;
  uint8_t file_size[SFU_COM_YMODEM_FILE_SIZE_LENGTH + 1U];
  uint8_t tmp = 0U;

  /* Check the pointers allocation */
  if ((peCOMStatus == NULL) || (puSize == NULL))
  {
    return SFU_ERROR;
  }

  *peCOMStatus = SFU_COM_YMODEM_OK;

  while ((session_done == 0U) && (*peCOMStatus == SFU_COM_YMODEM_OK))
  {
    packets_received = 0U;
    file_done = 0U;
    while ((file_done == 0U) && (*peCOMStatus == SFU_COM_YMODEM_OK))
    {
      switch (ReceivePacket(m_aPacketData, &packet_length, SFU_COM_YMODEM_DOWNLOAD_TIMEOUT))
      {
        case HAL_OK:
          errors = 0U;
          switch (packet_length)
          {
            case 3U:
              /* Startup sequence */
              break;
            case 2U:
              /* Abort by sender */
              SFU_COM_Serial_PutByte(SFU_COM_YMODEM_ACK);
              *peCOMStatus = SFU_COM_YMODEM_ABORT;
              break;
            case 0U:
              /* End of transmission */
              SFU_COM_Serial_PutByte(SFU_COM_YMODEM_ACK);
              *puSize = filesize;
              file_done = 1U;
              break;
            default:
              /* Normal packet */
              if (m_aPacketData[SFU_COM_YMODEM_PACKET_NUMBER_INDEX] != (packets_received & 0xff))
              {
                /* No NACK sent for a better synchro with remote : packet will be repeated */
              }
              else
              {
                if (packets_received == 0U)
                {
                  /* File name packet */
                  if (m_aPacketData[SFU_COM_YMODEM_PACKET_DATA_INDEX] != 0U)
                  {
                    /* File name extraction */
                    i = 0U;
                    file_ptr = m_aPacketData + SFU_COM_YMODEM_PACKET_DATA_INDEX;
                    while ((*file_ptr != 0U) && (i < SFU_COM_YMODEM_FILE_NAME_LENGTH))
                    {
                      m_aFileName[i++] = *file_ptr++;
                    }

                    /* File size extraction */
                    m_aFileName[i++] = '\0';
                    i = 0U;
                    file_ptr ++;
                    while ((*file_ptr != ' ') && (i < SFU_COM_YMODEM_FILE_SIZE_LENGTH))
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    Str2Int(file_size, &filesize);

                    /* Header packet received callback call*/
                    if (SFU_COM_YMODEM_HeaderPktRxCpltCallback((uint32_t) filesize) == SFU_SUCCESS)
                    {
                      SFU_COM_Serial_PutByte(SFU_COM_YMODEM_ACK);
                      SFU_LL_UART_Flush();
                      SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CRC16);
                    }
                    else
                    {
                      /* End session */
                      tmp = SFU_COM_YMODEM_CA;
                      SFU_COM_Transmit(&tmp, 1U, SFU_COM_YMODEM_NAK_TIMEOUT);
                      SFU_COM_Transmit(&tmp, 1U, SFU_COM_YMODEM_NAK_TIMEOUT);
                      return SFU_ERROR;
                    }
                  }
                  /* File header packet is empty, end session */
                  else
                  {
                    SFU_COM_Serial_PutByte(SFU_COM_YMODEM_ACK);
                    file_done = 1U;
                    session_done = 1U;
                    break;
                  }
                }
                else /* Data packet */
                {
                  ramsource = (uint32_t) & m_aPacketData[SFU_COM_YMODEM_PACKET_DATA_INDEX];

                  /* Data packet received callback call*/
                  if (SFU_COM_YMODEM_DataPktRxCpltCallback((uint8_t *) ramsource, (uint32_t) packet_length) ==
                      SFU_SUCCESS)
                  {
                    SFU_COM_Serial_PutByte(SFU_COM_YMODEM_ACK);

                  }
                  else /* An error occurred while writing to Flash memory */
                  {
                    /* End session */
                    SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CA);
                    SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CA);
                    *peCOMStatus = SFU_COM_YMODEM_DATA;
                  }

                }
                packets_received ++;
                session_begin = 1;
              }
              break;
          }
          break;
        case HAL_BUSY: /* Abort actually */
          SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CA);
          SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CA);
          *peCOMStatus = SFU_COM_YMODEM_ABORT;
          break;
        default:
          if (session_begin > 0U)
          {
            errors ++;
          }
          if (errors > SFU_COM_YMODEM_MAX_ERRORS)
          {
            /* Abort communication */
            SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CA);
            SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CA);
            *peCOMStatus = SFU_COM_YMODEM_ABORT;
          }
          else
          {
            SFU_COM_Serial_PutByte(SFU_COM_YMODEM_CRC16); /* Ask for a packet */
            TRACE("\b.");                                 /* Replace C char by . on display console */
            /*
             * Toggling the LED in case no console is available: the toggling frequency depends on
             * SFU_COM_YMODEM_DOWNLOAD_TIMEOUT
             */
            BSP_LED_Toggle(SFU_STATUS_LED);
          }
          break;
      }
    }
  }
  /* Make sure the status LED is turned off */
  BSP_LED_Off(SFU_STATUS_LED);

  if (*peCOMStatus == SFU_COM_YMODEM_OK)
  {
    return SFU_SUCCESS;
  }
  else
  {
    return SFU_ERROR;
  }
}

/**
  * @brief  Transmit a byte to the COM Port.
  * @param  uParam: The byte to be sent.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_COM_Serial_PutByte(uint8_t uParam)
{
  return SFU_LL_UART_Transmit(&uParam, 1U, SFU_COM_LOADER_SERIAL_TIME_OUT);
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup SFU_COM_LOADER_Private_Functions
  * @{
  */

/**
  * @brief  Convert a string to an integer.
  * @param  pInputStr: The string to be converted.
  * @param  pIntNum: The integer value.
  * @retval 1: Correct
  *         0: Error
  */
static uint32_t Str2Int(uint8_t *pInputStr, uint32_t *pIntNum)
{
  uint32_t i = 0U;
  uint32_t res = 0U;
  uint32_t val = 0U;

  if ((pInputStr[0] == '0') && ((pInputStr[1] == 'x') || (pInputStr[1] == 'X')))
  {
    i = 2U;
    while ((i < 11U) && (pInputStr[i] != '\0'))
    {
      if (ISVALIDHEX(pInputStr[i]))
      {
        val = (val << 4U) + CONVERTHEX(pInputStr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0U;
        break;
      }
      i++;
    }

    /* valid result */
    if (pInputStr[i] == '\0')
    {
      *pIntNum = val;
      res = 1U;
    }
  }
  else /* max 10-digit decimal input */
  {
    while ((i < 11U) && (res != 1U))
    {
      if (pInputStr[i] == '\0')
      {
        *pIntNum = val;
        /* return 1 */
        res = 1U;
      }
      else if (((pInputStr[i] == 'k') || (pInputStr[i] == 'K')) && (i > 0U))
      {
        val = val << 10U;
        *pIntNum = val;
        res = 1U;
      }
      else if (((pInputStr[i] == 'm') || (pInputStr[i] == 'M')) && (i > 0U))
      {
        val = val << 20U;
        *pIntNum = val;
        res = 1U;
      }
      else if (ISVALIDDEC(pInputStr[i]))
      {
        val = val * 10U + CONVERTDEC(pInputStr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0U;
        break;
      }
      i++;
    }
  }

  return res;
}

/**
  * @brief  Receive a packet from sender
  * @param  pData: pointer to received data.
  * @param  puLength
  *     0: end of transmission
  *     2: abort by sender
  *    >0: packet length
  * @param  uTimeOut: receive timeout (ms).
  * @retval HAL_OK: normally return
  *         HAL_BUSY: abort by user
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *pData, uint32_t *puLength, uint32_t uTimeout)
{
  uint32_t crc;
  uint32_t packet_size = 0U;
  HAL_StatusTypeDef status;
  SFU_ErrorStatus eRetStatus;

  uint8_t char1;

  *puLength = 0U;

  /* This operation could last long. Need to refresh the Watchdog if enabled. It could be implemented as a callback*/
  SFU_LL_SECU_IWDG_Refresh();

  eRetStatus = SFU_LL_UART_Receive(&char1, 1, uTimeout);

  if (eRetStatus == SFU_SUCCESS)
  {
    status = HAL_OK;

    switch (char1)
    {
      case SFU_COM_YMODEM_SOH:
        packet_size = SFU_COM_YMODEM_PACKET_SIZE;
        break;
      case SFU_COM_YMODEM_STX:
        packet_size = SFU_COM_YMODEM_PACKET_1K_SIZE;
        break;
      case SFU_COM_YMODEM_EOT:
        break;
      case SFU_COM_YMODEM_CA:
        if ((SFU_LL_UART_Receive(&char1, 1U, uTimeout) == SFU_SUCCESS) && (char1 == SFU_COM_YMODEM_CA))
        {
          packet_size = 2U;
        }
        else
        {
          status = HAL_ERROR;
        }
        break;
      case SFU_COM_YMODEM_ABORT1:
      case SFU_COM_YMODEM_ABORT2:
        status = HAL_BUSY;
        break;
      case SFU_COM_YMODEM_RB:
        SFU_LL_UART_Receive(&char1, 1U, uTimeout);             /* Ymodem starup sequence : rb ==> 0x72 + 0x62 + 0x0D */
        SFU_LL_UART_Receive(&char1, 1U, uTimeout);
        packet_size = 3U;
        break;
      default:
        status = HAL_ERROR;
        break;
    }
    *pData = char1;

    if (packet_size >= SFU_COM_YMODEM_PACKET_SIZE)
    {

      eRetStatus = SFU_LL_UART_Receive(&pData[SFU_COM_YMODEM_PACKET_NUMBER_INDEX],
                                       packet_size + SFU_COM_YMODEM_PACKET_OVERHEAD_SIZE, uTimeout);

      /* Simple packet sanity check */
      if (eRetStatus == SFU_SUCCESS)
      {
        status = HAL_OK;

        if (pData[SFU_COM_YMODEM_PACKET_NUMBER_INDEX] != ((pData[SFU_COM_YMODEM_PACKET_CNUMBER_INDEX]) ^
                                                          SFU_COM_YMODEM_NEGATIVE_BYTE))
        {
          packet_size = 0U;
          status = HAL_ERROR;
        }
        else
        {
          /* Check packet CRC*/
          crc = pData[ packet_size + SFU_COM_YMODEM_PACKET_DATA_INDEX ] << 8U;
          crc += pData[ packet_size + SFU_COM_YMODEM_PACKET_DATA_INDEX + 1U ];

          /*Configure CRC with 16-bit polynomial*/
          if (SFU_LL_CRC_Config(SFU_CRC_CONFIG_16BIT) == SFU_SUCCESS)
          {
            if (SFU_LL_CRC_Calculate((uint32_t *)&pData[SFU_COM_YMODEM_PACKET_DATA_INDEX], packet_size) != crc)
            {
              packet_size = 0U;
              status = HAL_ERROR;
            }

          }
          else
          {
            packet_size = 0U;
            status = HAL_ERROR;
          }
        }
      }
      else
      {
        status = HAL_ERROR;
      }
    }
  }
  else
  {
    status = HAL_ERROR;
  }

  *puLength = packet_size;
  return status;
}

/**
  * @}
  */

/** @defgroup SFU_COM_LOADER_Callback_Functions Callback Functions
  * @{
  */
/**
  * @brief  Ymodem Header Packet Transfer completed callback.
  * @param  uFileSize: Dimension of the file that will be received.
  * @retval None
  */
__weak SFU_ErrorStatus SFU_COM_YMODEM_HeaderPktRxCpltCallback(uint32_t uFileSize)
{

  /* NOTE : This function should not be modified, when the callback is needed,
            the SFU_COM_YMODEM_HeaderPktRxCpltCallback could be implemented in the user file
   */
  return SFU_SUCCESS;
}

/**
  * @brief  Ymodem Data Packet Transfer completed callback.
  * @param  pData: Pointer to the buffer.
  * @param  uSize: Packet dimension.
  * @retval None
  */
__weak SFU_ErrorStatus SFU_COM_YMODEM_DataPktRxCpltCallback(uint8_t *pData, uint32_t uSize)
{

  /* NOTE : This function should not be modified, when the callback is needed,
            the SFU_COM_YMODEM_DataPktRxCpltCallback could be implemented in the user file
   */
  return SFU_SUCCESS;
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
