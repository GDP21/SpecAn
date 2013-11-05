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
 Revision 1.1  2009/02/16 15:43:08  michael.melbourne
 Initial version

 Revision 1.123  2007/05/16 15:50:24  bal
 Added new pixel formats: ABGR8888, BGRA8888, ARGB8332. Removed
 include of krn.h as this does not appear to be used by the header.

 Revision 1.122  2007/03/30 15:35:40  thw
 Added IMG_sARGBColour.

 Revision 1.121  2007/03/29 11:51:26  thw
 Added pixel colour format 'IMG_PIXFMT_YUYV8888'.

 Revision 1.120  2007/03/28 13:21:20  thw
 Use of 'BFM_FLAGS_FORCE_LINE_START_ALIGNMENT' flag is no longer restricted to Meson_LC builds.

 Revision 1.119  2007/03/27 16:04:31  thw
 Added support for formats IMG_PIXFMT_I4A4 and IMG_PIXFMT_I8A8.

 Revision 1.118  2007/03/27 11:55:40  thw
 Added support for formats IMG_PIXFMT_VYUY8888 and IMG_PIXFMT_YVYU8888.

 Revision 1.117  2007/03/27 09:56:58  thw
 Added support for 411PL12YVU8.

 Revision 1.116  2007/03/26 11:14:33  thw
 Added support for 422PL12YVU8.

 Revision 1.115  2007/03/23 11:00:58  thw
 Added support for 420PL12YVU8.

 Revision 1.114  2007/03/13 12:21:02  thw
 Removed fields added in last version (except for eBufferFormat, which stays).
 Removed bPlanar - this can be inferred from pixel colour format.

 Revision 1.113  2007/03/12 17:17:12  thw
 Expanded 'IMG_sImageFileHeader'.

 Revision 1.112  2007/03/05 14:18:22  rml
 Add support for .img files and directory conversion

 Revision 1.111  2007/02/14 15:31:29  thw
 LST code now entirely separate from KRN code - allows both KRN stubs and LST code in 'meosal/utils' to be used together.

 Revision 1.110  2006/06/30 16:17:39  thw
 Added 'ui32ForceLineStartAlignment' to buffer control block.

 Revision 1.109  2006/05/30 09:52:49  grc
 Changed img_void ** to img_handle for shadow memory handles.

 Revision 1.108  2006/04/26 13:54:55  thw
 Added pointer to next buffer to be used.

 Revision 1.107  2006/03/20 09:57:38  bal
 Added IMG_SAMPLE_RATE_27_0 and IMG_SAMPLE_RATE_74_25 to
 IMG_eSampleRate. Added ui32VbiSourceFrameWidth and
 ui32VbiDestFrameWidth to IMG_sImageBufCB. These changes allow extending
 VBI support to more input formats.

 Revision 1.106  2006/03/08 15:46:08  emb
 Undo previous checkin (now in branch).

 Revision 1.105  2006/03/08 15:43:06  emb
 Added additional pixel formats for DBX2.

 Revision 1.104  2006/02/10 18:15:52  thw
 Added API IDs

 Revision 1.103  2006/01/26 12:39:34  jmg
 Improved comment about Time.

 Revision 1.102  2005/12/23 10:58:53  grc
 ui32DisplayCount => ui8DisplayCount.

 Revision 1.101  2005/12/08 12:22:50  gjd
 Added IMG_BUFMODE_CVBS_LOW_LATENCY

 Revision 1.100  2005/11/16 16:39:24  grc
 Added ui8Encoding member.

 Revision 1.99  2005/10/04 10:15:16  grc
 Added IMG_sPTSExt structure.

 Revision 1.98  2005/09/22 16:43:08  grc
 Added ui32DisplayCount to support the smhdp slow motion display modes.

 Revision 1.97  2005/09/08 14:53:58  emb
 Removed CSC specific definitions (moved to csc.h).

 Revision 1.96  2005/09/01 14:49:09  thw
 Added SCB API ID

 Revision 1.95  2005/09/01 11:26:51  grc
 Added ui32DisplayFrameNum for hard disc playback.

 Revision 1.94  2005/08/23 15:14:15  thw
 Added elapsed time structure.

 Revision 1.93  2005/08/04 07:44:15  thw
 Added PDP API ID.

 Revision 1.92  2005/07/14 14:57:02  grc
 Added SMVDEC to list of loggable entities.

 Revision 1.91  2005/07/08 14:38:34  emb
 Updated documentation on 'IMG_sColourSpaceData', reflecting change to max contrast value (PRN 679).

 Revision 1.90  2005/07/06 16:26:14  emb
 Added 'IMG_ePictureType' and 'ePictureType' to 'IMG_sImageBufCB' (MSH).

 Revision 1.89  2005/06/21 14:19:37  emb
 Changed IMG_sColourSpaceOffsets members from unsigned int to (signed) int.

 Revision 1.88  2005/06/09 12:31:49  jah
 Added SMHDP

 Revision 1.87  2005/05/24 08:56:21  bal
 added  i32VbiBlankLevel and i32VbiWhiteLevel to IMG_sImageBufCB

 Revision 1.86  2005/05/18 10:13:01  emb
 Added IMG_CSC_BT_709_YUV_TO_BT_601_YUV and IMG_CSC_BT_601_YUV_TO_BT_709_YUV to IMG_eCSCConfig.

 Revision 1.85  2005/05/12 10:15:06  bal
 Added IMG_NUM_SAMPLE_RATES as a type to IMG_eSampleRate

 Revision 1.84  2005/05/11 16:03:42  rml
 Add VPIN API for logging

 Revision 1.83  2005/05/09 09:21:14  emb
 Changed IMG_sVideoMatrixCoeffs members from unsigned int to signed int (PRN 572).

 Revision 1.82  2005/05/05 12:58:14  rml
 Add ND style event logging for DIGO

 Revision 1.81  2005/05/04 11:55:41  rml
 Add IMG_eSampleRate and eSampleRate and i32VbiChromaPhaseInc  to image CB

 Revision 1.80  2005/05/04 11:53:18  rml
 Add IMG_eSampleRate and eSampleRate and i32VbiChromaPhaseInc  to image CB

 Revision 1.79  2005/05/03 15:35:03  emb
 Changed IMG_sCSCCoefficientGains.ui32CoefficientRowXGain from unsigned int to signed int (PRN 572).

 Revision 1.78  2005/04/29 14:27:01  emb
 Changed IMG_sColourSpaceData.i32Hue docs to state that it is now in 8.23 (signed) fixed point,
 and allowed range is -90~+90 degrees in 0.5 degree steps.

 Revision 1.77  2005/04/28 08:18:55  ajp
 Added Field State and Status

 Revision 1.76  2005/04/27 11:38:32  rml
 Remove i32FrameXPos

 Revision 1.75  2005/04/26 10:29:32  rml
 no message

 Revision 1.74  2005/04/26 10:27:48  rml
 Add i32VbiFrameXPos

 Revision 1.73  2005/04/25 13:46:56  rml
 Add VBI parameters to image control block

 Revision 1.72  2005/04/20 09:54:03  emb
 Updated hue range documentation.

 Revision 1.71  2005/04/14 11:16:06  emb
 Moved new conversions IMG_CSC_RGB_0_255_TO_BT_601_YUV and IMG_CSC_RGB_0_255_TO_BT_709_YUV to end of enum.

 Revision 1.70  2005/04/06 09:42:41  ajp
 Added i32NoiseLevelEstimate to buffer control block to carry detected noise for that image.

 Revision 1.69  2005/03/17 10:04:17  emb
 Added new CSC conversions IMG_CSC_RGB_0_255_TO_BT_601_YUV and IMG_CSC_RGB_0_255_TO_BT_709_YUV.

 Revision 1.68  2005/03/10 16:33:56  mxb
 Added timestamp field

 Revision 1.67  2005/02/28 20:40:29  mmd
 Add API_ID_AUD to SYS_eApiId

 Revision 1.66  2005/01/20 15:00:18  msh
 Added support for audio thread 1 to thread 2 inter-thread comms

 Revision 1.65  2005/01/07 13:35:38  ajp
 Various changes for using ADC clocks, noBuffer buffer mode and Phase Adjustment.

 Revision 1.64  2005/01/05 13:57:59  rml
 Fix build problems with ITC_TEST

 Revision 1.63  2005/01/04 14:55:24  rml
 Add new API IDs

 Revision 1.62  2004/12/23 16:40:09  rml
 New API_IDs added

 Revision 1.61  2004/12/21 16:49:22  rml
 New API Ids added

 Revision 1.60  2004/12/17 09:51:45  rml
 Add RMEM tiled memory inter-thread notification

 Revision 1.59  2004/12/16 14:37:14  ajp
 Corrected CSC documentation on API ranges.

 Revision 1.58  2004/12/16 12:04:09  rml
 Add additional ITC API IDs

 Revision 1.57  2004/12/03 16:28:54  jmg
 Added BFM free twice checking.

 Revision 1.56  2004/11/26 12:26:08  thw
 Fixed comment for member 'ui32HwStride' of structure 'IMG_sImageBufCB'

 Revision 1.55  2004/11/24 16:51:03  grc
 Initial support added for PL12IMC2 pixel format.

 Revision 1.54  2004/11/11 15:20:35  ajp
 Put offset values back to unsigned!

 Revision 1.53  2004/11/11 14:54:00  ajp
 Changed offsets to signed values instead of unsigned.

 Revision 1.52  2004/11/09 16:59:30  thw
 Deprecated 'IMG_BUFMODE_DOUBLE' in 'IMG_eBufferMode' - this mode should no longer be used.

 Revision 1.51  2004/11/01 09:35:44  rml
 Fix compilation errors

 Revision 1.50  2004/10/28 14:51:03  grc
 Moved SYS_eApiId enum from system.h to img_common.h.

 Revision 1.49  2004/10/28 13:22:57  rml
 Add remote callback functionality

 Revision 1.48  2004/10/14 16:48:08  grc
 Reinstated IMG_EVENT_DEFAULT, which can coexist with IMG_EVENT_ALL.

 Revision 1.47  2004/10/14 11:02:16  grc
 Replaced IMG_EVENT_DEFAULT with IMG_EVENT_ALL.

 Revision 1.46  2004/09/22 11:44:09  mxb
 Added flags word to ImageBufCB

 Revision 1.45  2004/09/16 15:02:48  ajp
 Added YCC Comments to CSC modes.

 Revision 1.44  2004/09/16 15:00:26  ajp
 Checked in YPP->YUV (Ycc) 601 and 709 conversions. 709 just needs coeffs to be enabled (will assert at present). *NOTE* Where YUV is specified, YCC is what we actually mean!

 Revision 1.43  2004/09/09 16:15:58  ajp
 no message

 Revision 1.42  2004/09/09 15:57:42  ajp
 Updated CSC

 Revision 1.41  2004/09/01 16:21:26  jmg
 Added field display mode.

 Revision 1.40  2004/08/19 09:59:51  rml
 Chaneg CSC offset structure

 Revision 1.39  2004/08/19 09:11:45  rml
 Add paramater to ACI CSC API

 Revision 1.38  2004/08/13 16:06:00  rml
 Add support for tiled addressing

 Revision 1.37  2004/08/12 10:51:20  mmd
 Added IMG_sPTS

 Revision 1.36  2004/07/27 15:30:58  rml
 Add IMG_EVENT_DEFAULT

 Revision 1.35  2004/07/26 08:13:11  rml
 no message

 Revision 1.34  2004/07/19 14:49:22  rml
 Move NO_IRQ

 Revision 1.33  2004/07/19 12:34:01  rml
 Add YUV888 support

 Revision 1.32  2004/07/08 11:32:18  mxb
 Added AlphaBufSize to IMG_sImageBufCB

 Revision 1.31  2004/07/08 11:11:29  mxb
 Removed 10bit planar + Alpha
 Added bAlpha to IMG_sImageBufCB

 Revision 1.30  2004/07/07 18:57:06  mxb
 Added PL + Alpha pixel formats
 Modified IMG_sImageBufCB to support new formats

 Revision 1.29  2004/06/30 16:26:24  thw
 Changed members 'i32Gain' and 'i32DemodulationAngle' to unsigned in colour space descriptor structure.

 Revision 1.28  2004/06/29 15:22:35  thw
 Replaced older style custom scaler setup coefficient fields with array based ones.

 Revision 1.27  2004/06/17 11:28:30  emb
 Added IMG_PIXFMT_PL12Y8 and IMG_PIXFMT_PL12Y10 pixel formats.

 Revision 1.26  2004/06/11 16:25:39  mxb
 Added bTiled & ui32htile to image Buffer CB

 Revision 1.25  2004/05/26 14:30:36  mxb
 Added NO_IRQ

 Revision 1.24  2004/05/04 11:50:15  emb
 Added ui32Initial1 to IMG_sScalerConfigData.

 Revision 1.23  2004/05/04 09:11:29  grc
 Added IMG_PIXFORMAT_A4 to list.

 Revision 1.22  2004/04/21 14:51:27  rml
 Add BFM_AllocSecondaryCB.

 Revision 1.21  2004/04/19 15:37:05  ajp
 Added IMG_CSC_CUSTOM to the CSC Enumerated types.

 Revision 1.20  2004/04/15 11:29:09  ajp
 re-ordered video matrix input and output offsets

 Revision 1.19  2004/04/01 09:18:10  grc
 Tidied up doxygen comments.

 Revision 1.18  2004/03/30 14:15:14  grc
 Added IMG_PIXFMT_CLUT2 and IMG_PIXFMT_CLUT4
 to IMG_ePixelFormat enum.

 Revision 1.17  2004/03/23 14:33:13  emb
 Removed PL12Y8, PL12Y10, 420PL12UV8, 422PL12UV8, 420PL12UV10 and
 422PL12UV10 from IMG_ePixelFormat. These formats are not implemented and
 420PL12YUV8, 422PL12YUV8, 420PL12YUV10 and 422PL12YUV10 should be
 used instead (which describe both Y and UV components).

 Revision 1.16  2004/03/22 17:58:06  rml
 no message

 Revision 1.15  2004/03/22 17:48:27  rml
 Make platform independant

 Revision 1.14  2004/03/18 12:05:57  grc
 Added IMG_PIXFMT_RGB332 instance to IMG_ePixelFormat enum.

 Revision 1.13  2004/03/04 16:22:31  emb
 Changed SCC_MAXINTPT to 16.

 Revision 1.12  2004/03/04 15:24:44  thw
 Added linked list pointer, to allow 'chains' of buffers.

 Revision 1.11  2004/03/02 17:02:37  emb
 no message

 Revision 1.10  2004/03/02 17:01:02  emb
 no message

 Revision 1.9  2004/03/02 14:16:16  emb
 Renamed the four PL12UV pixel formats to PL12YUV.

 Revision 1.8  2004/03/02 11:09:42  emb
 Added pixel format IMG_PIXFMT_YUV8.

 Revision 1.7  2004/02/24 12:38:02  thw
 Added alpha only pixel 'colour' format, IMG_PIXFMT_A8.

 Revision 1.6  2004/02/24 10:50:52  rml
 Move time macros

 Revision 1.5  2004/02/23 17:09:08  grc
 Added comment for new VYAUYA_8888 pixel format.

 Revision 1.4  2004/02/19 10:37:26  emb
 Added extra colour formats to IMG_ePixelFormat.

 Revision 1.3  2004/02/13 10:12:58  emb
 Added ui32FilterOrder to IMG_sScalerConfigData.

 Revision 1.2  2004/02/12 11:07:08  rml
 Brought forward from previous repository.

 Revision 1.53  2004/02/11 17:19:54  emb
 no message

 Revision 1.52  2004/02/11 12:51:09  emb
 Fixed typo in sScalerConfigData.ui32Intial

 Revision 1.51  2004/02/11 11:45:23  emb
 Added uint32InitialSize to sScalerConfigData

 Revision 1.50  2004/02/09 12:53:57  emb
 Changed aui8Coeff in sScalerConfigData to aui32CoeffRegs.

 Revision 1.49  2004/02/06 18:22:37  mjd
 Added Power Down enumeration to IMG_eInputActiveSettings

 Revision 1.48  2004/02/05 11:57:19  emb
 Added SCC_MAXTAP and SCC_MAXINTPT defines (moved from scc.h).  Also added some extra members to IMG_sScalerConfigData, including a coeff array aui8Coeffs.

 Revision 1.47  2004/02/03 17:04:03  ajp
 no message

 Revision 1.46  2004/02/03 13:13:38  rml
 Meson tidy-up

 Revision 1.45  2004/01/23 10:43:25  thw
 Merged with Mike Brown's changes - 'IMG_sImageBufCB' updated. UV component of buffers now located separately.

 Revision 1.44  2004/01/15 15:02:15  thw
 Forced value of first member of 'IMG_ePixelFormat' enumeration to 0x00.
 Added 'IMG_NO_OF_PIXEL_COLOUR_FORMATS' place holder to end of  'IMG_ePixelFormat' enumeration.

 Revision 1.43  2003/12/22 13:35:48  rml
 Stretch Blit modifications

 Revision 1.42  2003/12/19 13:35:45  thw
 Added 'NO_OF_TYPES' member to enumeration 'IMG_eBufferFormat'.

 Revision 1.41  2003/12/19 13:10:21  ajp
 no message

 Revision 1.40  2003/12/18 13:26:53  ajp
 no message

 Revision 1.39  2003/12/11 15:20:21  ajp
 Removed obsolete IMG_sCSCConfigDataF structure and moved to CSC Api

 Revision 1.38  2003/12/11 12:17:47  thw
 Added IMG_sXY structure.

 Revision 1.37  2003/12/09 14:49:14  ajp
 no message

 Revision 1.36  2003/12/08 11:43:48  rml
 Add Neutrino pixel formats

 Revision 1.35  2003/11/20 17:03:01  mxb
 Sort out timer ticks for real hardware

 Revision 1.34  2003/11/18 16:21:12  ajp
 Revised API

 Revision 1.33  2003/11/13 16:48:23  ajp
 no message

 Revision 1.32  2003/11/12 16:31:14  ajp
 no message

 Revision 1.31  2003/11/11 13:28:14  lab
 no message

 Revision 1.30  2003/11/07 13:10:24  rml
 Add application specific data pointer to buffer control block

 Revision 1.29  2003/11/06 13:29:52  ajp
 no message

 Revision 1.28  2003/10/31 11:34:33  thw
 Added 'buffer_format_single_field'.

 Revision 1.27  2003/10/22 16:28:03  hs
 Updated for CSC correctly

 Revision 1.26  2003/10/22 15:46:37  ajp
 no message

 Revision 1.25  2003/10/13 17:49:51  aja
 First checkin of Ensigma API definitions

 Revision 1.24  2003/10/10 17:12:21  ajp
 no message

 Revision 1.23  2003/10/08 14:07:39  hs
 Brought CSC and scaler in line with common code

 Revision 1.22  2003/10/03 14:50:14  ajp
 no message

 Revision 1.21  2003/09/22 12:44:31  rml
 Change to use lists

 Revision 1.20  2003/09/11 11:12:12  rml
 Sort-out MeOS timers.

 Revision 1.19  2003/09/11 11:07:39  ajp
 *** empty log message ***

 Revision 1.18  2003/09/09 16:03:56  rml
 Add KRN_POOLLINK to buffer control block.

 Revision 1.17  2003/08/28 10:54:10  hs
 Added MILLISECOND_TO_TICK(x) and CENTISECOND_TO_TICK(x) macros

 Revision 1.16  2003/08/11 15:09:43  rml
 Add scaler config support.

 Revision 1.15  2003/08/05 13:48:13  ajp
 *** empty log message ***

 Revision 1.14  2003/08/01 09:45:22  ajp
 *** empty log message ***

 Revision 1.13  2003/07/31 13:04:05  ajp
 *** empty log message ***

 Revision 1.12  2003/07/31 10:56:13  ajp
 *** empty log message ***

 Revision 1.11  2003/07/29 11:31:30  lab
 *** empty log message ***

 Revision 1.10  2003/07/29 09:37:30  ajp
 no message

 Revision 1.9  2003/07/25 13:44:55  ajp
 no message

 Revision 1.8  2003/07/25 13:07:45  ajp
 no message

 Revision 1.7  2003/07/24 10:17:48  ajp
 Moved color space structs from ACI to Common

 Revision 1.6  2003/07/16 10:53:33  rml
 Add support for noise reduction pixel formats

 Revision 1.5  2003/07/16 09:58:13  ajp
 no message

 Revision 1.4  2003/07/15 12:32:50  rml
 Add support for remaiing Meson pixel formats

 Revision 1.3  2003/07/03 12:14:45  rml
 Small formating change

 Revision 1.2  2003/07/03 10:43:08  rml
 Tidy-up file headers

 Revision 1.1  2003/07/01 16:03:27  hs
 Initial checkin of the new 'consumerav' CVS tree structure.

 This is major commit #1 of 3. There will be another major commit based
 upon the IMG Framework application and libraries, as well as another
 for scripts and tagging.

 -----> THIS IS NOT YET A USEABLE STRUCTURE <-----

 Revision 1.10  2003/06/26 14:57:56  ajp
 no message

 Revision 1.9  2003/06/26 12:22:19  ajp
 no message

 Revision 1.8  2003/06/24 16:31:37  ajp
 no message

 Revision 1.7  2003/06/24 16:29:46  ajp
 no message

 Revision 1.6  2003/06/23 15:42:31  ajp
 no message

 Revision 1.5  2003/06/23 15:41:17  ajp
 no message

 Revision 1.4  2003/06/23 15:26:31  ajp
 Added some generic types for the API's

 Revision 1.3  2003/06/23 11:10:03  ajp
 no message

 Revision 1.2  2003/06/05 11:39:21  rml
 Various updates and changes to API documenation

 Revision 1.1.1.1  2003/06/04 14:37:23  rml
 no message


