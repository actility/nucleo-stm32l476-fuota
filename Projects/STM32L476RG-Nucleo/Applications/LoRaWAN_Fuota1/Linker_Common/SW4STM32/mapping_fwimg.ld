/*
*****************************************************************************
**

**  File        : LinkerScript.ld
**
**  Abstract    : Linker script for STM32L475VGTx Device with
**                1024KByte FLASH, 96KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
**  (c)Copyright Ac6.
**  You may use this file as-is or modify it according to the needs of your
**  project. Distribution of this file (unmodified or modified) is not
**  permitted. Ac6 permit registered System Workbench for MCU users the
**  rights to distribute the assembled, compiled & linked contents of this
**  file as part of an application binary file, provided that it is built
**  using the System Workbench for MCU toolchain.
**
*****************************************************************************
*/


/* Slots Regions must be aligned on 2048 bytes (0x800) */

/* swap region (256 kbytes) */
__ICFEDIT_region_SWAP_start__   = 0x080B4000;
__ICFEDIT_region_SWAP_end__     = 0x080FDFFF;

/* slot 0 region (256 kbytes) */
__ICFEDIT_region_SLOT_0_start__ = 0x0806A000;
__ICFEDIT_region_SLOT_0_end__   = 0x080B3FFF;

/* slot 1 region (256 kbytes) */
__ICFEDIT_region_SLOT_1_start__ = 0x08014000;
__ICFEDIT_region_SLOT_1_end__   = 0x0805DFFF;

