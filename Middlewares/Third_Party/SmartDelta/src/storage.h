/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2020 Actility
License: Revised BSD License, see LICENSE.TXT file include in the project
Description: Flash API
*/

#ifndef __STORAGE__
#define __STORAGE__

#include "stdint.h"
#include "stdbool.h"

/*
 * Size of buffer storage used in patch as uncompress buffer
 * and temporary storage to keep contents of rewritten
 * flash page. Should no less then MCU flash page size
 * and match compression ratio in patch processing
 */
#define RAM_STORAGE_SZ   2048
#define FLASH_BLANK_BYTE   (0xFF)
/*
 * Number of RAM_STORAGE_SZ len pages in SWAP slot where data fragments are received
 * Array of MAX_PAGES / 8 is used to store dirty pages information
 *
 * NOTE: You need to adjust it if SWAP slot size is changed !
 *
 */
#define MAX_PAGES		149

/*!
 * Length of firmware image header. Should be exactly the same
 * as header length used in SBSFU
 */
#define HEADER_OFFSET   512

#define STORAGE_CRITICAL_ENTER()    (__disable_irq())   /* TODO: it needs to use another method for enter and exit of critical section. It must be RTOS compatible.*/
#define STORAGE_CRITICAL_EXIT()     (__enable_irq())    /* TODO: it needs to use another method for enter and exit of critical section. It must be RTOS compatible.*/

#define STORAGE_RESOURCE_TAKE()     {if(storage_isbusy()) {return STR_BUSY;} else {storage_setbusy(true);}}
#define STORAGE_RESOURCE_GIVE()     {storage_setbusy(false);}

typedef enum {
  STORAGE_SLOT_ACTIVE = 0,  /* the base firmware*/
  STORAGE_SLOT_NEWIMG,      /* the new firmware to pass it to the bootloader to swap*/
  STORAGE_SLOT_SCRATCH,     /* place to unpack patch */
  STORAGE_SLOT_SOURCE,      /* place to download patch */
} storage_slot_t;

typedef enum storage_status_s {
    STR_OK = 0,
    STR_BAD_LEN,
    STR_BAD_OFFSET,
    STR_HAL_ERR,
    STR_INCONSISTENCY,
    STR_BADALIGN,
    STR_VER_ERR,
    STR_BUSY,
    STR_NOMEM,
} storage_status_t;

uint8_t FragDecoderActilityRead(uint32_t addr, uint8_t *data, uint32_t size);
uint8_t FragDecoderActilityWrite(uint32_t addr, uint8_t *data, uint32_t size);
storage_status_t move_image(storage_slot_t src, storage_slot_t dst, uint32_t size, uint8_t flag);
storage_status_t storage_check_blank_slot(storage_slot_t slot);
storage_status_t storage_crc32 (storage_slot_t slot, uint32_t offset, uint32_t length, uint32_t *crc_out);
void 			 storage_datafile_init (void);
storage_status_t storage_erase_slot(storage_slot_t slot);
uint32_t 	     storage_get_rambuf(uint8_t **ram_buf);
storage_status_t storage_get_slot_info(storage_slot_t slot, uint32_t *start, uint32_t *len);
storage_status_t storage_init(void);
bool 			 storage_isbusy(void);
storage_status_t storage_read(storage_slot_t slot, uint32_t offset, uint8_t* data, uint32_t len);
void 			 storage_setbusy(bool busy);
storage_status_t storage_write(storage_slot_t slot, uint32_t offset, uint8_t* data, uint32_t len);
storage_status_t storage_read_no_holes (storage_slot_t slot, uint32_t offset, uint8_t* data, uint32_t len);

#endif /* __STORAGE__ */

