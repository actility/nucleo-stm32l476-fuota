/*  _        _   _ _ _ _
   / \   ___| |_(_) (_) |_ _   _
  / _ \ / __| __| | | | __| | | |
 / ___ \ (__| |_| | | | |_| |_| |
/_/   \_\___|\__|_|_|_|\__|\__, |
                           |___/
    (C)2017 Actility
License: see LICENCE_SLA0ACT.TXT file include in the project
Description: FOTA Firmware Patching Support
*/

#ifndef __PATCH_H__
#define __PATCH_H__
//----------------------------------------------------------------------------
#include <stdint.h>
//----------------------------------------------------------------------------
typedef enum { patchDecoded, patchError, patchUnsupported, patchUnrecognized, patchCorrupt, patchWrong } patch_res_t;
#define PATCH_RES_STRINGS { "DECODED", "ERROR", "UNABLE", "UNKNOWN", "CORRUPT", "WRONG" }
patch_res_t patch(uint32_t len);

#define SCRATCH_BUF_SIZE  8

struct patch_context_s {
  uint32_t scratch_start;
  uint32_t scratch_len;
  uint32_t scratch_pos;
  uint32_t active_start;
  uint32_t active_len;
};

void patch_init(void);
storage_status_t move_image (storage_slot_t src, storage_slot_t dst, uint32_t size, uint8_t flag);
int32_t SmartDeltaVerifyHeader (const uint8_t * fwimagebody);
//----------------------------------------------------------------------------
#endif /* __PATCH_H__ */

