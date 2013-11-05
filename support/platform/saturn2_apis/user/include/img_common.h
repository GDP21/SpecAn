/*!
******************************************************************************
 @file   : img_common.h

 @brief

 @Author Ray Livesley

 @date   02/06/2003

         <b>Copyright 2003 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 \n<b>Description:</b>\n
         This file contains the IMG common structures and definitions.

 \n<b>Platform:</b>\n
         Platform Independent

 @Version
    -   1.1 1st Release under Doxygen.
    -   1.2 Changed the description of buffer format and image type.
    -   1.3 Update control block structure.
    -   1.4 Added IMG_INPUT_SLEEP to IMG_eInputActiveSettings.
    -   1.5 Add IMG_sScalerConfigData.
    -   1.6 Add KRN_POOLLINK the control block structure.
    -   1.7 Removed Sleep Mode input type
    -   1.8 Add field in IMG_sImageBufCB for application specific data.
    -   1.9 Add Neutrino pixel formats
    -   1.10 Add information to image control block for stretch blit.
    -   1.11 Added IMG_CSC_CUSTOM to the CSC Enumerated Types.
    -   1.12 Add fields to IMG_sImageBufCB to support secondary control blocks.

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: img_common.h,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 
 
  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

 Added LOGMAN API

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 
 Sp.

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

 This is major commit #1 of 3. There will be another major commit based
 upon the IMG Framework application and libraries, as well as another
 for scripts and tagging.

 -----> THIS IS NOT YET A USEABLE STRUCTURE <-----

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


*****************************************************************************/
#if !defined(IMG_KERNEL_MODULE)
  #include <time.h>
#endif
#include "img_defs.h"

#if defined (__META_MEOS__) || defined (__MTX_MEOS__)
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>
#else
#include "lst.h"
#endif

#if !defined (__IMG_COMMON_H__)
#define __IMG_COMMON_H__

