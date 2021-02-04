/**
  @page Fuota Readme file
 
  @verbatim
  ******************************************************************************
  * @file    Fuota/readme.txt 
  * @author  MCD Application Team
  * @brief   This application is a Fuota demo of a LoRa Object connecting to 
  *          a LoRa Network. 
  ******************************************************************************
  *
  * Copyright (c) 2018 STMicroelectronics. All rights reserved.
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                               www.st.com/SLA0044
  *
  ******************************************************************************
   @endverbatim

@par Example Description

This directory contains a set of source files that implements a simple FUOTA demo of an end 
device also known as a LoRa Object connecting to a Lora Network. The LoRa Object will be 
a STM32L476-Nucleo board and Lora Radio expansion board, optionnally a sensor board. 
By setting the LoRa Ids in comissioning.h file according to the LoRa Network requirements, 
the end device will :
   - send periodically frames to the LoRa network ( could be a sensor data).
   - be able to received from a LoRa Network a request to establish a Firmware Update Over The Air 
     campaign. This Fuota campaign shall only be done through a Class C mode Session.
  ******************************************************************************



@par Directory contents 


  - Fuota/LoRaWAN/App/inc/bsp.h               Header for bsp.c
  - Fuota/LoRaWAN/App/inc/Commissioning.h     End device commissioning parameters
  - Fuota/LoRaWAN/App/inc/debug.h             interface to debug functionally
  - Fuota/LoRaWAN/App/inc/hw.h                group all hw interface
  - Fuota/LoRaWAN/App/inc/hw_conf.h           file to manage Cube SW family used and debug switch
  - Fuota/LoRaWAN/App/inc/hw_gpio.h           Header for hw_gpio.c
  - Fuota/LoRaWAN/App/inc/hw_msp.h            Header for driver hw msp module
  - Fuota/LoRaWAN/App/inc/hw_rtc.h            Header for hw_rtc.c
  - Fuota/LoRaWAN/App/inc/hw_spi.h            Header for hw_spi.c
  - Fuota/LoRaWAN/App/inc/utilities_conf.h    configuration for utilities
  - Fuota/LoRaWAN/App/inc/vcom.h              interface to vcom.c
  - Fuota/LoRaWAN/App/inc/version.h           version file
  - Fuota/LoRaWAN/App/inc/FlashMemHandler.h   Header for Flash interface 
  - Fuota/LoRaWAN/App/inc/FwUpdateAgent.h     Header for Update Agent
  
  - Fuota/Core/inc/stm32lXxx_hal_conf.h       Library Configuration file
  - Fuota/Core/inc/stm32lXxx_hw_conf.h        Header for stm32lXxx_hw_conf.c
  - Fuota/Core/inc/stm32lXxx_it.h             Header for stm32lXxx_it.c
  
  - Fuota/LoRaWAN/App/src/bsp.c               manages the sensors on the application
  - Fuota/LoRaWAN/App/src/debug.c             debug driver
  - Fuota/LoRaWAN/App/src/hw_gpio.c           gpio driver
  - Fuota/LoRaWAN/App/src/hw_rtc.c            rtc driver
  - Fuota/LoRaWAN/App/src/hw_spi.c            spi driver
  - Fuota/LoRaWAN/App/src/main.c              Main program file
  - Fuota/LoRaWAN/App/src/vcom.c              virtual com port interface on Terminal
  - Fuota/LoRaWAN/App/src/FlashMemHandler.c   Flash Handlerv
  - Fuota/LoRaWAN/App/src/UpdateAgent.c       Firmware Update functionalities
  
  - Fuota/Core/src/stm32lXxx_hal_msp.c        stm32lXxx specific hardware HAL code
  - Fuota/Core/src/stm32lXxx_hw.c             stm32lXxx specific hardware driver code
  - Fuota/Core/src/stm32lXxx_it.c             stm32lXxx Interrupt handlers
 
@par Hardware and Software environment 


  - This example runs on STM32L476RG devices.
    
  - This application has been tested with STMicroelectronics:
    NUCLEO-L476RG RevC	
    boards and can be easily tailored to any other supported device 
    and development board.

  - STM32L476-Nucleo Set-up    
    - Connect the Nucleo board to your PC with a USB cable type A to mini-B 
      to ST-LINK connector (CN1).
    - Please ensure that the ST-LINK connector CN2 jumpers are fitted.
  -Set Up:


             --------------------------  V    V  --------------------------
             |      LoRa Object       |  |    |  |      LoRa Netork       |
             |                        |  |    |  |                        |
   ComPort<--|                        |--|    |--|                        |-->Web Server
             |                        |          |                        |
             --------------------------          --------------------------

@par How to use it ? 
In order to make the program work, you must do the following :
  - Open your preferred toolchain 
  - Rebuild all files and load your image into target memory
  - Run the example
  - Open two Terminals, each connected the respective LoRa Object
  - Terminal Config = 921600, 8b, 1 stopbit, no parity, no flow control ( in src/vcom.c)
   
 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
