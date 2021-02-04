/*!
 * \file      LmhpFirmwareManagement.h
 *
 * \brief     Implements the LoRa-Alliance firmware management rc2 package
 *            Specification: 
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *            
 *              (C)2019-2020 Actility
 *
 * \endcode
 *
 * \author    Oleg Tabarovskiy ( RTLS )
 */
#ifndef __LMHP_FWMANAGEMENT_H__
#define __LMHP_FWMANAGEMENT_H__

#include "LoRaMac.h"
#include "LmHandlerTypes.h"
#include "LmhPackage.h"

/*!
 * Firmware Management Protocol package identifier.
 *
 * \remark This value must be unique amongst the packages
 */
#define PACKAGE_ID_FWMANAGEMENT                    4

/*!
 * Device hardware version returned in DevVersionAns
 * and used to validate firmware upgrade
 */
#define __HW_VERSION_VEND3   (0x01U) /*!< [31:24] main version */
#define __HW_VERSION_VEND2   (0xBAU) /*!< [23:16] sub1 version */
#define __HW_VERSION_VEND1   (0x35U) /*!< [15:8]  sub2 version */
#define __HW_VERSION_DEV     (0x01U) /*!< [7:0]  release candidate */
#define __HW_VERSION         ((__HW_VERSION_VEND3 <<24)\
                                             |(__HW_VERSION_VEND2 << 16)\
                                             |(__HW_VERSION_VEND1 << 8 )\
                                             |(__HW_VERSION_DEV))

#define FWM_APPLY_ASAP       0x00000000
#define FWM_CANCEL_UPGRADE   0xFFFFFFFF

#define	FWM_DEL_ERRORINVALIDVERSION		0x0002
#define FWM_DEL_ERRORNOVALIDIMAGE		0x0001

typedef enum { // TODO: To be moved to image validation include
  UPIMG_STATUS_ABSENT = 0,
  UPIMG_STATUS_CORRUPT= 1,
  UPIMG_STATUS_WRONG  = 2,
  UPIMG_STATUS_VALID  = 3,
} UpImageStatus_t;

/*!
 * Firmware management package parameters
 */
typedef struct LmhpFWManagementParams_s
{
    /*!
     * Validate new image after reception is finished 
     *
     * \param [IN] status Fragmentation session status [FRAG_SESSION_ONGOING,
     *                                                  FRAG_SESSION_FINISHED or
     *                                                  FragDecoder.Status.FragNbLost]
     * \param [IN] size   Received file size
     */
    void ( *ImageValidate )( void *params );
    /*!
     * Status of new firmware image validation
     */
    uint8_t NewImageValidateStatus;
    /*! 
     * New firmware image version
     */
    uint32_t NewImageFWVersion;
} LmhpFWManagementParams_t;

LmhPackage_t *LmhpFWManagementPackageFactory( void );

#endif // __LMHP_FWMANAGEMENT_H__
