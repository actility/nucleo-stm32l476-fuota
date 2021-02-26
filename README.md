# Introduction
This git repository contains all needed information to do LoRaWAN FUOTA (Firmware Update Over The Air) with Actility FUOTA service.  
EU868, US916 and AS923 ISM bands are supported.

Main steps are:
* [Get a device](#get-a-device)
* [How to compile a device firmware (for Windows)](#How-to-compile-a-device-firmware-for-Windows)
* [Load device firmware onto device](#Load-device-firmware-onto-device)
* [How to customize device devEUI/JoinEUI/AppKey/GenAppKey and ISM band](#How-to-customize-device-devEUIJoinEUIAppKeyGenAppKey-and-ISM-band)
* [Contact Actility Team to create a free account and play with FUOTA service](#Contact-Actility-Team-to-create-a-free-account-and-play-with-FUOTA-service)

# Get a validated device
Buy following parts:
* ST MCU development board NUCLEO-L476RG. https://www.st.com/en/evaluation-tools/nucleo-l476rg.html
* Semtech LoRa shield SX1272MB2DAS. https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1272mb2das  

and assemble the 2 parts together.

# How to compile a device firmware (for Windows)
* Install STM32Cube IDE: https://www.st.com/en/development-tools/stm32cubeide.html
* Clone this git repository (git@github.com:actility/nucleo-stm32l476-fuota.git or https://github.com/actility/nucleo-stm32l476-fuota.git)
*	Start STM32CubeIDE application
*	In `File > Open Project from File system`, press `Directory` and select the following project locations:  
    *	`<root_dir>\Projects\STM32L476RG-Nucleo\Applications\LoRaWAN_Fuota1\2_Images_SECoreBin\SW4STM32\2_Images_SECoreBin`  
    *	`<root_dir>\Projects\STM32L476RG-Nucleo\Applications\LoRaWAN_Fuota1\2_Images_SBSFU\SW4STM32\2_Images_SBSFU`  
    *	`<root_dir>\Projects\STM32L476RG-Nucleo\Applications\LoRaWAN_Fuota1\Fuota\SW4STM32\sx1272mb2das`  
*	In  `Project explorer` window, click on the “Build” button for projects:  
    *	`2_Images_SECoreBin`  
    *	`2_Images_SBSFU`  
*	In `Project explorer` window, under the project `sx1272mb2das`, double click on `Build Targets > all`

# Load device firmware onto device
* Install STM32Cube Programmer: https://www.st.com/en/development-tools/stm32cubeprog.html
* Start STM32Cube Programmer
    * Click on `Open file` and select `<root_dir>\Projects\STM32L476RG-Nucleo\Applications\LoRaWAN_Fuota1\Fuota\Binary\SBSFU_UserApp.bin`
    * Click `Download`. At this point, the firmware is loaded onto the device
    * Click `Disconnect`
* Connect to the serial port of the device (using TeraTerm for example) and check the device start successfully (port speed: 921600)

Side note:  
`<root_dir>\Projects\STM32L476RG-Nucleo\Applications\LoRaWAN_Fuota1\Fuota\Binary\UserApp.sfb` is the binary file to be used in FUOTA UI for FUOTA campaigns.

# How to customize device devEUI/JoinEUI/AppKey/GenAppKey and ISM band
* To customize the devEUI/JoinEUI/AppKey/GenAppKey
     * Edit `<root_dir>\Projects\STM32L476RG-Nucleo\Applications\LoRaWAN_Fuota1\Fuota\LoRaWAN\App\inc\Commissioning.h`
     * Modify one of these parameters: `LORAWAN_DEVICE_EUI`, `LORAWAN_JOIN_EUI`, `LORAWAN_APP_KEY`, `LORAWAN_GEN_APP_KEY`
     * Recompile the firmware by executing the last step of section [How to compile a device firmware (for Windows)](#How-to-compile-a-device-firmware-for-Windows)
* To customize the ISM band of the device
    * Start STM32Cube IDE
    * Right-click on project `sx1272mb2das`
    * Select `Properties > C/C++ General > Paths and Symbols`
    * In `Symbols` tab, adapt the symbols. For example, for US915, set `ACTIVE_REGION=LORAMAC_REGION_US915`, create symbol `REGION_US915` and delete symbol `REGION_EU868`
    * Recompile the firmware by executing the last step of section [How to compile a device firmware (for Windows)](#How-to-compile-a-device-firmware-for-Windows)
    
# Contact Actility Team to create a free account and play with FUOTA service
Send an email to partners.fuota@actility.com to ask for a free account and start playing with LoRaWAN FUOTA
