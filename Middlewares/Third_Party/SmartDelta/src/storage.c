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

#include "string.h"
#include "storage.h"
#include "sfu_app_new_image.h"
#include "stm32l4xx_hal.h"
#include "FragDecoder.h"
#include "FlashMemHandler.h"
#include "util_console.h"
//-----------------------------------------------------------------------------------------------
static bool str_busy = false;
static SFU_FwImageFlashTypeDef *fw_active_area_p = NULL;
static SFU_FwImageFlashTypeDef *fw_newimg_area_p = NULL;
static SFU_FwImageFlashTypeDef *fw_scratch_area_p = NULL;
static SFU_FwImageFlashTypeDef *fw_source_area_p = NULL;

__attribute__((aligned (8))) static uint8_t ram_storage[RAM_STORAGE_SZ];

static uint8_t dirty_pages[MAX_PAGES/8 + 1];

static void dirty_pages_init (void);
static bool flash_is_empty(const void *psrc, uint32_t len);

//---------------------------------------------------------------------------------------------------------------
__weak uint32_t storage_GetSourceAreaInfo(SFU_FwImageFlashTypeDef *pArea) {
  return SFU_APP_GetSwapAreaInfo(pArea); //by default
}
//---------------------------------------------------------------------------------------------------------------
/**
 * @brief Helper function to move full image with alignment holes from
 * 		  one slot to another.
 * @param src : Source image slot consisting of fragments aligned on double word
 * @param dst : Destination image slot for image without alignment holes
 * @param size : Size of destination image
 * @retval storage_status_t STR_OK on success, anything
 * 		  else on failure
 */
storage_status_t move_image(storage_slot_t src, storage_slot_t dst, uint32_t size, uint8_t flag)
{
	storage_status_t st;
	uint32_t pos, sz, rbsz;
	uint8_t * ram_buf;

	rbsz = storage_get_rambuf(&ram_buf);
	if( (st = storage_erase_slot(dst)) == STR_OK ) {
		for( pos = 0; pos < size; pos += rbsz ) {
			sz = size - pos;
			if( sz > rbsz ) {
				sz = rbsz;
			}
			memset(ram_buf, 0xff, rbsz);
			if (flag) {
				if( (st = storage_read_no_holes(src, pos, ram_buf, sz)) != STR_OK ) {
					break;
				}
			} else {
				if( (st = storage_read(src, pos, ram_buf, sz)) != STR_OK ) {
					break;
				}
			}
			if( (st = storage_write(dst, pos, ram_buf, sz)) != STR_OK ) {
				break;
			}
		}
	}
	return st;
}

void storage_datafile_init (void) {

	storage_erase_slot(STORAGE_SLOT_SOURCE);
	dirty_pages_init();
}

storage_status_t storage_init(void) {
    STORAGE_RESOURCE_TAKE();
    storage_status_t str_st = STR_OK;

    uint32_t rc;
    static SFU_FwImageFlashTypeDef fw_active_area;
    static SFU_FwImageFlashTypeDef fw_newimg_area;
    static SFU_FwImageFlashTypeDef fw_scratch_area;
    static SFU_FwImageFlashTypeDef fw_source_area;

    if ((rc = SFU_APP_GetActiveAreaInfo(&fw_active_area)) != HAL_OK) {
      str_st = STR_HAL_ERR;
      goto storage_init_err;
    }

    if ((rc = SFU_APP_GetDownloadAreaInfo(&fw_newimg_area)) != HAL_OK) {
      str_st = STR_HAL_ERR;
      goto storage_init_err;
    }

    // at the moment scratch and newimg slots are same
    if ((rc = SFU_APP_GetDownloadAreaInfo(&fw_scratch_area)) != HAL_OK) {
      str_st = STR_HAL_ERR;
      goto storage_init_err;
    }

    if ((rc = storage_GetSourceAreaInfo(&fw_source_area)) != HAL_OK) {
      str_st = STR_HAL_ERR;
      goto storage_init_err;
    }

    if((rc = FlashMemHandlerFct.Init()) != HAL_OK) {
        str_st = STR_HAL_ERR;
        goto storage_init_err;
    }
    (void)rc;   // avoid warnings 
    fw_active_area_p = &fw_active_area;
    fw_newimg_area_p = &fw_newimg_area;
    fw_scratch_area_p = &fw_scratch_area;
    fw_source_area_p = &fw_source_area;

storage_init_err:
    STORAGE_RESOURCE_GIVE();
    return str_st;
}