*****************************************************************************/

#include <time.h>
#include "img_defs.h"
#include "lst.h"

#if !defined (__IMG_COMMON_H__)
#define __IMG_COMMON_H__

#if (__cplusplus)
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


/*!
******************************************************************************

 This type defines the pixel formats

******************************************************************************/
typedef enum
{
    IMG_PIXFMT_CLUT1    = 0x00,                 //!< 1-bit palettised.
    IMG_PIXFMT_CLUT2,                           //!< 2-bit palettised.
    IMG_PIXFMT_CLUT4,                           //!< 4-bit palettised.
    IMG_PIXFMT_I4A4,							//!< 4-bit palettised + 4-bit alpha.
    IMG_PIXFMT_I8A8,							//!< 8-bit palettised + 8-bit alpha.
    IMG_PIXFMT_RGB8,                            //!< 8-bit palettised.
    IMG_PIXFMT_RGB332,                          //!< 8-bit RGB (R=3, G=3, B=2).
    IMG_PIXFMT_RGB555,                          //!< 15-bit RGB (R=5, G=5, B=5).
    IMG_PIXFMT_ARGB4444,                        //!< 16-bit RGB (R=4, G=4, B=4) + 4-bit alpha.
    IMG_PIXFMT_ARGB1555,                        //!< 16-bit RGB (R=5, G=5, B=5) + 1-bit alpha.
    IMG_PIXFMT_RGB565,                          //!< 16-bit RGB (R=5, G=6, B=5).
    IMG_PIXFMT_RGB888,                          //!< 24-bit RGB (packed).
    IMG_PIXFMT_ARGB8888,                        //!< 24-bit RGB + 8-bit alpha.

    IMG_PIXFMT_UYVY8888,                        //!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_VYUY8888,						//!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_YVYU8888,						//!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_YUYV8888,						//!< YUV 4:2:2 8-bit per component.
    IMG_PIXFMT_UYVY10101010,                    //!< YUV 4:2:2 10-bit per component.
    IMG_PIXFMT_VYAUYA8888,                      //!< YUV 4:2:2:4 8-bit per component.
    IMG_PIXFMT_YUV101010,                       //!< YUV 4:4:4 10-bit per component.
    IMG_PIXFMT_AYUV4444,                        //!< 12-bit YUV 4:4:4 + 4-bit alpha.
    IMG_PIXFMT_YUV888,                          //!< 24-bit YUV (packed).
    IMG_PIXFMT_AYUV8888,                        //!< 24-bit YUV 4:4:4 + 8-bit alpha.
    IMG_PIXFMT_AYUV2101010,                     //!< 30-bit YUV 4:4:4 + 2-bit alpha.

	IMG_PIXFMT_411PL12YVU8,                   	//!< Planar 8-bit 4:1:1 format.
    IMG_PIXFMT_420PL12YUV8,                     //!< Planar 8-bit 4:2:0 format.
    IMG_PIXFMT_420PL12YVU8,                     //!< Planar 8-bit 4:2:0 format - as per YUV8 but with reversed U and V samples.
    IMG_PIXFMT_422PL12YUV8,                     //!< Planar 8-bit 4:2:2 format.
    IMG_PIXFMT_422PL12YVU8,                     //!< Planar 8-bit 4:2:2 format - as per YUV8 but with reversed U and V samples.
    IMG_PIXFMT_420PL12YUV10,                    //!< Planar 10-bit 4:2:0 format.
    IMG_PIXFMT_422PL12YUV10,                    //!< Planar 10-bit 4:2:2 format.

    IMG_PIXFMT_420PL12YUV8_A8,                  //!< Planar 8-bit 4:2:0 format + 8-bit alpha.
    IMG_PIXFMT_422PL12YUV8_A8,                  //!< Planar 8-bit 4:2:2 format + 8-bit alpha.

    IMG_PIXFMT_PL12Y8,                          //!< Y only 8 bit
    IMG_PIXFMT_PL12Y10,                         //!< Y only 10 bit

    IMG_PIXFMT_PL12YV8,                         //!< Planar 8-bit 4:2:0 (Y data, followed by U data, followed by V data)
    IMG_PIXFMT_PL12IMC2,                        //!< Planar 8-bit 4:2:0 (Y data, followed by line of U, line of V, interleaved)

    IMG_PIXFMT_A4,                              //!< Alpha only 4 bit
    IMG_PIXFMT_A8,                              //!< Alpha only 8 bit
    IMG_PIXFMT_YUV8,                            //!< 8-bit palettised.
    IMG_PIXFMT_CVBS10,                          //!< Composite video (CVBS) data stored by the UCC for YC separation.

	IMG_PIXFMT_ABGR8888,						//!< 24-bit RGB + 8-bit alpha (ABGR).
	IMG_PIXFMT_BGRA8888,						//!< 24-bit RGB + 8-bit alpha (BGRA).
	IMG_PIXFMT_ARGB8332,						//!< 16-bit RGB (R=3, G=3, B=2) + 8-bit alpha.

    /* PLACE MARKER - NOT A VALID PIXEL COLOUR FORMAT */
    IMG_NO_OF_PIXEL_COLOUR_FORMATS

} IMG_ePixelFormat;


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

 This structure contains the image buffer information

 The pixels within the image buffers are grouped into "blocks-of-pixels" (BOPs)
 dependent upon the image type.

 BOPs are byte aligned structures that contains the data for a integral number
 of pixels and organised in a form suitable for the hardware.