#if defined (__cplusplus)
extern "C" {
#endif

#define SCC_MAXTAP              9
#define SCC_MAXINTPT            16

/*!
******************************************************************************

 This type defines a handle to a callback structure

******************************************************************************/
typedef void *          IMG_hCallback;

#define IMG_EVENT_ALL       (0xFFFFFFFF)        /*!< Event handler will be called for all events. */
#define IMG_EVENT_DEFAULT   (0xFFFFFFFE)        /*!< Event handler will only be called if no other handler is registered. */

/* Used by sysbrg auto code generation */
#define __DO_NOT_BRIDGE__
#define __PTR_AND_SIZE__

/*!
******************************************************************************
 This type defines the pixel formats

 NOTE: The pixel formats have been assigned numeric values to when adding new
 formats thenext free value should be used.  The exiting assignment should be
 retained so the .img image files will  be correctly processed.
******************************************************************************/
typedef enum
{
    IMG_PIXFMT_CLUT1        =   0,              //!< 1-bit palettised.
    IMG_PIXFMT_CLUT2        =   1,              //!< 2-bit palettised.
    IMG_PIXFMT_CLUT4        =   2,              //!< 4-bit palettised.
    IMG_PIXFMT_I4A4         =   3,				//!< 4-bit palettised + 4-bit alpha.
    IMG_PIXFMT_I8A8         =   4,				//!< 8-bit palettised + 8-bit alpha.
	IMG_PIXFMT_A8I8         =   51,				//!< 8-bit palettised + 8-bit alpha.    
	IMG_PIXFMT_RGB8         =   5,              //!< 8-bit palettised.
    IMG_PIXFMT_RGB332       =   6,              //!< 8-bit RGB (R=3, G=3, B=2).
    IMG_PIXFMT_RGB555       =   7,              //!< 15-bit RGB (R=5, G=5, B=5).
    IMG_PIXFMT_ARGB4444     =   8,              //!< 16-bit RGB (R=4, G=4, B=4) + 4-bit alpha.
    IMG_PIXFMT_ARGB1555     =   9,              //!< 16-bit RGB (R=5, G=5, B=5) + 1-bit alpha.
    IMG_PIXFMT_RGB565       =   10,             //!< 16-bit RGB (R=5, G=6, B=5).
    IMG_PIXFMT_RGB888       =   11,             //!< 24-bit RGB (packed).
    IMG_PIXFMT_ARGB8888     =   12,             //!< 24-bit RGB + 8-bit alpha.

    IMG_PIXFMT_UYVY8888     =   13,             //!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_VYUY8888     =   14,				//!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_YVYU8888     =   15,				//!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_YUYV8888     =   16,				//!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_UYVY10101010 =   17,             //!< YUV 4:2:2 10-bit per component.
    IMG_PIXFMT_VYAUYA8888   =   18,             //!< YUV 4:2:2:4 8-bit per component.
    IMG_PIXFMT_YUV101010    =   19,             //!< YUV 4:4:4 10-bit per component.
    IMG_PIXFMT_AYUV4444     =   20,             //!< 12-bit YUV 4:4:4 + 4-bit alpha.
    IMG_PIXFMT_YUV888       =   21,             //!< 24-bit YUV (packed).
    IMG_PIXFMT_AYUV8888     =   22,             //!< 24-bit YUV 4:4:4 + 8-bit alpha.
    IMG_PIXFMT_AYUV2101010  =   23,             //!< 30-bit YUV 4:4:4 + 2-bit alpha.

	IMG_PIXFMT_411PL12YVU8  =   24,             //!< Planar 8-bit 4:1:1 format.

    IMG_PIXFMT_420PL12YUV8  =   25,             //!< Planar 8-bit 4:2:0 format.
    IMG_PIXFMT_420PL12YVU8  =   26,             //!< Planar 8-bit 4:2:0 format - as per YUV8 but with reversed U and V samples.
    IMG_PIXFMT_422PL12YUV8  =   27,             //!< Planar 8-bit 4:2:2 format.
    IMG_PIXFMT_422PL12YVU8  =   28,             //!< Planar 8-bit 4:2:2 format - as per YUV8 but with reversed U and V samples.
    IMG_PIXFMT_420PL12YUV10 =   29,             //!< Planar 10-bit 4:2:0 format.
    IMG_PIXFMT_420PL12YVU10 =   45,             //!< Planar 10-bit 4:2:0 format - as per YUV10 but with reversed U and V samples.
    IMG_PIXFMT_422PL12YUV10 =   30,             //!< Planar 10-bit 4:2:2 format.
    IMG_PIXFMT_422PL12YVU10 =   46,             //!< Planar 10-bit 4:2:2 format - as per YUV10 but with reversed U and V samples.

    IMG_PIXFMT_420PL8YUV8   =   47,             //!< Planar 8-bit 4:2:0 format - (Y, U and V are in separate planes).
    IMG_PIXFMT_422PL8YUV8   =   48,             //!< Planar 8-bit 4:2:2 format - (Y, U and V are in separate planes).
    IMG_PIXFMT_420PL8YUV10  =   49,             //!< Planar 10-bit 4:2:0 format - (Y, U and V are in separate planes).
    IMG_PIXFMT_422PL8YUV10  =   50,             //!< Planar 10-bit 4:2:2 format - (Y, U and V are in separate planes).

    IMG_PIXFMT_420PL12YUV8_A8 =   31,           //!< Planar 8-bit 4:2:0 format + 8-bit alpha.
    IMG_PIXFMT_422PL12YUV8_A8 =   32,           //!< Planar 8-bit 4:2:2 format + 8-bit alpha.

    IMG_PIXFMT_PL12Y8       =   33,             //!< Y only 8 bit
    IMG_PIXFMT_PL12Y10      =   34,             //!< Y only 10 bit

    IMG_PIXFMT_PL12YV8      =   35,             //!< Planar 8-bit 4:2:0 (Y data, followed by U data, followed by V data)
    IMG_PIXFMT_PL12IMC2     =   36,             //!< Planar 8-bit 4:2:0 (Y data, followed by line of U, line of V, interleaved)

    IMG_PIXFMT_A4           =   37,             //!< Alpha only 4 bit
    IMG_PIXFMT_A8           =   38,             //!< Alpha only 8 bit
    IMG_PIXFMT_YUV8         =   39,             //!< 8-bit palettised.
    IMG_PIXFMT_CVBS10       =   40,             //!< Composite video (CVBS) data stored by the UCC for YC separation.

	IMG_PIXFMT_ABGR8888     =   41,				//!< 24-bit RGB + 8-bit alpha (ABGR).
	IMG_PIXFMT_BGRA8888     =   42,				//!< 24-bit RGB + 8-bit alpha (BGRA).
	IMG_PIXFMT_ARGB8332     =   43,				//!< 16-bit RGB (R=3, G=3, B=2) + 8-bit alpha.

	IMG_PIXFMT_PL12YV12     =   44,				//!< YV12 Planar 8-bit 4:2:0 (Y data, followed by U data, followed by V data)					
	
	#if (!defined (METAG) && !defined (MTXG))	/* Floats are not allowed in embedded systems */
		IMG_PIXFMT_F16			=	52,				//!< Single channel 16 bit IEEE floating point format
		IMG_PIXFMT_F32			=	53,				//!< Single channel 32 bit IEEE floating point format
	#endif
	
	IMG_PIXFMT_L16			=	54,				//!< Y only 16 bit
	IMG_PIXFMT_L32			=	55,				//!< Y only 32 bit

	IMG_PIXFMT_ARBPLANAR8		= 65536,		//!< 8-bit samples with up to four planes. Lower 16-bits of pixel format encodes subsampling factors. Colour-space is not specified.
	IMG_PIXFMT_ARBPLANAR8_LAST	= IMG_PIXFMT_ARBPLANAR8+0xffff,
	
} IMG_ePixelFormat;

/*!
******************************************************************************
 This maximum number of planes possible for a pixel format
******************************************************************************/
#define IMG_MAX_NUM_PLANES		4


/*!
******************************************************************************
 This type describes a pixel format
******************************************************************************/
typedef struct
{
	IMG_BOOL abPlanes[IMG_MAX_NUM_PLANES];			/*! Booleans indicating which planes are in use. */
	IMG_UINT32 ui32BOPDenom;						/*! Common denominator for bytes per pixel calculation. */
	IMG_UINT32 aui32BOPNumer[IMG_MAX_NUM_PLANES];	/*! Per plane numerators for bytes per pixel calculation. */
	IMG_UINT32 ui32HDenom;							/*! Common denominator for horizontal pixel sub-sampling calculation. */
	IMG_UINT32 ui32VDenom;							/*! Common denominator for vertical pixel sub-sampling calculation. */
	IMG_UINT32 aui32HNumer[IMG_MAX_NUM_PLANES];		/*! Per plane numerators for horizontal pixel sub-sampling calculation. */
	IMG_UINT32 aui32VNumer[IMG_MAX_NUM_PLANES];		/*! Per plane numerators for vertical pixel sub-sampling calculation. */
} IMG_sPixelFormatDesc;


/*!
******************************************************************************

 This type defines the buffer format

******************************************************************************/
typedef enum
{
    /*! Image is a progressive image which can be a single field            */
    IMG_BUFFER_FORMAT_PROGRESSIVE                                       = 0x00,
    /*! Image is two interlaced/interwoven fields (even followed by odd).   */
    IMG_BUFFER_FORMAT_INTERLACED_FIELDS,
    /*! Image is one interlaced field (either odd or even). The height      */
    /*! will be half that of the full, progressive, image which is obtained */
    /*! when both fields are stuck together.                                */
    IMG_BUFFER_FORMAT_SINGLE_FIELD,

    /* Place holder only - this is not a valid buffer format                */
    IMG_BUFFER_FORMAT_NUMBER_OF_TYPES

} IMG_eBufferFormat;


/*!
******************************************************************************

 This type defines the image field type

******************************************************************************/
typedef enum
{
    IMG_IMAGE_FIELD_TYPE_FIELD_0,               //!< Image is a even field.
    IMG_IMAGE_FIELD_TYPE_FIELD_1,               //!< Image is an odd field.

} IMG_eImageFieldType;


/*!
******************************************************************************

 This type defines the field display mode

******************************************************************************/
typedef enum
{
    IMG_IMAGE_FIELD_MODE_FRAME = 0,             //!< Display the whole frame (default).
    IMG_IMAGE_FIELD_MODE_FIELD_0,               //!< Display only the even field.
    IMG_IMAGE_FIELD_MODE_FIELD_1,               //!< Display only the odd field.

} IMG_eImageFieldMode;


/*!
******************************************************************************

 This type defines the picture type

******************************************************************************/
typedef enum
{
    IMG_PICTYPE_FRAME = 0,					    //!< FRAME picture in image buffer
    IMG_PICTYPE_FIELD_TOP,                      //!< TOP FIELD picture in image buffer
    IMG_PICTYPE_FIELD_BOTTOM,                   //!< BOTTOM FIELD picture in image buffer
    IMG_PICTYPE_PAIR

} IMG_ePictureType;


/*!
******************************************************************************

 This type defines the AYUV8888 colour structure.

******************************************************************************/
typedef struct
{
    img_uint8   ui8Alpha;                       //!< Alpha component
    img_uint8   ui8Y;                           //!< Y component
    img_uint8   ui8U;                           //!< U component
    img_uint8   ui8V;                           //!< V component

} IMG_sAYUVColour;

/*!
******************************************************************************

 This type defines the ARGB8888 colour structure.

******************************************************************************/
typedef struct
{
    img_uint8   ui8Alpha;                       //!< Alpha component
    img_uint8   ui8R;                           //!< R component
    img_uint8   ui8G;                           //!< G component
    img_uint8   ui8B;                           //!< B component

} IMG_sARGBColour;


/*!
******************************************************************************

 This type defines the field status

******************************************************************************/
typedef enum
{
 	/*! Current Field Status is Interlaced */
	IMG_FIELD_STATUS_INTERLACED,
 	/*! Current Field Status is ODD Fields only */
	IMG_FIELD_STATUS_ODD_ONLY,
	/*! Current Field Status is EVEN fields only */
	IMG_FIELD_STATUS_EVEN_ONLY,
	/*! Current Field Status cannot be matched to one of the above. */
	IMG_FIELD_STATUS_IRREGULAR

} IMG_eFieldStatus;

/*!
******************************************************************************

 This type defines the field state used in ensigmas state machine

******************************************************************************/
typedef enum
{
 	/*! */
	IMG_FIELD_STATE_RESET,
 	/*! */
	IMG_FIELD_STATE_ODD,
	/*! */
	IMG_FIELD_STATE_EVEN

} IMG_eFieldState;

/*!
******************************************************************************

  This structure contains information about a co-ordinate

******************************************************************************/
typedef struct
{
    img_int32   i32X;
    img_int32   i32Y;

} IMG_sXY, * IMG_psXY;


/*!
******************************************************************************

  This structure contains information about a frame

******************************************************************************/
typedef struct
{
    /*! The horizontal offset of the top left hand corner of the frame image,
        in pixels from the left hand edge of the image. */
    img_int32       i32FrameXPos;
    /*! The vertical offset of the top left hand corner of the frame image,
        in lines from the top edge of the image. */
    img_int32       i32FrameYPos;

    img_uint32      ui32FrameWidth;             //!< The width of the frame image, in pixels.
    img_uint32      ui32FrameHeight;            //!< The height of the frame image, in lines.

} IMG_sFrame;



/*!
******************************************************************************

 This type defines whether the input is active or not

******************************************************************************/
typedef enum
{
    IMG_INPUT_POWER_DOWN,                       //!< Power down the module input
    IMG_INPUT_DISABLED,                         //!< Disable the module input
    IMG_INPUT_SLEEP,                            //!< Disable the module input
    IMG_INPUT_ENABLED,                          //!< Enable the module input

} IMG_eInputActiveSettings;



/*!
******************************************************************************

 This type defines whether the input is active or not

******************************************************************************/
typedef enum
{
    IMG_OUTPUT_DISABLED,                        //!< Disable the module input
    IMG_OUTPUT_ENABLED                          //!< Enable the module input

} IMG_eOutputActiveSettings;



/*!
******************************************************************************

 This type defines the AAYUV8888 CLUT.

******************************************************************************/
typedef struct
{
    img_uint32  aui32ClutEntry[256];

} IMG_sAYUV8888Clut;

/*!
******************************************************************************

 This type defines the AVUY8888 CLUT.

******************************************************************************/
typedef struct
{
    img_uint32  aui32ClutEntry[256];

} IMG_sAVUY8888Clut;

/*!
******************************************************************************

 This type defines the ARGB8888 CLUT.

******************************************************************************/
typedef struct
{
    img_uint32  aui32ClutEntry[256];

} IMG_sARGB8888Clut;

/*!
******************************************************************************

 This type defines the ABGR8888 CLUT.

******************************************************************************/
typedef struct
{
    img_uint32  aui32ClutEntry[256];

} IMG_sABGR8888Clut;

/*!
******************************************************************************

 This type allows a 32 bit IEEE floating point value to be viewed as raw bits

******************************************************************************/
typedef union
{
	img_float	fx;
	img_uint32	ui32x;

} IMG_uFLUINT32;

/*!
******************************************************************************

 This type defines the union of possibe CLUT entries

******************************************************************************/
typedef union
{
    IMG_sAYUV8888Clut   sAYUV8888Clut;      //!< AYUV8888 CLUT entries.
    IMG_sAVUY8888Clut   sAVUY8888Clut;      //!< AVUY8888 CLUT entries.
    IMG_sARGB8888Clut   sARGB8888Clut;      //!< ARGB8888 CLUT entries.
    IMG_sABGR8888Clut   sABGR8888Clut;      //!< ABGR8888 CLUT entries.

} IMG_uClut;

/*!
******************************************************************************

 This type defines the CLUT type

******************************************************************************/
typedef enum
{
    IMG_CLUTTYPE_AYUV8888,                  //!< Clut AYUV8888
    IMG_CLUTTYPE_AVUY8888,                  //!< Clut AVUY8888
    IMG_CLUTTYPE_ARGB8888,                  //!< Clut ARGB8888
    IMG_CLUTTYPE_ABGR8888                   //!< Clut ABGR8888

} IMG_eClutType;

/*!
******************************************************************************

 This structure contains CLUT information

******************************************************************************/
typedef struct IMG_sClut_TAG
{
    struct IMG_sClut_TAG *  pNextCLUT;      //!< Pointer to next CLUT, IMG_NULL if last associated CLUT.
    IMG_eClutType           eClutType;      //!< CLUT type.
    img_uint32              ui32SizeOfClut; //!< Size of CLUT.
    IMG_uClut               uClut;          //!<An array of CLUT entries, type defined by eCLUTType.

} IMG_sClut;

/*!
******************************************************************************

 This type defines the video sample rate

******************************************************************************/
typedef enum
{
	IMG_SAMPLE_RATE_14_3,					//!< Video sample rate 14.3 Mhz
	IMG_SAMPLE_RATE_28_6,					//!< Video sample rate 28.6 Mhz
	IMG_SAMPLE_RATE_50_6,					//!< Video sample rate 50.6 Mhz
	IMG_SAMPLE_RATE_13_5,					//!< Video sample rate 13.5 Mhz
	IMG_SAMPLE_RATE_27_0,					//!< Video sample rate 27.0 Mhz
	IMG_SAMPLE_RATE_74_25,					//!< Video sample rate 74.25 Mhz
	IMG_NUM_SAMPLE_RATES,					//!< Number of sample rates

} IMG_eSampleRate;

/*!
******************************************************************************

 This type defines the image orientation.  Rotation is in the clockwise direction

******************************************************************************/
typedef enum
{
	IMG_ROTATE_NEVER,
	IMG_ROTATE_0,
	IMG_ROTATE_90,
	IMG_ROTATE_180,
	IMG_ROTATE_270,

} IMG_eOrientation;


/*!
******************************************************************************

 This type defines the subsampling of an image

******************************************************************************/
typedef enum
{
	//Keep values as it is to allow for easy masking 
	IMG_SUBSAMPLE_NONE = 0,    //!< No subsampling
    IMG_SUBSAMPLE_X    = 1,    //!< Image contains half the amount of data in the x-axis
    IMG_SUBSAMPLE_Y    = 2,    //!< Image contains half the amount of data in the y-axis
    IMG_SUBSAMPLE_XY   = 3,    //!< Image contains half the amount of data in both x and y

} IMG_eSubsample;


/*!
******************************************************************************

 This type specifies the type of row stride to use.

******************************************************************************/
typedef enum
{
	IMG_ROWSTRIDE_SMALLEST,     //!< Smallest possible stride
	IMG_ROWSTRIDE_VIDEO,        //!< Smallest possible video stride
	IMG_ROWSTRIDE_512,          //!< Smallest possible stride that is a multiple of 512 pixels
	IMG_ROWSTRIDE_MAX           //!< The Maximum stride

} IMG_eRowStrideType;


/*!
******************************************************************************

 This structure contains the image buffer information

 The pixels within the image buffers are grouped into "blocks-of-pixels" (BOPs)
 dependent upon the image type.

 BOPs are byte aligned structures that contains the data for a integral number
 of pixels and organised in a form suitable for the hardware.

******************************************************************************/
typedef struct IMG_sImageBufCB_tag
{
    LST_LINK;											//!< List link (allows the buffer control block to be part of a MeOS list).
    img_uint32          ui32BufferPool;     			//!< Buffer pool.
    img_uint32          ui32PoolID;         			//!< Buffer ID.
    IMG_ePixelFormat    ePixelFormat;      				//!< Pixel format of data.
    IMG_eBufferFormat   eBufferFormat;      			//!< Buffer format
    img_bool            bPlanar;            			//!< IMG_TRUE if planar, otherwise IMG_FALSE.
    img_bool            bAlpha;             			//!< IMG_TRUE if alpha plane, otherwise IMG_FALSE.

    img_uint32          ui32ImageWidth;     			//!< Width of the image (in pixels).
    img_uint32          ui32ImageHeight;    			//!< Height of the image (in lines).
    img_uint32          ui32ImageStride;				//!< The stride of the image (in bytes).
	img_uint32          ui32RotatedStride;				//!< Stride of the image when rotated to 90 or 270 (in bytes).

    img_uint32          ui32OriginalDisplayWidth;       //!< Original display width of the image (before rotation or scaling)
    img_uint32          ui32OriginalDisplayHeight;      //!< Original display height of image (before rotation or scaling)

    img_bool            bScaled;                        //!< Buffer contains a scaled image
    IMG_eOrientation	eOrientation;					//!< Orientation of the image.
    IMG_eSubsample      eSubsample;                     //!< Subsampling of the image
    IMG_sXY             sOffset;                        //!< Offset of top-left corner of image

    /*! IMG_TRUE if the stride of UV plane is half that of the Y
     *  plane, otherwise IMG_FALSE.  Only valid if bPlanar is IMG_TRUE.
     */
    img_bool            bUVStrideHalved;
    /*! IMG_TRUE if the height of UV plane is half that of the Y
     *  plane, otherwise IMG_FALSE.  Only valid if bPlanar is IMG_TRUE.
     */
    img_bool            bUVHeightHalved;

    IMG_eImageFieldType eImageFieldType;    			//!< Image field type.
    img_uint32          ui32BufSize;        			//!< Size of the buffer
    img_uint32          ui32UVBufSize;      			//!< Size of UV buffer - if Planar
    img_uint32          ui32VBufSize;      	 			//!< Size of V buffer - if Planar and separate U/V
    img_uint32          ui32AlphaBufSize;   			//!< Size of Alpha buffer - if present
    img_uint32          ui32BufAlignment;   			//!< Alignment of the buffer (1, 2, 4, 8, 16, 32...)
    
    /*!< Pointer to an associated set of CLUT structures.  Only valid for IMG_PIXFMT_RGB8 format	*/
    /*!< images, but can also be IMG_NULL if not palette has been associated with this image.		*/
    IMG_sClut *         psClut;
    
    img_uint8 *         pvBufBase;          			//!< The buffer base address.  NOTE: This address is non-aligned and should NOT be used outside of the Buffer Management System
    img_uint8 *         pvUVBufBase;            		
    img_uint8 *         pvVBufBase;             		
    img_uint8 *         pvAlphaBufBase;         		
                                                		
    img_uint32          ui32YPixelInBOPs;   			//!< Number of pixels in a block-of-pixels.
    img_uint32          ui32YBytesInBOPs;   			//!< Number of bytes in a block-of-pixels.
                                                		
    img_uint8 *         pvYBufAddr;         			//!< Y or non-planar buffer address
    img_uint32          ui32UVBytesInBOPs;  			//!< Number of bytes in a block-of-pixels in the UV plane.  Only valid if bPlanar is IMG_TRUE.
    img_uint8 *         pvUVBufAddr;        			//!< UV buffer address.  Only valid if bPlanar is IMG_TRUE.
    img_uint8 *         pvVBufAddr;         			//!< V buffer address.  Only valid if bPlanar is IMG_TRUE.
                                                		
    img_uint32          ui32AlphaBytesInBOPs;   		
    img_uint8 *         pvAlphaBufAddr;     			//!< Alpha buffer address.  Only valid in PL+A formats.
                                                		
    img_void *          pApplicationData;   			//!< Pointer to application specific data.  IMG_NULL if not data has been associated with this buffer.
                                                		
    img_uint32          ui32MemSpaceId;     			//!< Device dependent Memory Space ID
    img_bool			bShadowMemoryIsPaged; 			//!< Used to indicate whether the MMU has been used to map device/shadow memory
    img_handle			hShadowMem;         			//!< Handle to shadow memory associated with this buffer
    img_handle			hShadowMemUV;
    img_handle			hShadowMemV;
    img_handle			hShadowMemAlpha;

    img_bool            bTiled;             			//!< IMG_TRUE if buffer tiled
    img_uint32          ui32htile;          			//!< N - refer to MC_DDR spec for details
    img_uint32          ui32HwStride;       			//!< h/w stride - used for writes to hardware regardless of whether tiling is on or not
    
    /*!< If non zero then the start of every line (i.e.: the buff start address AND the line stride 	*/
    /*!< will be rounded to the nearest multiple of this number.										*/
    /*!< Note : 	Unless the 'BFM_FLAGS_FORCE_LINE_START_ALIGNMENT' is set in the flags word, this	*/
    /*!< 			field will be ignored, and line alignment will not be performed.					*/
    img_uint32		ui32ForceLineStartAlignment;
    
    struct IMG_sImageBufCB_tag * psNextImageBufCB; 		//!< Pointer to the next buffer in a chain
                                                    	
    struct IMG_sImageBufCB_tag * psPrimImageBufCB; 		//!< Pointer to the primary buffer control block (if this is a secondary), IMG_NULL if this is a primary.
    img_uint32          ui32NoSec;          			//!< The number of associated secondary buffer control blocks (only valid if this is a primary).
    img_bool            bPrimFreed;         			//!< Flag to indicate that the primary has been freed.

    IMG_eImageFieldMode eImageFieldMode;				//!< Image field display mode.
                                        					
    img_uint32          ui32FlagsWord;  				//!< Configuration flags.  See #BFM_eFlagsWord.
    img_bool            bAllocated;     				//!< IMG_TRUE until this CB has been made free. Used to trap free twice.
	img_uint32			Time;							//!< Timestamp. This is typically set in the VPIN API.
	
	/*!< The rms noise level estimated during the vertical blanking region, in IRE units.	*/
	/*!< 16 fractional bits. 																*/
	img_int32			i32NoiseLevelEstimate;

    img_int32  i32VbiFrameXPos;							//!< VBI First available pixel on each line of the frame																																																																										
	img_int32  i32VbiFrameYPos;							//!< VBI Line number of first frame line in the buffer in terms of standard line numbering system																																																																										
	img_uint32 ui32VbiActiveWidth;						//!< VBI Width in pixels of active area of the image buffer																																																																										
	img_uint32 ui32VbiActiveHeight;						//!< VBI Height in lines of the active area of the image buffer																																																																										
	img_int32  i32VbiChromaRef;							//!< Chroma phase measurement taken on WideClearVision VBI line																																																																										
	img_int32  i32VbiChromaPhaseInc;					//!< Chroma phase increment taken on WideClearVision VBI line																																																																										
                                        																																																																														
	img_int32  i32VbiBlankLevel;						//!< VBI Blanking level																																																																										
	img_int32  i32VbiWhiteLevel;        				//!< VBI Peak white level																																																																										
                                        																																																																														
	img_int32	ui32VbiSourceFrameWidth;				//!< VBI Source frame width																																																																										
	img_int32	ui32VbiDestFrameWidth;					//!< VBI Destination frame width																																																																										
                                        																																																																														
	IMG_eSampleRate		eSampleRate;					//!< Video sample rate																																																																										
	IMG_ePictureType	ePictureType;					//!< Indicates whether FRAME, FIELD_TOP or FIELD_BOTTOM																																																																										
	img_uint32	ui32DisplayFrameNum;					//!< The frame number in display order																																																																										
	img_uint8	ui8DisplayCount;						//!< The number of times to present this frame																																																																										
	img_uint8	ui8Encoding;							//!< The encoding type (I, P or B) of the picture																																																																										
	
	struct IMG_sImageBufCB_tag * psNextBufferToBeUsed; //!< Pointer to the next buffer to be used (in temporal order)

} IMG_sImageBufCB;


/*!
******************************************************************************

 This structure is used defines the layout and content of the IMG Image File
 Header.

******************************************************************************/
typedef struct
{
    IMG_CHAR            pszId[4];       	//!< Signature "IMG"
    IMG_UINT32          ui32HdrVersion; 	//!< Should be 1 - allows for a new version of the header to be defined
    IMG_UINT32          bSingleImage;  	 	//!< IMG_TRUE if this is a single image, otherwise IMG_FALSE (for a sequence)
	IMG_UINT32          ePixelFormat;   	//!< Pixel format see #IMG_ePixelFormat
	IMG_UINT32			eBufferFormat;		//!< Buffer format. See IMG_eBufferFormat
	IMG_UINT32		    ui32Width;      	//!< The width of the image (in pixels)
	IMG_UINT32		    ui32Height;     	//!< The height of the image (in lines)
	IMG_UINT32		    ui32Stride;     	//!< The stride (in bytes)

} IMG_sImageFileHeader;

/*!
******************************************************************************

 @Function              IMG_pfnEventCallback

 @Description

 This is the prototype for event callback functions.

 NOTE: The values and meaning of the parameters eEvent, ui32Param and pvParam
 are dependent upon the context of the callback.  See the API documentation for
 details.

 @Input    pCallbackParameter   : A pointer to the application specific data.

 @Input    eEvent       : Indicates the event which has occued.

 @Input    ui32Param    : Unsigned integer parameter.

 @Input    pvParam      : Pointer parameter.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
typedef img_result ( * IMG_pfnEventCallback) (
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_void *                  pvParam
);


/*!
******************************************************************************

 @Function              IMG_pfnRemoteEventCallback

 @Description

 This is the prototype for the remote event callback functions.

 NOTE: The values and meaning of the parameters eEvent, ui32Param and pvParam
 are dependent upon the context of the callback.  See the API documentation for
 details.

 @Input     ui32APIId   : The id of the API (on the remote thread).  This
                          should be uniquie on the thread as it is used to
                          serialise callbacks from the API.

 @Input     pfnEventCallback    : A pointer to the callback function to be called.

 @Input     eEvent      : The event id to be passed to the callback.

 @Input     pCallbackParameter  : A callback parameter defined when the callback
                          function was "added".

 @Input     ui32Param   : A uint32 value to be passed to the callback.  The
                          value/meaning is dependent upon the event being signalled.

 @Input     ui32Sizeui32ParamData : If ui32Param is being used to pass a pointer
                          to some data then this contains the size (in bytes) of the
                          data.  This allows the code calling the callback to flush
                          the cashe to ensure that the data is "current" and
                          not stale.  0 if ui32Param does not point to data being
                          passed.

 @Input     pvParam     : A pointer value to be passed to the callback.  The
                          value/meaning is dependent upon the event being signalled.

 @Input     ui32SizepvParamData : If pvParam is being used to pass a pointer
                          to some data then this contains the size (in bytes) of the
                          data.  This allows the code calling the callback to flush
                          the cashe to ensure that the data is "current" and
                          not stale.  0 if pvParam does not point to data being
                          passed.

 @Return   img_result   : This function returns either IMG_SUCCESS or an
                          error code.

******************************************************************************/
typedef img_result ( * IMG_pfnRemoteEventCallback) (
    img_uint32                  ui32APIId,
    IMG_pfnEventCallback        pfnEventCallback,
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_uint32                  ui32Sizeui32ParamData,
    img_void *                  pvParam,
    img_uint32                  ui32SizepvParamData
);


/*!
******************************************************************************

 This type defines the callback mode

******************************************************************************/
typedef enum
{
    IMG_CBMODE_CLEAR_CBS_ON_RESET,          //!< Callback functions cleared on reset.
    IMG_CBMODE_CBS_PERSISTENT,              //!< Callback functions are persistent (not cleared on reset).

} IMG_eCallbackMode;


/*!
******************************************************************************

 This type defines the required buffer mode

 The Analog Component Input supports three modes of buffer management:
 <b>single</b>, <b>double</b> and <b>streaming</b>.

 In <b>single</b> buffer mode, the foreground buffer is set before the input
 is enabled.  All input is captured into this buffer.  It is not possible to
 change the buffer whilst the input is enabled.

 In <b>double</b> buffer mode, the foreground and background buffers are set
 before the input is enabled.  Input is captured into the foreground buffer
 and the buffers "flipped" when the buffer is full (the foreground becomes
 the background and visa-versa).  The buffer flipping continues until the
 input is disabled.

 <b>Streaming</b> mode is a variant of double buffering mode.  In <b>streaming</b>
 mode, the background buffer can be replaced after the buffers have been
 flipped.  The buffer is replaced using ACI_SetDestinationBuffer and would
 normally be called from within the event callback indicating that the buffer
 has an image.

******************************************************************************/
typedef enum
{
    IMG_BUFMODE_SINGLE,                     //!< Selects single buffer mode
    IMG_BUFMODE_STREAM,                     //!< Selects streaming mode
    IMG_BUFMODE_CVBS,                       //!< Selects CVBS streaming mode
    IMG_BUFMODE_NOBUFFER,                   //!< Used with inputs/outputs that have a direct connection with another block.
    IMG_BUFMODE_DOUBLE,                     //!< This mode has been deprecated and should no longer be used
    IMG_BUFMODE_CVBS_LOW_LATENCY            //!< Selects CVBS streaming mode with lowest possible latency

} IMG_eBufferMode;



/*!
******************************************************************************

 This type defines the buffer type

******************************************************************************/
typedef enum
{
    IMG_BUFTYPE_FOREGROUND,                 //!< Indicates a foreground buffer
    IMG_BUFTYPE_BACKGROUND,                 //!< Indicates a background buffer
    IMG_BUFTYPE_VIDEO,                      //!< Indicates a video buffer
    IMG_BUFTYPE_VBI                         //!< Indicates a VBI buffer

} IMG_eBufferType;

/*!
******************************************************************************

 This structure contains the scaler control parameters.

 NOTE: The controls within this structure may not be implemented in all
 scalers and the precision may vary (e.g the number of coefficients may be
 less that setout in this structure, the scaler may not have separate
 coefficients for luma and chroma).  Details of the scaler controls
 can be found in the appropriate TRM.

******************************************************************************/
typedef struct
{
    img_bool        bScaleBp;               //!< IMG_TRUE to bypas the scaler, otherwise IMG_FALSE.
    img_uint32      ui32NoTaps;             //!< The number of taps (must be valid for the scaler being set).
    img_uint32      ui32FilterOrder;        //!< The filter order
    img_uint32      ui32Pitch;              //!< The pitch (1/Scale Factor).  Fixed point 16.16 - will be truncated to match the scaler being set.
    img_uint32      uint32InitialSize;      //!< The initial size, in pixels
    img_uint32      uint32ScaledSize;       //!< The scaled size, in pixels
    img_uint32      ui32NoPixelsToClip;     //!< The number of pixels to clip after scaling
    img_uint32      ui32Initial;            //!< Initial position for field 0.  Fixed point 16.16 - will be truncated to match the scaler being set.
    img_uint32      ui32Initial1;           //!< Initial position for field 1.  Fixed point 16.16 - will be truncated to match the scaler being set.
    img_uint32      ui32Increment;          //!< Increment Value    .  Fixed point 16.16 - will be truncated to match the scaler being set.
    img_uint32      ui32StartRamp;          //!< Start ramp position
    img_uint32      ui32EndRamp;            //!< End ramp position
    img_bool        bForceLineStore;        //!< Force scalar Line store on Underrun
    img_bool        bFetchInterlaced;       //!< Fetch Interlaced Data
    /*! Sscaling under-run control:
     *      IMG_FALSE   Acknowledge under-run, and treat in same manner as unscaled data.
     *      IMG_TRUE    Ignore under-run. Gives better results in many situations due toscaler FIFO.
     */
    img_bool        bUnderRunControl;
    /*! Scaler chroma filter control:
     *      IMG_FALSE   Advance chroma filter at half rate of luma filter (legacy mode for 420 only when luma and chroma filter coefficients are identical).
     *      IMG_TRUE    Advance chroma filter at full rate of luma filter (422, 444, or 420 with modified chroma filter coefficients).
     */
    img_bool        bChromaControl;

    /* Coeff Register Values */
    img_uint32      aui32CoeffRegs [ ( (SCC_MAXTAP * SCC_MAXINTPT) / 4 ) + 1 ];     /* !< Fixed point 16.16 - will be truncated to match the scaler being set. */

} IMG_sScalerConfigData;

/*!
******************************************************************************

 This structure is used to hold a PTS (Presentation Time Stamp).

******************************************************************************/
typedef struct
{
    img_uint32      ui32PTSHi;              //!< MS 32-bits of the PTS.
    img_uint32      ui32PTSLo;              //!< LS 32-bits of the PTS.

} IMG_sPTS;

/*!
******************************************************************************

 This structure is used to hold an "extended" PTS (Presentation Time Stamp).

 Note that this is essentially a redefinition of the IMG_sPTS structure above.
 It is important to ensure that order of the byte fields which replace
 ui32PTSHi are such that ui8PTSHi is the first field.

******************************************************************************/
typedef struct
{
    img_uint8       ui8PTSHi;               //!< MS 8-bits of the PTS.
	img_uint8		ui8FrameRateCode;		//!< Frame rate code as per MPEG spec 13818-2 (with possible extensions)
	img_uint8		ui8Flags;				//!< Flags bits
	img_uint8		ui8Spare;				//!< Currently unused.
    img_uint32      ui32PTSLo;              //!< LS 32-bits of the PTS.

} IMG_sPTSExt;

#define	PTSEXTFLAG_TOP_FIELD	0x01		//!< Used to indicate that the PTS applies to the top field of a frame

/*!
******************************************************************************

 This structure is used to store an elapsed time.

******************************************************************************/
typedef struct
{
	img_uint32	ui32Days;
	img_uint32	ui32Hours;
	img_uint32	ui32Minutes;
	img_uint32	ui32Seconds;

} IMG_sElapsedTime;

/*!
******************************************************************************

This type defines the API Ids for remote callbacks.

******************************************************************************/
typedef enum
{
    API_ID_TEST = 0x01,             //!< TEST is a dummy API used by tests to log events
    API_ID_BFM,                     //!< BFM API Id
    API_ID_SMDEC,                   //!< SMDEC API Id
	API_ID_SMHDP,                   //!< SMHDP API Id
    API_ID_SMADEC,                  //!< SMADEC API Id
    API_ID_SMVDEC,                  //!< SMVDEC API Id
    API_ID_SMVOUT,                  //!< SMVOUT API Id
    API_ID_SMAOUT,                  //!< SMAOUT API Id
    API_ID_TSD,                     //!< TSD API Id
    API_ID_RCBA,                    //!< RCBA API Id
    API_ID_RCBB,                    //!< RCBB API Id
    API_ID_RCBC,                    //!< RCBB API Id
    API_ID_RMEM,                    //!< RMEM API Id
    API_ID_ITC,                     //!< ITC API Id
    API_ID_ISR,                     //!< ISR API Id
    API_ID_ENDDRV,                  //!< ENDDRV API Id
    API_ID_MEOSAL,                  //!< MEOSAL API Id
    API_ID_AUD,                     //!< AUD API Id
    API_ID_DIGO,                    //!< DIGO API Id
    API_ID_VPIN,                    //!< VPIN API Id
    API_ID_PDP,						//!< PDP API Id
#ifdef __ITC_TEST__
    API_ID_thread3,                  //!< thread3 Id
    API_ID_thread2,                  //!< thread2 Id
#endif
	API_ID_SCB,						//!< SCB API Id
	API_ID_BLIT,					//!< BLIT API Id
	API_ID_DEVIO,					//!< DEVIO Id
    API_ID_MPM,                     //!< MPM API Id
    API_ID_SPIS,					//!< SPI slave API Id
    API_ID_DMAC,					//!< DMAC API Id
    API_ID_DMAC_PERIP,
    API_ID_DMAN,					//!< DMAN API
    API_ID_DMANKM,					//!< DMANKM API
    API_ID_MSVDXDEVICE,				//!< MSVDX Device
    API_ID_TOPAZDEVICE,				//!< TOPAZ Device
    API_ID_PALLOC,					//!< PALLOC API
    API_ID_WRAPU,					//!< WRAPU API
    API_ID_SYSBRG,					//!< SYSBRG API
    API_ID_UMP,						//!< UMP API
    API_ID_UMISR,					//!< UMISR API

	API_ID_IPC_UM,					//!< IPC_UM API
	API_ID_PDUMP_CMDS,				//!< PDUMP_CMDS API
	API_ID_DEVICEIO,				//!< DEVICEIO API
	API_ID_DMACDD,					//!< DMAC DEVICE DRIVER API
	API_ID_LOGMAN,                  //!< LOGMAN API
	
	API_ID_GDMA,					//!< GDMA API

	// API_ID_FAKEDEVICE,              //!< FAKE Device API Id

    API_ID_MAX                      //!< Max. API Id.

} SYS_eApiId;

#if defined (__cplusplus)
}
#endif

#endif /* __IMG_COMMON_H__  */