/**
 * @brief Helper function to get pointer to temporary ram buffer
 * @param ram_buf : pointer where to write ram buffer start pointer
 * @retval size of ram buffer or 0 on failure
 */
uint32_t storage_get_rambuf(uint8_t **ram_buf) {

	*ram_buf = &ram_storage[0];
	return RAM_STORAGE_SZ;
}
//---------------------------------------------------------------------------------------------------------------
bool storage_isbusy(void) {
    return str_busy;
}
//---------------------------------------------------------------------------------------------------------------
void storage_setbusy(bool busy) {
    str_busy = busy;
}
//-----------------------------------------------------------------------------------------------
storage_status_t storage_get_slot_info(storage_slot_t slot, uint32_t *start, uint32_t *len) {
  storage_status_t str_st = STR_OK;
  SFU_FwImageFlashTypeDef *area_p = NULL;

  switch(slot) {
    case STORAGE_SLOT_ACTIVE:
      area_p = fw_active_area_p;
      break;

    case STORAGE_SLOT_NEWIMG:
      area_p = fw_newimg_area_p;
      break;

    case STORAGE_SLOT_SCRATCH:
      area_p = fw_scratch_area_p;
      break;

    case STORAGE_SLOT_SOURCE:
      area_p = fw_source_area_p;
      break;

    default:
      str_st = STR_INCONSISTENCY;
      goto storage_get_slot_info_err;
      break;
  }

  if(!area_p) {
    str_st = STR_INCONSISTENCY;
    goto storage_get_slot_info_err;
  }

  if(start) {*start = area_p->DownloadAddr;}
  if(len) {*len = area_p->MaxSizeInBytes;}

storage_get_slot_info_err:

  return str_st;
}
//-----------------------------------------------------------------------------------------------
storage_status_t storage_read(storage_slot_t slot, uint32_t offset, uint8_t* data, uint32_t len) {
  STORAGE_RESOURCE_TAKE();
  storage_status_t str_st = STR_OK;
  SFU_FwImageFlashTypeDef *area_p = NULL;

  switch(slot) {
    case STORAGE_SLOT_ACTIVE:
      area_p = fw_active_area_p;
      break;

    case STORAGE_SLOT_NEWIMG:
      area_p = fw_newimg_area_p;
      break;

    case STORAGE_SLOT_SCRATCH:
      area_p = fw_scratch_area_p;
      break;

    case STORAGE_SLOT_SOURCE:
      area_p = fw_source_area_p;
      break;

    default:
      str_st = STR_INCONSISTENCY;
      goto storage_read_err;
      break;
  }

  if(!area_p) {
    str_st = STR_INCONSISTENCY;
    goto storage_read_err;
  }

  HAL_StatusTypeDef rc;
  if ((rc = FlashMemHandlerFct.Read((uint8_t*)(area_p->DownloadAddr + offset), data, len)) != HAL_OK) {
	  str_st = STR_HAL_ERR;
  }
  (void)rc;  // avoid warnings 

storage_read_err:

  //print_frame(data, len);
  STORAGE_RESOURCE_GIVE();
  return str_st;
}
//-----------------------------------------------------------------------------------------------
storage_status_t storage_write(storage_slot_t slot, uint32_t offset, uint8_t* data, uint32_t len) {
  
  storage_status_t str_st = STR_OK;
  SFU_FwImageFlashTypeDef *area_p = NULL;
  HAL_StatusTypeDef rc;
  
  STORAGE_RESOURCE_TAKE();
  
  switch(slot) {
    case STORAGE_SLOT_ACTIVE:
      area_p = fw_active_area_p;
      break;

    case STORAGE_SLOT_NEWIMG:
      area_p = fw_newimg_area_p;
      break;

    case STORAGE_SLOT_SCRATCH:
      area_p = fw_scratch_area_p;
      break;

    case STORAGE_SLOT_SOURCE:
      area_p = fw_source_area_p;
      break;

    default:
      str_st = STR_INCONSISTENCY;
      goto storage_write_err;
      break;
  }

  if ((rc = FlashMemHandlerFct.Write((uint8_t*)(area_p->DownloadAddr + offset), data, len )) != HAL_OK) {
	  str_st = STR_HAL_ERR;
  }
  (void)rc;  // avoid warnings 

storage_write_err:
  STORAGE_RESOURCE_GIVE();
  return str_st;
}
//-----------------------------------------------------------------------------------------------
storage_status_t storage_check_blank_slot(storage_slot_t slot) {
    STORAGE_RESOURCE_TAKE();
    storage_status_t str_st = STR_OK;
    SFU_FwImageFlashTypeDef *area_p = NULL;

    switch(slot) {
      case STORAGE_SLOT_ACTIVE:
        area_p = fw_active_area_p;
        break;

      case STORAGE_SLOT_NEWIMG:
        area_p = fw_newimg_area_p;
        break;

      case STORAGE_SLOT_SCRATCH:
        area_p = fw_scratch_area_p;
        break;

      case STORAGE_SLOT_SOURCE:
        area_p = fw_source_area_p;
        break;

      default:
        str_st = STR_INCONSISTENCY;
        goto storage_check_blank_slot_err;
        break;
    }

    if(!area_p) {
      str_st = STR_INCONSISTENCY;
      goto storage_check_blank_slot_err;
    }

    if (!flash_is_empty((uint8_t*)area_p->DownloadAddr, area_p->MaxSizeInBytes)) {
      str_st = STR_VER_ERR;
    }

storage_check_blank_slot_err:
    STORAGE_RESOURCE_GIVE();
    return str_st;
}
//-----------------------------------------------------------------------------------------------
storage_status_t storage_erase_slot(storage_slot_t slot) {
    STORAGE_RESOURCE_TAKE();
    storage_status_t str_st = STR_OK;
    SFU_FwImageFlashTypeDef *area_p = NULL;

    switch(slot) {
      case STORAGE_SLOT_ACTIVE:
        area_p = fw_active_area_p;
        break;

      case STORAGE_SLOT_NEWIMG:
        area_p = fw_newimg_area_p;
        break;

      case STORAGE_SLOT_SCRATCH:
        area_p = fw_scratch_area_p;
        break;

      case STORAGE_SLOT_SOURCE:
        area_p = fw_source_area_p;
        break;

      default:
        str_st = STR_INCONSISTENCY;
        goto storage_erase_slot_err;
        break;
    }

    if(!area_p) {
      str_st = STR_INCONSISTENCY;
      goto storage_erase_slot_err;
    }

    HAL_StatusTypeDef rc;
    if ((rc = FlashMemHandlerFct.Erase_Size((void *)area_p->DownloadAddr, area_p->MaxSizeInBytes)) != HAL_OK) {
    	str_st = STR_HAL_ERR;
    }
    (void)rc;  // avoid warnings 
    
storage_erase_slot_err:

    STORAGE_RESOURCE_GIVE();
    return str_st;
}
//-----------------------------------------------------------------------------------------------

