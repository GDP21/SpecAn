/*
** FILE NAME:	 usb_dfuspec.h
**
** PROJECT:	  	 Meta/DSP Software Library
**
** TITLE:		 USB Device Firmware Upgrade Class.
**
** AUTHOR:		 Ensigma Technologies, a division of Imagination Technologies
**
** DESCRIPTION:	 Specification definitions.
**
** MODIFICATION
** HISTORY:		  1.0
*/
#ifndef USB_DFUSPEC_H
#define USB_DFUSPEC_H

#if defined (WIN32)
#define __ATTR_PACKED__
#else
#define __ATTR_PACKED__		__attribute__ ((packed))
#endif

/* DFU class codes */
#define DFU_CLASS_CODE    0xfe
#define DFU_SUBCLASS_CODE 0x01

/* DFU functional descriptor values */
#define DFU_LENGTH          0x07
#define DFU_DESCRIPTOR_TYPE 0x21

/* DFU functional descriptor */
typedef struct usb_dfu_functional_descriptor_t
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bmAttributes;
    unsigned char wDetatchTimeOut[2];
    unsigned char wTransferSize[2];
}__ATTR_PACKED__ DFU_FUNCTIONAL_DESCRIPTOR;

#define DFU_FUNCTIONAL_DESCRIPTOR_LENGTH	DFU_LENGTH

/* DFU request codes.
 * Note: These have been extended to include the non-DFU spec request
 * DFU_DISCONNECT.
 */
enum DFU_REQUEST_CODES {
    DFU_DETATCH = 0,
    DFU_DNLOAD,
    DFU_UPLOAD,
    DFU_GETSTATUS,
    DFU_CLRSTATUS,
    DFU_GETSTATE,
    DFU_ABORT,
    DFU_DISCONNECT
};

/* DFU device states */
enum DFU_STATES {
    DFU_APP_IDLE = 0,
    DFU_APP_DETATCH,
    DFU_IDLE,
    DFU_DNLOAD_SYNC,
    DFU_DNBUSY,
    DFU_DNLOAD_IDLE,
    DFU_MANIFEST_SYNC,
    DFU_MANIFEST,
    DFU_MANIFEST_WAIT_RESET,
    DFU_UPLOAD_IDLE,
    DFU_ERROR,
    DFU_NUM_STATES
};

/* Invalid state definition.
 * This is not part of the DFU spec.
 */
#define DFU_INVALID_STATE 0xffffffff

/* DFU status codes */
enum DFU_STATUS_CODES {
    DFU_OK = 0,
    DFU_ERR_TARGET,
    DFU_ERR_FILE,
    DFU_ERR_WRITE,
    DFU_ERR_ERASE,
    DFU_ERR_CHECK_ERASED,
    DFU_ERR_PROG,
    DFU_ERR_VERIFY,
    DFU_ERR_ADDRESS,
    DFU_ERR_NOTDONE,
    DFU_ERR_FIRMWARE,
    DFU_ERR_VENDOR,
    DFU_ERR_USBR,
    DFU_ERR_POR,
    DFU_ERR_UNKNOWN,
    DFU_ERR_STALLEDPKT
};

#define DFU_RESERVED_BITS 0xa

/* The Attributes field of a DFU Functional Descriptor is a bitmask that
 * describes the capabilities of the device. Define bit masks and extraction
 * macros.
 */
#define DFU_DOWNLOAD_CAPABLE       0x01
#define DFU_UPLOAD_CAPABLE         0x02
#define DFU_MANIFESTATION_TOLERANT 0x04
#define DFU_IS_DOWNLOAD_CAPABLE(Attributes)       (Attributes & DFU_DOWNLOAD_CAPABLE)
#define DFU_IS_UPLOAD_CAPABLE(Attributes)         (Attributes & DFU_UPLOAD_CAPABLE)
#define DFU_IS_MANIFESTATION_TOLERANT(Attributes) ((Attributes) & DFU_MANIFESTATION_TOLERANT)

/* The DFU specification number we support */
#define DFU_SPEC_NUMBER 0x0100

/* The magic DFU signature */
#define DFU_SIGNATURE 0x444655

/* DFU suffix length */
#define DFU_SUFFIX_LENGTH 16

/* Length to exclude when computing CRC */
#define DFU_CRC_EXCLUDE 4

/* CRC initialization value */
#define DFU_CRC_INIT 0xffffffff

/* Macro to extract the PollTimeout member of a DFU_STATUS structure */
#define DFU_GET_POLL_TIMEOUT(Status)  \
    ((Status)->PollTimeout[0])      | \
    ((Status)->PollTimeout[1] << 8) | \
    ((Status)->PollTimeout[2] << 16)

/* Macro to extract the Signature memeber of a DFU_SUFFIX structure */
#define DFU_GET_SIGNATURE(Suffix)   \
    ((Suffix)->Signature[0])      | \
    ((Suffix)->Signature[1] << 8) | \
    ((Suffix)->Signature[2] << 16)

/* DFU status structure.
 * This is exchanged using a DFU_GETSTATUS request.
 */
typedef struct dfu_status_t
{
    unsigned char Status;
    unsigned char PollTimeout[3];
    unsigned char State;
    unsigned char String;
} DFU_STATUS_T, DFU_STATUS, *PDFU_STATUS;

/* DFU suffix structure.
 * This structure is appended to the end of a firmware binary file to
 * provide id and CRC information.
 */
typedef struct dfu_suffix_t
{
    unsigned long  Crc;
    unsigned char  Length;
    unsigned char  Signature[3];
    unsigned short Spec;
    unsigned short VendorId;
    unsigned short ProductId;
    unsigned short DeviceRelease;
} DFU_SUFFIX_T, DFU_SUFFIX, *PDFU_SUFFIX;

#endif /* USB_DFUSPEC_H */