******************************************************************************/
typedef struct IMG_sImageBufCB_tag
{
    LST_LINK;								//!< List link (allows the buffer control block to be part of a MeOS list).
    img_uint32          ui32BufferPool;     //!< Buffer pool.
    img_uint32          ui32PoolID;         //!< Buffer ID.
    IMG_ePixelFormat    ePixelFormat;       //!< Pixel format of data.
    IMG_eBufferFormat   eBufferFormat;      //!< Buffer format
    img_bool            bPlanar;            //!< IMG_TRUE if planar, otherwise IMG_FALSE.
    img_bool            bAlpha;             //!< IMG_TRUE if alpha plane, otherwise IMG_FALSE.
    img_uint32          ui32ImageWidth;     //!< Width of the image (in pixels).
    img_uint32          ui32ImageHeight;    //!< Height or the image (in lines).
    img_uint32          ui32ImageStride;	//!< The stride of the image (in bytes).
    /*! IMG_TRUE if the stride of UV plane is half that of the Y
     *  plane, otherwise IMG_FALSE.  Only valid if bPlanar is IMG_TRUE.
     */
    img_bool            bUVStrideHalved;
    /*! IMG_TRUE if the height of UV plane is half that of the Y
     *  plane, otherwise IMG_FALSE.  Only valid if bPlanar is IMG_TRUE.
     */
    img_bool            bUVHeightHalved;
    IMG_eImageFieldType eImageFieldType;    //!< Image field type.
    img_uint32          ui32BufSize;        //!< Size of the buffer
    img_uint32          ui32UVBufSize;      //!< Size of UV buffer - if Planar
    img_uint32          ui32AlphaBufSize;   //!< Size of Alpha buffer - if present
    img_uint32          ui32BufAlignment;   //!< Alignment of the buffer (1, 2, 4, 8, 16, 32...)
    /* Pointer to an associated set of CLUT structures.  Only valid for IMG_PIXFMT_RGB8 format
     *   images, but can also be IMG_NULL if not palette has been associated with this image.
     */
    IMG_sClut *         psClut;
    img_uint8 *         pvBufBase;          //!< The buffer base address.  NOTE: This address is non-aligned and should NOT be used outside of the Buffer Management System
    img_uint8 *         pvUVBufBase;
    img_uint8 *         pvAlphaBufBase;

    img_uint32          ui32YPixelInBOPs;   //!< Number of pixels in a block-of-pixels.
    img_uint32          ui32YBytesInBOPs;   //!< Number of bytes in a block-of-pixels.

    img_uint8 *         pvYBufAddr;         //!< Y or non-planar buffer address
    img_uint32          ui32UVBytesInBOPs;  //!< Number of bytes in a block-of-pixels in the UV plane.  Only valid if bPlanar is IMG_TRUE.
    img_uint8 *         pvUVBufAddr;        //!< UV buffer address.  Only valid if bPlanar is IMG_TRUE.

    img_uint32          ui32AlphaBytesInBOPs;
    img_uint8 *         pvAlphaBufAddr;     //!< Alpha buffer address.  Only valid in PL+A formats.

    img_void *          pApplicationData;   //!< Pointer to application specific data.  IMG_NULL if not data has been associated with this buffer.

    img_uint32          ui32MemSpaceId;     //!< Device dependant Memory Space ID
    img_handle			hShadowMem;         //!< Handle to shadow memory associated with this buffer
    img_handle			hShadowMemUV;
    img_handle			hShadowMemAlpha;

    img_bool            bTiled;             //!< IMG_TRUE if buffer tiled
    img_uint32          ui32htile;          //!< N - refer to MC_DDR spec for details
    img_uint32          ui32HwStride;       //!< h/w stride - used for writes to hardware regardless of whether tiling is on or not
    
    /*!< If non zero then the start of every line (i.e.: the buff start address AND the line stride 	*/
    /*!< will be rounded to the nearest multiple of this number.										*/
    /*!< Note : 	Unless the 'BFM_FLAGS_FORCE_LINE_START_ALIGNMENT' is set in the flags word, this	*/
    /*!< 			field will be ignored, and line alignment will not be performed.					*/
    img_uint32		ui32ForceLineStartAlignment;
    
    struct IMG_sImageBufCB_tag * psNextImageBufCB; //!< Pointer to the next buffer in a chain

    struct IMG_sImageBufCB_tag * psPrimImageBufCB; //!< Pointer to the primary buffer control block (if this is a secondary), IMG_NULL if this is a primary.
    img_uint32          ui32NoSec;          //!< The number of associated secondary buffer control blocks (only valid if this is a primary).
    img_bool            bPrimFreed;         //!< Flag to indicate that the primary has been freed.

    IMG_eImageFieldMode eImageFieldMode;    //!< Image field display mode.

    img_uint32          ui32FlagsWord;      //!< Configuration flags.  See #BFM_eFlagsWord.
    img_bool            bAllocated;         //!< IMG_TRUE until this CB has been made free. Used to trap free twice.
	img_uint32			Time;				//!< Timestamp. This is typically set in the VPIN API.
	/*! The rms noise level estimated during the vertical blanking region, in IRE units.
	16 fractional bits. */
	img_int32			i32NoiseLevelEstimate;

    img_int32  i32VbiFrameXPos;				//!< VBI First available pixel on each line of the frame
	img_int32  i32VbiFrameYPos;				//!< VBI Line number of first frame line in the buffer in terms of standard line numbering system
	img_uint32 ui32VbiActiveWidth;			//!< VBI Width in pixels of active area of the image buffer
	img_uint32 ui32VbiActiveHeight;			//!< VBI Height in lines of the active area of the image buffer
	img_int32  i32VbiChromaRef;				//!< Chroma phase measurement taken on WideClearVision VBI line
	img_int32  i32VbiChromaPhaseInc;		//!< Chroma phase increment taken on WideClearVision VBI line

	img_int32  i32VbiBlankLevel;			//!< VBI Blanking level
	img_int32  i32VbiWhiteLevel;            //!< VBI Peak white level

	img_int32	ui32VbiSourceFrameWidth;	//!< VBI Source frame width
	img_int32	ui32VbiDestFrameWidth;		//!< VBI Destination frame width

	IMG_eSampleRate		eSampleRate;		//!< Video sample rate
	IMG_ePictureType	ePictureType;		//!< Indicates whether FRAME, FIELD_TOP or FIELD_BOTTOM
	img_uint32	ui32DisplayFrameNum;		//!< The frame number in display order
	img_uint8	ui8DisplayCount;			//!< The number of times to present this frame
	img_uint8	ui8Encoding;				//!< The encoding type (I, P or B) of the picture
	
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

    API_ID_MAX                      //!< Max. API Id.

} SYS_eApiId;


#if (__cplusplus)
}
#endif

#endif /* __IMG_COMMON_H__  */