static void dirty_pages_init (void) {
	memset(dirty_pages, 0, sizeof(dirty_pages));
}

static void mark_page_dirty (uint32_t pg) {
	dirty_pages[ pg / 8 ] |= 1 << (pg % 8);
}

static bool page_is_dirty (uint32_t pg) {

	return (dirty_pages[ pg / 8 ] & (1 << (pg % 8))) != 0 ? true : false;
}

static bool flash_is_empty(const void *psrc, uint32_t len) {
  for(uint32_t byte = 0; byte < len; byte++) {
    if(*((uint8_t *)psrc + byte) != FLASH_BLANK_BYTE) {
      return false;
    }
  }
  return true;
}

static uint32_t original_frag_size = 0;

uint8_t FragDecoderActilityWrite(uint32_t addr, uint8_t *data, uint32_t size)
{
  uint32_t ptr, len, aligned, alignaddr, pg, pgaddr, nextpgaddr;
  uint16_t row, first;
  uint32_t rbsz;
  uint8_t * ram_buf;

  rbsz = storage_get_rambuf(&ram_buf);
  original_frag_size = size; /* will need it in image alignment holes cleanup process */
  storage_get_slot_info(STORAGE_SLOT_SOURCE, &ptr, &len);
  if( (addr % size) != 0 ) {
	  return (uint8_t) - 1; /* addr should be multiple of size */
  }
  row = addr / size;
  aligned = ((size - 1) / 8 + 1) * 8;	/* align fragment size to write it to flash correctly */
  alignaddr = aligned * row;
  if( alignaddr + aligned > len ) {
	  return (uint8_t) - 1; /* If datafile from aligned fragments will not fit SWAP slot */
  }
  pg = alignaddr / rbsz;
  pgaddr = pg * rbsz + ptr;
  nextpgaddr = pgaddr + rbsz;
  /*
   * First check that we are not trying to write fragment
   * on to the dirty page. Dirty page is one that passed read->erase->write
   * cycle at least once.
   * When we process main fragments we never write to the same place so
   * all pages are fresh and we will just write fragments.
   * Only when processing redundancy fragments we will start rewrite pages
   * and mark them dirty.
   * We also need to specially handle cases when current page is not dirty but next
   * is (for fragments that cross page boundary). In real life this will probably
   * never happen but for the simplicity we just mark current page dirty.
   *
   * And finally if flash for the fragment we are about to write is not empty
   * we should mark page dirty and process current page via read->erase->write page
   */

  if( page_is_dirty(pg) || page_is_dirty(pg + 1)
		  || !flash_is_empty((void *)(alignaddr + ptr), aligned) ) {

	  /*
	   * Fragment could start on one page and end on another.
	   * In this worst case we should read/erase/write fragment
	   * on 2 sequential pages
	   */
	  mark_page_dirty( pg );
	  if( FlashMemHandlerFct.Read((void *)pgaddr, ram_buf, rbsz) != HAL_OK ) {
		  return (uint8_t) - 1;
	  }
	  if ( FlashMemHandlerFct.Erase_Size((void *)pgaddr, rbsz) != HAL_OK ) {
		  return (uint8_t) - 1;
	  }
	  if( (alignaddr + aligned - 1) / rbsz > pg ) {
		  first = rbsz - (alignaddr % rbsz);
		  memcpy(ram_buf + rbsz - first, data, first);
		  if( FlashMemHandlerFct.Write((void *)pgaddr, ram_buf, rbsz) != HAL_OK ) {
		  	  return (uint8_t) - 1;
		  }
		  if( (page_is_dirty(pg) && !page_is_dirty(pg + 1))
				  && flash_is_empty((void *)nextpgaddr, aligned - first) ) {
			/*
			 * Current page is dirty and next is not and we just write remaining
			 * part of the fragment on the new page without erasing it and rewriting
			 * completely. Page is aligned on 64 bit boundary and fragment also so the remainder
			 * will be aligned as well and we do not specially care about "aligned - first"
			 */
			  memset(ram_buf, 0, aligned - first);
			  memcpy(ram_buf, data + first, size - first);
			  if( FlashMemHandlerFct.Write((void *)nextpgaddr, ram_buf, aligned - first) != HAL_OK ) {
				  return (uint8_t) - 1;
			  }
		  } else {
			  /*
			   * Next page is dirty or not empty so we should do read->erase-write cycle for
			   * it and put remaining part of the fragment at the beginning
			   * of the page. Mark page dirty to be sure.
			   */
			  mark_page_dirty( pg + 1 );
			  if( FlashMemHandlerFct.Read((void *)nextpgaddr, ram_buf, rbsz) != HAL_OK ) {
				  return (uint8_t) - 1;
			  }
			  memset(ram_buf, 0, aligned - first);
			  memcpy(ram_buf, data + first, size - first);
			  if ( FlashMemHandlerFct.Erase_Size((void *)nextpgaddr, rbsz) != HAL_OK ) {
				  return (uint8_t) - 1;
			  }
			  if( FlashMemHandlerFct.Write((void *)nextpgaddr, ram_buf, rbsz) != HAL_OK ) {
				  return (uint8_t) - 1;
			  }
		  }
	  } else {
		  /*
		   * Just write back dirty page. We do not need any special
		   * handling if the fragment is in the middle of dirty page
		   */
		  memset(ram_buf + alignaddr % rbsz, 0, aligned);
		  memcpy(ram_buf + alignaddr % rbsz, data, size);
		  if( FlashMemHandlerFct.Write((void *)pgaddr, ram_buf, rbsz) != HAL_OK ) {
		   	  return (uint8_t) - 1;
		  }
	  }
  } else {
	  memset(ram_buf, 0, aligned);
	  memcpy(ram_buf, data, size);
	  if( FlashMemHandlerFct.Write((void *)(alignaddr + ptr), ram_buf, aligned) != HAL_OK) {
		  return (uint8_t) - 1;
	  }
	  /*
	   * If we wrote fragment consisting of only FFs we should mark current page dirty
	   * otherwise we will not be able to distinguish it from the erased space
	   * and fail later
	   */
	  if( flash_is_empty((void *)(alignaddr + ptr), aligned) ) {
		  mark_page_dirty( pg );
	  }
  }
  return 0;
}

uint8_t FragDecoderActilityRead(uint32_t addr, uint8_t *data, uint32_t size)
{
  uint32_t ptr, len, aligned, alignaddr;
  uint16_t row;

  storage_get_slot_info(STORAGE_SLOT_SOURCE, &ptr, &len);
  row = addr / size;
  aligned = ((size - 1) / 8 + 1) * 8;	/* align fragment size to read it from flash correctly */
  alignaddr = aligned * row;
  if( alignaddr + aligned > len ) {
	  return (uint8_t) - 1; /* If datafile from aligned fragments will not fit SWAP slot */
  }
  if( FlashMemHandlerFct.Read((void *)(alignaddr + ptr), data, size) == HAL_OK ) {
	  return 0; // Success
  }
  return (uint8_t) - 1;
}

/**
 * @brief Helper function to read image date with alignment holes of any size
 * 		  and return buffer without them
 * @param slot : Source image slot consisting of fragments aligned on double word
 * @param offset : Offset in image when alignment holes removed
 * @param data : Buffer to store data without holes
 * @param size : Size of data to move to data buffer
 * @retval storage_status_t STR_OK on success, anything
 * 		  else on failure
 */
storage_status_t storage_read_no_holes (storage_slot_t slot, uint32_t offset, uint8_t* data, uint32_t len) {

	uint32_t ptr, aligned, row_offset, ssize;
	int32_t ll, sz;
	uint16_t row;

	ll = len;
	storage_get_slot_info(slot, &ptr, &ssize);
	row = offset / original_frag_size;
	row_offset = offset % original_frag_size;
	aligned = ((original_frag_size - 1) / 8 + 1) * 8;
	if( aligned * row + row_offset + ll > ssize ) {
		  return STR_BAD_LEN; /* If requested data is beyond the boundary of SWAP slot */
	}
	sz = original_frag_size - row_offset;
	if( ll < sz ) {
		sz = ll;
	}
	if( FlashMemHandlerFct.Read((void *)(row_offset + aligned * row + ptr), data, sz) != HAL_OK ) {
		return STR_HAL_ERR;
	}
	ll -= sz;
	while( ll > 0 ) {
		data += sz;
		row++;
		sz = original_frag_size;
		ll -= sz;
		if( ll < 0 ) {
			sz = ll + original_frag_size;
		}
		if( FlashMemHandlerFct.Read((void *)(aligned * row + ptr), data, sz) != HAL_OK ) {
				return STR_HAL_ERR;
		}
	}
	return STR_OK;
}

#define CRC32_BUFSZ	32

storage_status_t storage_crc32 (storage_slot_t slot, uint32_t offset, uint32_t length, uint32_t *crc_out) {


	  uint32_t len;
	  uint8_t buf[CRC32_BUFSZ];
	  storage_status_t st;

	  // The CRC calculation follows CCITT - 0x04C11DB7
	  const uint32_t reversedPolynom = 0xEDB88320;

	  // CRC initial value
	  uint32_t crc = 0xFFFFFFFF;
	  uint32_t ptr = offset;

	  while (length)
	  {
		  len = length > CRC32_BUFSZ ? CRC32_BUFSZ : length;
		  length -= len;
		  if ((st = storage_read_no_holes (slot, ptr, buf, len)) != STR_OK) {
			return st;
		  }
		  for (uint16_t i = 0; i < len; ++i)
		  {
			crc ^= (uint32_t)buf[i];
			for (uint16_t i = 0; i < 8; i++)
			{
			  crc = (crc >> 1) ^ (reversedPolynom & ~((crc & 0x01) - 1));
			}
		  }
		  ptr += len;
	  }
	  *crc_out = ~crc;
	  return STR_OK;
}

